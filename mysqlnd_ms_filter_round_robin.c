/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2012 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Andrey Hristov <andrey@php.net>                              |
  |         Ulf Wendel <uw@php.net>                                      |
  |         Johannes Schlueter <johannes@php.net>                        |
  +----------------------------------------------------------------------+
*/

/* $Id: mysqlnd_ms.c 311179 2011-05-18 11:26:22Z andrey $ */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_priv.h"
#ifndef mnd_emalloc
#include "ext/mysqlnd/mysqlnd_alloc.h"
#endif
#include "mysqlnd_ms.h"
#include "mysqlnd_ms_switch.h"
#include "mysqlnd_ms_enum_n_def.h"
#include "mysqlnd_ms_lb_weights.h"
#include "mysqlnd_ms_config_json.h"

/* {{{ mysqlnd_ms_filter_rr_context_dtor */
static void
mysqlnd_ms_filter_rr_context_dtor(void * data)
{
#if U0
	MYSQLND_MS_FILTER_RR_CONTEXT * context = * (MYSQLND_MS_FILTER_RR_CONTEXT **) data;
	TSRMLS_FETCH();
	if (context) {
		mnd_pefree(context, 1);
	}
#endif
}
/* }}} */


/* {{{ rr_filter_dtor */
static void
rr_filter_dtor(struct st_mysqlnd_ms_filter_data * pDest TSRMLS_DC)
{
	MYSQLND_MS_FILTER_RR_DATA * filter = (MYSQLND_MS_FILTER_RR_DATA *) pDest;
	DBG_ENTER("rr_filter_dtor");

	zend_hash_destroy(&filter->master_context);
	zend_hash_destroy(&filter->slave_context);
	zend_hash_destroy(&filter->lb_weight);
	mnd_pefree(filter, filter->parent.persistent);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_filter_rr_reset_current_weight */
static void
mysqlnd_ms_filter_rr_reset_current_weight(void * data TSRMLS_DC)
{
	MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT * context = *(MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT **) data;
	context->lb_weight->current_weight = context->lb_weight->weight;
}
/* }}} */


/* {{{ mysqlnd_ms_rr_filter_ctor */
MYSQLND_MS_FILTER_DATA *
mysqlnd_ms_rr_filter_ctor(struct st_mysqlnd_ms_config_json_entry * section, zend_llist * master_connections, zend_llist * slave_connections, MYSQLND_ERROR_INFO * error_info, zend_bool persistent TSRMLS_DC)
{
	MYSQLND_MS_FILTER_RR_DATA * ret;
	DBG_ENTER("mysqlnd_ms_rr_filter_ctor");
	DBG_INF_FMT("section=%p", section);
	/* section could be NULL! */
	ret = mnd_pecalloc(1, sizeof(MYSQLND_MS_FILTER_RR_DATA), persistent);
	if (ret) {
		ret->parent.filter_dtor = rr_filter_dtor;
		zend_hash_init(&ret->master_context, 4, NULL/*hash*/, mysqlnd_ms_filter_rr_context_dtor, persistent);
		zend_hash_init(&ret->slave_context, 4, NULL/*hash*/, mysqlnd_ms_filter_rr_context_dtor, persistent);
		zend_hash_init(&ret->lb_weight, 4, NULL/*hash*/, mysqlnd_ms_filter_lb_weigth_dtor, persistent);

		/* roundrobin => array(weights  => array(name => w, ... )) */
		if (section &&
			(TRUE == mysqlnd_ms_config_json_section_is_list(section TSRMLS_CC) &&
			TRUE == mysqlnd_ms_config_json_section_is_object_list(section TSRMLS_CC)))
		{
			struct st_mysqlnd_ms_config_json_entry * subsection = NULL;
			do {
				char * current_subsection_name = NULL;
				size_t current_subsection_name_len = 0;

				subsection = mysqlnd_ms_config_json_next_sub_section(section,
																	&current_subsection_name,
																	&current_subsection_name_len,
																	NULL TSRMLS_CC);
				if (!subsection) {
					break;
				}
				if (!strcmp(current_subsection_name, SECT_LB_WEIGHTS)) {
					mysqlnd_ms_filter_ctor_load_weights_config(&ret->lb_weight, PICK_RROBIN, subsection, master_connections,  slave_connections, error_info, persistent TSRMLS_CC);
					break;
				}
			} while (1);
		}
	}
	DBG_RETURN((MYSQLND_MS_FILTER_DATA *) ret);
}
/* }}} */


/* {{{ mysqlnd_ms_choose_connection_rr_use_slave */
static MYSQLND_CONN_DATA *
mysqlnd_ms_choose_connection_rr_use_slave(zend_llist * master_connections,
										  zend_llist * slave_connections,
										  MYSQLND_MS_FILTER_RR_DATA * filter,
										  struct mysqlnd_ms_lb_strategies * stgy,
										  enum enum_which_server * which_server,
										  MYSQLND_ERROR_INFO * error_info TSRMLS_DC)
{
	unsigned int * pos;
	unsigned int i = 0;
	MYSQLND_MS_LIST_DATA * element = NULL;
	MYSQLND_CONN_DATA * connection = NULL;
	smart_str fprint = {0};
	zend_llist * l = slave_connections;
	MYSQLND_MS_FILTER_RR_CONTEXT * context = NULL;
	unsigned int retry_count = 0;

	DBG_ENTER("mysqlnd_ms_choose_connection_rr_use_slave");
	*which_server = USE_SLAVE;

	do {
			if (0 == zend_llist_count(l) && SERVER_FAILOVER_MASTER == stgy->failover_strategy) {
				*which_server = USE_MASTER;
				DBG_RETURN(connection);
			}
			/* LOCK on context ??? */
			{
				mysqlnd_ms_get_fingerprint(&fprint, l TSRMLS_CC);
				DBG_INF_FMT("fingerprint=%*s", fprint.len, fprint.c);
				if (SUCCESS != zend_hash_find(&filter->slave_context, fprint.c, fprint.len /*\0 counted*/, (void **) &context)) {
					int retval;
					DBG_INF("Init the slave context");
					/* persistent = 1 to be on the safe side */
					context = mnd_pecalloc(1, sizeof(MYSQLND_MS_FILTER_RR_CONTEXT), 1);
					context->pos = 0;
					mysqlnd_ms_weight_list_init(&context->weight_list TSRMLS_CC);

					retval = zend_hash_add(&filter->slave_context, fprint.c, fprint.len /*\0 counted*/, context, sizeof(MYSQLND_MS_FILTER_RR_CONTEXT), NULL);

					if (SUCCESS == retval) {
						/* fetch ptr to the data inside the HT */
						retval = zend_hash_find(&filter->slave_context, fprint.c, fprint.len /*\0 counted*/, (void**)&context);
					}
					smart_str_free(&fprint);
					if (SUCCESS != retval) {
						break;
					}

					pos = &(context->pos);
					if (zend_hash_num_elements(&filter->lb_weight)) {
						/* sort list for weighted load balancing */
						if (PASS != mysqlnd_ms_populate_weights_sort_list(&filter->lb_weight, &context->weight_list, l TSRMLS_CC)) {
							break;
						}
						DBG_INF_FMT("Sort list has %d elements", zend_llist_count(&context->weight_list));
					}
				} else {
					smart_str_free(&fprint);
					pos = &(context->pos);
				}
				DBG_INF_FMT("look under pos %u", *pos);
			}
			do {
				retry_count++;

				if (zend_llist_count(&context->weight_list)) {
					zend_llist_position	tmp_pos;
					MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT * lb_weight_context, ** lb_weight_context_pp;
					DBG_INF("Sorting");
					/*
					Round 0, sort list
					slave 1, current_weight 3 -->  pick, current_weight--
					slave 2, current_weight 2
					slave 3, current_weight 1

					Round 1, sort list
					slave 1, current_weight 2 --> pick, current_weight--
					slave 2, current_weight 2
					slave 3, current_weight 1

					Round 2, sort list
					slave 2, current_weight 2 --> pick, current_weight--
					slave 1, current_weight 1
					slave 3, current_weight 1
					  NOTE: slave 1, slave 3 ordering is undefined/implementation dependent!

					Round 3, sort list
					slave 2, current_weight 1 --> pick, current_weight--, reset
					slave 1, current_weight 1
					slave 3, current_weight 1

					Round 4, sort list
					slave 1, current_weight 1 --> pick, current_weight--
					slave 3, current_weight 1
					slave 2, current_weight 0

					Round 5, sort list
					slave 3, current_weight 1 --> pick, current_weight--
					slave 2, current_weight 0
					slave 1, current_weight 0

					Round 6, sort list
					slave 3, current_weight 0 --> RESET -> 1 --> sort again
					slave 2, current_weight 0           -> 2
					slave 1, current_weight 0           -> 3
					*/
					do {
						mysqlnd_ms_weight_list_sort(&context->weight_list TSRMLS_CC);
						lb_weight_context_pp = (MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT **)zend_llist_get_first_ex(&context->weight_list, &tmp_pos);

						if (lb_weight_context_pp && (lb_weight_context = *lb_weight_context_pp)) {
							element = lb_weight_context->element;
							DBG_INF_FMT("element %p current_weight %d", element, lb_weight_context->lb_weight->current_weight);
							if (0 == lb_weight_context->lb_weight->current_weight) {
								/* RESET */
								zend_llist_apply(&context->weight_list, mysqlnd_ms_filter_rr_reset_current_weight TSRMLS_CC);
								continue;
							}
							lb_weight_context->lb_weight->current_weight--;
						} else {
							DBG_INF("Sorting failed");
						}
					} while (0);
				} else	{
					BEGIN_ITERATE_OVER_SERVER_LIST(element, l);
						if (i == *pos) {
							break;
						}
						i++;
					END_ITERATE_OVER_SERVER_LIST;
					DBG_INF_FMT("i=%u pos=%u", i, *pos);
				}
				if (!element) {
					/* there is no such safe guard in the random filter. Random tests for connection */
					if ((SERVER_FAILOVER_LOOP == stgy->failover_strategy) &&
						((0 == stgy->failover_max_retries) || (retry_count <= stgy->failover_max_retries))) {
						DBG_INF("Trying next slave, if any");
						/* time to increment the position */
						*pos = ((*pos) + 1) % zend_llist_count(l);
						DBG_INF_FMT("pos is now %u", *pos);
						continue;
					}
					/* unlikely */
					{
						char error_buf[256];
						snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Couldn't find the appropriate slave connection. %d slaves to choose from. Something is wrong", zend_llist_count(l));
						error_buf[sizeof(error_buf) - 1] = '\0';
						SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
						DBG_ERR_FMT("%s", error_buf);
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_buf);
						DBG_RETURN(NULL);
					}
				}
				/* time to increment the position */
				*pos = ((*pos) + 1) % zend_llist_count(l);
				DBG_INF_FMT("pos is now %u", *pos);
				if (element->conn) {
					connection = element->conn;
				}
				if (connection) {
					smart_str fprint_conn = {0};
					DBG_INF_FMT("Using slave connection "MYSQLND_LLU_SPEC"", connection->thread_id);

					if (stgy->failover_remember_failed) {
						zend_bool * failed;
						mysqlnd_ms_get_fingerprint_connection(&fprint_conn, &element TSRMLS_CC);
						if (SUCCESS == zend_hash_find(&stgy->failed_hosts, fprint_conn.c, fprint_conn.len /*\0 counted*/, (void **) &failed)) {
							smart_str_free(&fprint_conn);
							DBG_INF("Skipping previously failed connection");
							continue;
						}
					}

					if (CONN_GET_STATE(connection) > CONN_ALLOCED || PASS == mysqlnd_ms_lazy_connect(element, FALSE TSRMLS_CC)) {
						MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_SLAVE);
						SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(connection));
						if (stgy->failover_remember_failed) {
							smart_str_free(&fprint_conn);
						}
						DBG_RETURN(connection);
					}

					if (stgy->failover_remember_failed) {
						zend_bool failed = TRUE;
						if (SUCCESS != zend_hash_add(&stgy->failed_hosts, fprint_conn.c, fprint_conn.len /*\0 counted*/, &failed, sizeof(zend_bool), NULL)) {
							DBG_INF("Failed to remember failing connection");
						}
						smart_str_free(&fprint_conn);
					}

					if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
						DBG_INF("Failover disabled");
						DBG_RETURN(connection);
					} else if ((SERVER_FAILOVER_LOOP == stgy->failover_strategy) &&
						((0 == stgy->failover_max_retries) || (retry_count <= stgy->failover_max_retries))) {
						DBG_INF("Trying next slave, if any");
						continue;
					}
					DBG_INF("Falling back to the master");
					break;
				} else {
					/* unlikely */
					if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
						DBG_INF("Failover disabled");
						DBG_RETURN(NULL);
					} else if ((SERVER_FAILOVER_LOOP == stgy->failover_strategy) &&
						((0 == stgy->failover_max_retries) || (retry_count <= stgy->failover_max_retries))) {
						DBG_INF("Trying next slave, if any");
						continue;
					}
					DBG_INF("Falling back to the master");
					break;
				}
			} while (retry_count < zend_llist_count(l));

			if ((SERVER_FAILOVER_LOOP == stgy->failover_strategy) && (0 == zend_llist_count(master_connections))) {
				/* must not fall through as we'll loose the connection error */
				DBG_INF("No masters to continue search");
				DBG_RETURN(connection);
			}
			if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
				/*
				We may get here with remember_failed but no failover strategy set.
				TODO: Is this a valid configuration at all?
				*/
				DBG_INF("Failover disabled");
				DBG_RETURN(connection);
			}
			retry_count = 0;
			/* UNLOCK of context ??? */
	} while (0);
	*which_server = USE_MASTER;
	DBG_RETURN(NULL);
}
/* }}} */

/* {{{ mysqlnd_ms_choose_connection_rr_use_master */
static MYSQLND_CONN_DATA *
mysqlnd_ms_choose_connection_rr_use_master(zend_llist * master_connections,
										   MYSQLND_MS_FILTER_RR_DATA * filter,
										   struct mysqlnd_ms_lb_strategies * stgy,
										   zend_bool forced_tx_master,
										   MYSQLND_ERROR_INFO * error_info TSRMLS_DC)
{
	zend_llist * l = master_connections;
	unsigned int * pos;
	unsigned int i = 0;
	MYSQLND_MS_LIST_DATA * element = NULL;
	MYSQLND_CONN_DATA * connection = NULL;
	MYSQLND_MS_FILTER_RR_CONTEXT * context;
	unsigned int retry_count = 0;

	DBG_ENTER("mysqlnd_ms_choose_connection_rr");
	do {
		smart_str fprint = {0};
		mysqlnd_ms_get_fingerprint(&fprint, l TSRMLS_CC);
		if (SUCCESS != zend_hash_find(&filter->master_context, fprint.c, fprint.len /*\0 counted*/, (void **) &context)) {
			int retval;
			DBG_INF("Init the master context");
			/* persistent = 1 to be on the safe side */
			context = mnd_pecalloc(1, sizeof(MYSQLND_MS_FILTER_RR_CONTEXT), 1);
			context->pos = 0;
			mysqlnd_ms_weight_list_init(&context->weight_list TSRMLS_CC);

			retval = zend_hash_add(&filter->master_context, fprint.c, fprint.len /*\0 counted*/, context, sizeof(MYSQLND_MS_FILTER_RR_CONTEXT), NULL);
			if (SUCCESS == retval) {
				/* fetch ptr to the data inside the HT */
				retval = zend_hash_find(&filter->master_context, fprint.c, fprint.len /*\0 counted*/, (void**)&context);
			}
			smart_str_free(&fprint);
			if (SUCCESS != retval) {
				break;
			}

			pos = &(context->pos);
			if (zend_hash_num_elements(&filter->lb_weight)) {
				/* sort list for weighted load balancing */
				if (PASS != mysqlnd_ms_populate_weights_sort_list(&filter->lb_weight, &context->weight_list, l TSRMLS_CC)) {
					break;
				}
				DBG_INF_FMT("Sort list has %d elements", zend_llist_count(&context->weight_list));
			}
		} else {
			smart_str_free(&fprint);
			pos = &(context->pos);
		}
		if (0 == zend_llist_count(l)) {
			char error_buf[256];
			snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Couldn't find the appropriate master connection. %d masters to choose from. Something is wrong", zend_llist_count(l));
			error_buf[sizeof(error_buf) - 1] = '\0';
			SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
			DBG_ERR_FMT("%s", error_buf);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_buf);
			DBG_RETURN(NULL);
		}
		while (retry_count < zend_llist_count(l)) {
			retry_count++;

			if (zend_llist_count(&context->weight_list)) {
				zend_llist_position	tmp_pos;
				MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT * lb_weight_context, ** lb_weight_context_pp;
				DBG_INF("USE_MASTER sorting");

				do {
					mysqlnd_ms_weight_list_sort(&context->weight_list TSRMLS_CC);
					lb_weight_context_pp = (MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT **)zend_llist_get_first_ex(&context->weight_list, &tmp_pos);

					if (lb_weight_context_pp && (lb_weight_context = *lb_weight_context_pp)) {
						element = lb_weight_context->element;
						DBG_INF_FMT("element %p current_weight %d", element, lb_weight_context->lb_weight->current_weight);
						if (0 == lb_weight_context->lb_weight->current_weight) {
							/* RESET */
							zend_llist_apply(&context->weight_list, mysqlnd_ms_filter_rr_reset_current_weight TSRMLS_CC);
							continue;
						}
						lb_weight_context->lb_weight->current_weight--;
					} else {
						DBG_INF("Sorting failed");
					}
				} while (0);
			} else {
				DBG_INF_FMT("USE_MASTER pos=%lu", *pos);
				i = 0;
				BEGIN_ITERATE_OVER_SERVER_LIST(element, l);
					if (i == *pos) {
						break;
					}
					i++;
				END_ITERATE_OVER_SERVER_LIST;
			}
			connection = NULL;

			if (!element) {
				if ((SERVER_FAILOVER_LOOP == stgy->failover_strategy) &&
					((0 == stgy->failover_max_retries) || (retry_count <= stgy->failover_max_retries)))
				{
					/* we must move to the next position and ignore forced_tx_master */
					*pos = ((*pos) + 1) % zend_llist_count(l);
					DBG_INF("Trying next master, if any");
					continue;
				}
				{
					char error_buf[256];
					snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Couldn't find the appropriate master connection. %d masters to choose from. Something is wrong", zend_llist_count(l));
					error_buf[sizeof(error_buf) - 1] = '\0';
					SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
					DBG_ERR_FMT("%s", error_buf);
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_buf);
					DBG_RETURN(NULL);
				}
			}
			if (element->conn) {
				connection = element->conn;
			}
			DBG_INF("Using master connection");
			if (FALSE == forced_tx_master) {
				/* time to increment the position */
				*pos = ((*pos) + 1) % zend_llist_count(l);
			}
			if (connection) {
				smart_str fprint_conn = {0};

				if (stgy->failover_remember_failed) {
					mysqlnd_ms_get_fingerprint_connection(&fprint_conn, &element TSRMLS_CC);
					if (zend_hash_exists(&stgy->failed_hosts, fprint_conn.c, fprint_conn.len /*\0 counted*/)) {
						smart_str_free(&fprint_conn);
						if (TRUE == forced_tx_master) {
							/* we must move to the next position and ignore forced_tx_master */
							*pos = ((*pos) + 1) % zend_llist_count(l);
						}
						DBG_INF("Skipping previously failed connection");
						continue;
					}
				}

				if ((CONN_GET_STATE(connection) > CONN_ALLOCED || PASS == mysqlnd_ms_lazy_connect(element, TRUE TSRMLS_CC))) {
					MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_MASTER);
					SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(connection));
					if (stgy->failover_remember_failed) {
						smart_str_free(&fprint_conn);
					}
					DBG_RETURN(connection);
				}

				if (stgy->failover_remember_failed) {
					zend_bool failed = TRUE;
					if (SUCCESS != zend_hash_add(&stgy->failed_hosts, fprint_conn.c, fprint_conn.len /*\0 counted*/, &failed, sizeof(zend_bool), NULL)) {
						DBG_INF("Failed to remember failing connection");
					}
					smart_str_free(&fprint_conn);
				}

				if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
					DBG_INF("Failover disabled");
					DBG_RETURN(connection);
				} else if ((SERVER_FAILOVER_LOOP == stgy->failover_strategy) &&
					((0 == stgy->failover_max_retries) || (retry_count <= stgy->failover_max_retries))) {
					if (TRUE == forced_tx_master) {
						/* we must move to the next position and ignore forced_tx_master */
						*pos = ((*pos) + 1) % zend_llist_count(l);
					}
					DBG_INF("Trying next master, if any");
					continue;
				}
				DBG_RETURN(connection);
			} else {
				if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
					DBG_INF("Failover disabled");
					DBG_RETURN(NULL);
				} else if ((SERVER_FAILOVER_LOOP == stgy->failover_strategy) &&
					((0 == stgy->failover_max_retries) || (retry_count <= stgy->failover_max_retries))) {
					if (TRUE == forced_tx_master) {
						/* we must move to the next position and ignore forced_tx_master */
						*pos = ((*pos) + 1) % zend_llist_count(l);
					}
					DBG_INF("Trying next master, if any");
					continue;
				}
			}
			break;
		}
		DBG_RETURN(connection);
	} while (0);
	DBG_RETURN(NULL);
}
/* }}} */


/* {{{ mysqlnd_ms_choose_connection_rr */
MYSQLND_CONN_DATA *
mysqlnd_ms_choose_connection_rr(void * f_data, const char * const query, const size_t query_len,
								struct mysqlnd_ms_lb_strategies * stgy, MYSQLND_ERROR_INFO * error_info,
								zend_llist * master_connections, zend_llist * slave_connections,
								enum enum_which_server * which_server TSRMLS_DC)
{
	enum enum_which_server tmp_which;
	zend_bool forced;
	MYSQLND_MS_FILTER_RR_DATA * filter = (MYSQLND_MS_FILTER_RR_DATA *) f_data;
	zend_bool forced_tx_master = FALSE;
	MYSQLND_CONN_DATA * conn = NULL;
	DBG_ENTER("mysqlnd_ms_choose_connection_rr");

	if (!which_server) {
		which_server = &tmp_which;
	}
	*which_server = mysqlnd_ms_query_is_select(query, query_len, &forced TSRMLS_CC);
	if ((stgy->trx_stickiness_strategy == TRX_STICKINESS_STRATEGY_MASTER) && stgy->in_transaction && !forced) {
		DBG_INF("Enforcing use of master while in transaction");
		*which_server = USE_MASTER;
		forced_tx_master = TRUE;
		MYSQLND_MS_INC_STATISTIC(MS_STAT_TRX_MASTER_FORCED);
	} else if (stgy->mysqlnd_ms_flag_master_on_write) {
		if (*which_server != USE_MASTER) {
			if (stgy->master_used && !forced) {
				switch (*which_server) {
					case USE_MASTER:
					case USE_LAST_USED:
						break;
					case USE_SLAVE:
					default:
						DBG_INF("Enforcing use of master after write");
						*which_server = USE_MASTER;
						break;
				}
			}
		} else {
			DBG_INF("Use of master detected");
			stgy->master_used = TRUE;
		}
	}
	switch (*which_server) {
		case USE_SLAVE:
			conn = mysqlnd_ms_choose_connection_rr_use_slave(master_connections, slave_connections, filter, stgy, which_server, error_info TSRMLS_CC);
			if (NULL != conn || USE_MASTER != *which_server) {
				DBG_RETURN(conn);
			}
			DBG_INF("Fall-through to master");
			/* fall-through */
		case USE_MASTER:
			DBG_RETURN(mysqlnd_ms_choose_connection_rr_use_master(master_connections, filter, stgy, forced_tx_master, error_info TSRMLS_CC));
		case USE_LAST_USED:
			DBG_INF("Using last used connection");
			if (!stgy->last_used_conn) {
				char error_buf[256];
				snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Last used SQL hint cannot be used because last used connection has not been set yet. Statement will fail");
				error_buf[sizeof(error_buf) - 1] = '\0';
				SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
				DBG_ERR_FMT("%s", error_buf);
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_buf);
			} else {
				SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(stgy->last_used_conn));
			}
			DBG_RETURN(stgy->last_used_conn);
		default:
			/* error */
			DBG_RETURN(NULL);
			break;
	}
	DBG_RETURN(NULL);
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
