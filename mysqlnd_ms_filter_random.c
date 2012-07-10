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
#include "ext/standard/php_rand.h"
#include "mysqlnd_ms_enum_n_def.h"
#include "mysqlnd_ms_switch.h"
#include "mysqlnd_ms_config_json.h"
#include "mysqlnd_ms_lb_weights.h"

/* {{{ random_filter_dtor */
static void
random_filter_dtor(struct st_mysqlnd_ms_filter_data * pDest TSRMLS_DC)
{
	MYSQLND_MS_FILTER_RANDOM_DATA * filter = (MYSQLND_MS_FILTER_RANDOM_DATA *) pDest;
	DBG_ENTER("random_filter_dtor");

	zend_hash_destroy(&filter->sticky.master_context);
	zend_hash_destroy(&filter->sticky.slave_context);
	mnd_pefree(filter, filter->parent.persistent);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_random_filter_ctor */
MYSQLND_MS_FILTER_DATA *
mysqlnd_ms_random_filter_ctor(struct st_mysqlnd_ms_config_json_entry * section, zend_llist * master_connections, zend_llist * slave_connections, MYSQLND_ERROR_INFO * error_info, zend_bool persistent TSRMLS_DC)
{
	MYSQLND_MS_FILTER_RANDOM_DATA * ret;
	DBG_ENTER("mysqlnd_ms_random_filter_ctor");
	DBG_INF_FMT("section=%p", section);
	ret = mnd_pecalloc(1, sizeof(MYSQLND_MS_FILTER_RANDOM_DATA), persistent);
	if (ret) {
		ret->parent.filter_dtor = random_filter_dtor;
		zend_hash_init(&ret->lb_weight, 4, NULL/*hash*/, mysqlnd_ms_filter_lb_weigth_dtor, persistent);

		/* section could be NULL! */
		if (section) {
			/* random => array(sticky => true) */
			struct st_mysqlnd_ms_config_json_entry * subsection = NULL;
			zend_bool value_exists = FALSE, is_list_value = FALSE;
			char * once_value = mysqlnd_ms_config_json_string_from_section(section, PICK_ONCE, sizeof(PICK_ONCE) - 1, 0,
																		   &value_exists, &is_list_value TSRMLS_CC);
			if (value_exists && once_value) {
				ret->sticky.once = !mysqlnd_ms_config_json_string_is_bool_false(once_value);
				mnd_efree(once_value);
			}

			if ((TRUE == mysqlnd_ms_config_json_section_is_list(section TSRMLS_CC) &&
			TRUE == mysqlnd_ms_config_json_section_is_object_list(section TSRMLS_CC))) {
				struct st_mysqlnd_ms_config_json_entry * subsection = NULL;
				/* random => array(weights => ...) */
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
						mysqlnd_ms_filter_ctor_load_weights_config(&ret->lb_weight, PICK_RANDOM, subsection, master_connections,  slave_connections, error_info, persistent TSRMLS_CC);
						break;
					}
				} while (1);
			}

		} else {
			 /*
			   Stickiness by default when no filters section in the config
			   Implies NULL passed to this ctor.
			 */
			ret->sticky.once = TRUE;
		}
		DBG_INF_FMT("sticky=%d", ret->sticky.once);
		/* XXX: this could be initialized only in case of ONCE */
		zend_hash_init(&ret->sticky.master_context, 4, NULL/*hash*/, NULL/*dtor*/, persistent);
		zend_hash_init(&ret->sticky.slave_context, 4, NULL/*hash*/, NULL/*dtor*/, persistent);

		zend_hash_init(&ret->weight_context.master_context, 4, NULL/*hash*/, NULL/*dtor*/, persistent);
		zend_hash_init(&ret->weight_context.slave_context, 4, NULL/*hash*/, NULL/*dtor*/, persistent);
	}
	DBG_RETURN((MYSQLND_MS_FILTER_DATA *) ret);
}
/* }}} */


/* {{{ mysqlnd_ms_random_remove_conn */
static int
mysqlnd_ms_random_remove_conn(void * element, void * data) {
	MYSQLND_MS_LIST_DATA * entry = NULL, ** entry_pp = NULL;
	entry_pp = (MYSQLND_MS_LIST_DATA **)element;
	if (entry_pp && (entry = *entry_pp) && (entry == data)) {
		return 1;
	}
	return 0;
}
/* }}} */


/* {{{ mysqlnd_ms_random_sort_list_remove_conn */
static int
mysqlnd_ms_random_sort_list_remove_conn(void * element, void * data) {
	MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT * entry = NULL, ** entry_pp = NULL;
	entry_pp = (MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT **)element;
	if (entry_pp && (entry = *entry_pp) && (entry->element) && (entry->element == data)) {
		return 1;
	}
	return 0;
}
/* }}} */


/* {{{ mysqlnd_ms_init_sort_context */
static int
mysqlnd_ms_init_sort_context(HashTable *context, smart_str * fprint, zend_llist * server_list, HashTable * lb_weight, zend_llist * sort_list, unsigned int * total_weight TSRMLS_DC) {
	MYSQLND_MS_FILTER_RANDOM_LB_CONTEXT * lb_context = NULL;
	MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT * lb_weight_context, ** lb_weight_context_pp;
	zend_llist_position	pos;
	DBG_ENTER("mysqlnd_ms_init_sort_list");

	lb_context = mnd_pecalloc(1, sizeof(MYSQLND_MS_FILTER_RANDOM_LB_CONTEXT), 1);
	zend_llist_init(&lb_context->sort_list, sizeof(MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT *), NULL /* dtor */, 1);
	lb_context->total_weight = 0;

	if (SUCCESS != zend_hash_add(context, fprint->c, fprint->len /*\0 counted*/, lb_context, sizeof(MYSQLND_MS_FILTER_RANDOM_LB_CONTEXT), NULL)) {
		DBG_INF("Failed to add context");
		DBG_RETURN(FAIL);
	}
	/* fetch ptr to the data inside the HT */
	if (SUCCESS != zend_hash_find(context, fprint->c, fprint->len /*\0 counted*/, (void**)&lb_context)) {
		DBG_INF_FMT("Failed to get ptr to context - fingerprint='%s' len=%d", fprint->c, fprint->len);
		DBG_RETURN(FAIL);
	}

	if (PASS != mysqlnd_ms_populate_weights_sort_list(lb_weight, &lb_context->sort_list, server_list TSRMLS_CC)) {
		DBG_INF("Failed to populate weights sort list");
		DBG_RETURN(FAIL);
	}

	mysqlnd_ms_weight_list_sort(&lb_context->sort_list TSRMLS_CC);
	/* TODO: Move total counter into MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT ? */
	for (lb_weight_context_pp = (MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT **)zend_llist_get_first_ex(&lb_context->sort_list, &pos);
		(lb_weight_context_pp) && (lb_weight_context = *lb_weight_context_pp);
		lb_weight_context_pp = (MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT **)zend_llist_get_next_ex(&lb_context->sort_list, &pos))
	{
		lb_context->total_weight += lb_weight_context->lb_weight->weight;
	}

	/* we must copy as we remove entries during retry */
	mysqlnd_ms_weight_list_init(sort_list TSRMLS_CC);
	zend_llist_copy(sort_list, &lb_context->sort_list);
	*total_weight = lb_context->total_weight;

	DBG_RETURN(SUCCESS);
}
/* }}} */


/*
    Basic idea: summarize weights, compute random between 1... total_weight.
    Pick server that matches the rande of the random value.

	Example list, total_weight = 4, rnd_idx = 1..4
	slave 1, weight  2  -> rnx_idx = 1..2
	slave 2, weight  1  -> rnd_idx = 3
	slave 3, weight  1  -> rnd_idx = 4

	Without failover:
	Round 0 - rnd_idx = 3

	slave 1, weight  2  -> rnx_idx = 1..2
	slave 2, weight  1  -> rnd_idx = 3    --> try slave 2
	slave 3, weight  1  -> rnd_idx = 4


	With failover:

	Round 0 - total_weight = 4, rnd range 1..4, rnd = 3
	slave 1, weight  2  -> rnx_idx = 1..2
	slave 2, weight  1  -> rnd_idx = 3    --> try slave 2, fails
	slave 3, weight  1  -> rnd_idx = 4

	Trim list - total_weight = 3:
	slave 1, weight  2  -> rnx_idx = 1..2
	slave 3, weight  1  -> rnd_idx = 3

	Round 1 - total_weight = 3, rnd range 1..3, rnd = 1
	slave 1, weight  2  -> rnx_idx = 1..2 --> try slave 1, fails
	slave 3, weight  1  -> rnd_idx = 3

	Trim list - total_weight = 1:
	slave 3, weight  1  -> rnd_idx = 1

	Round 2 - total_weight = 1, rnd range 1..1, rnd = 1
	slave 3, weight  1  -> rnd_idx = 1 -> try slave 3, fails

	Trim list
	(empty)

	Either consider masters or give up.
*/

/* {{{ mysqlnd_ms_choose_connection_random_use_slave */
static MYSQLND_CONN_DATA *
mysqlnd_ms_choose_connection_random_use_slave(zend_llist * master_connections,
										  zend_llist * slave_connections,
										  MYSQLND_MS_FILTER_RANDOM_DATA * filter,
										  struct mysqlnd_ms_lb_strategies * stgy,
										  enum enum_which_server * which_server,
										  MYSQLND_ERROR_INFO * error_info TSRMLS_DC)
{
	smart_str fprint = {0};
	unsigned int retry_count = 0;
	zend_llist_position	pos;
	zend_llist * l = slave_connections;
	MYSQLND_MS_LIST_DATA * element = NULL, ** element_pp = NULL;
	unsigned long rnd_idx;
	unsigned int i = 0;
	MYSQLND_CONN_DATA * connection = NULL;
	MYSQLND_CONN_DATA ** context_pos;
	MYSQLND_MS_FILTER_RANDOM_LB_CONTEXT * lb_context = NULL;
	MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT * lb_weight_context, ** lb_weight_context_pp;
	zend_bool use_lb_context = FALSE;
	zend_llist sort_list;
	unsigned int total_weight;

	DBG_ENTER("mysqlnd_ms_choose_connection_random_use_slave");
	DBG_INF_FMT("%d slaves to choose from", zend_llist_count(l));
	do {
		if (0 == zend_llist_count(l) && (SERVER_FAILOVER_DISABLED == stgy->failover_strategy)) {
			/* SERVER_FAILOVER_MASTER and SERVER_FAILOVER_LOOP will fall through to master */
			mysqlnd_ms_client_n_php_error(error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, E_WARNING TSRMLS_CC,
										MYSQLND_MS_ERROR_PREFIX " Couldn't find the appropriate slave connection. "
										"%d slaves to choose from. Something is wrong", zend_llist_count(l));
			DBG_RETURN(NULL);
		}

		mysqlnd_ms_get_fingerprint(&fprint, l TSRMLS_CC);

		/* LOCK on context ??? */
		switch (zend_hash_find(&filter->sticky.slave_context, fprint.c, fprint.len /*\0 counted*/, (void **) &context_pos)) {
			case SUCCESS:
				smart_str_free(&fprint);
				connection = context_pos? *context_pos : NULL;
				if (!connection) {
					mysqlnd_ms_client_n_php_error(error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, E_WARNING TSRMLS_CC,
												  MYSQLND_MS_ERROR_PREFIX " Something is very wrong for slave random/once.");
				} else {
					DBG_INF_FMT("Using already selected slave connection "MYSQLND_LLU_SPEC, connection->thread_id);
					MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_SLAVE);
					SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(connection));
					DBG_RETURN(connection);
				}
				break;
			case FAILURE:
				if (zend_hash_num_elements(&filter->lb_weight)) {
					DBG_INF("Weighted load balancing");
					/* SEE COMMENT above the function*/
					use_lb_context = TRUE;
					if (FAILURE == zend_hash_find(&filter->weight_context.slave_context, fprint.c, fprint.len /*\0 counted*/, (void **)&lb_context)) {
						/* build sort list for weighted load balancing */
						if (SUCCESS != mysqlnd_ms_init_sort_context(&filter->weight_context.slave_context, &fprint, l, &filter->lb_weight, &sort_list, &total_weight TSRMLS_CC)) {
							break;
						}
					} else {
						zend_llist_copy(&sort_list, &lb_context->sort_list);
						total_weight = lb_context->total_weight;
					}
					DBG_INF_FMT("Sort list has %d elements, total_weight = %d", zend_llist_count(&sort_list), total_weight);
					if (0 == zend_llist_count(&sort_list)) {
						mysqlnd_ms_client_n_php_error(error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, E_WARNING TSRMLS_CC,
												  MYSQLND_MS_ERROR_PREFIX " Something is very wrong for slave random/once. The sort list is empty.");
						break;
					}
				}
				while (zend_llist_count(l) > 0) {
					retry_count++;

					rnd_idx = php_rand(TSRMLS_C);
					if (use_lb_context) {
						RAND_RANGE(rnd_idx, 1, total_weight, PHP_RAND_MAX);
						DBG_INF_FMT("USE_SLAVE weighted, rnd_idx=%lu", rnd_idx);
						for (i = 0, lb_weight_context_pp = (MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT **)zend_llist_get_first_ex(&sort_list, &pos);
								(lb_weight_context_pp) && (lb_weight_context = *lb_weight_context_pp);
								(lb_weight_context_pp = (MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT **)zend_llist_get_next_ex(&sort_list, &pos))) {
							i += lb_weight_context->lb_weight->weight;
							if (i >= rnd_idx)
								break;
						}
						connection = (lb_weight_context && (element = lb_weight_context->element) && element->conn) ? element->conn : NULL;
					} else {
						RAND_RANGE(rnd_idx, 0, zend_llist_count(l) - 1, PHP_RAND_MAX);
						DBG_INF_FMT("USE_SLAVE rnd_idx=%lu", rnd_idx);
						element_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(l, &pos);
						while (i++ < rnd_idx) {
							element_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(l, &pos);
						}
						connection = (element_pp && (element = *element_pp) && element->conn) ? element->conn : NULL;
					}

					if (!connection) {
						/* Q: how can we get here? */
						if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
							/* TODO: connection error would be better */
							mysqlnd_ms_client_n_php_error(error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, E_WARNING TSRMLS_CC,
											MYSQLND_MS_ERROR_PREFIX " Couldn't find the appropriate slave connection. "
											"%d slaves to choose from. Something is wrong", zend_llist_count(l));
							/* should be a very rare case to be here - connection shouldn't be NULL in first place */
							smart_str_free(&fprint);
							DBG_RETURN(NULL);
						} else	if ((SERVER_FAILOVER_LOOP == stgy->failover_strategy) &&
								((0 == stgy->failover_max_retries) || (retry_count <= stgy->failover_max_retries))) {
							/* drop failed server from list, test remaining slaves before fall-through to master */
							DBG_INF("Trying next slave, if any");
							zend_llist_del_element(l, element, mysqlnd_ms_random_remove_conn);
							if (use_lb_context) {
								total_weight -= lb_weight_context->lb_weight->weight;
								zend_llist_del_element(&sort_list, element, mysqlnd_ms_random_sort_list_remove_conn);
							}
							continue;
						}
						/* must be SERVER_FAILOVER_MASTER */
						break;
					} else {
						smart_str fprint_conn = {0};

						if (stgy->failover_remember_failed) {
							zend_bool * failed;
							mysqlnd_ms_get_fingerprint_connection(&fprint_conn, &element TSRMLS_CC);
							if (SUCCESS == zend_hash_find(&stgy->failed_hosts, fprint_conn.c, fprint_conn.len /*\0 counted*/, (void **) &failed)) {
								smart_str_free(&fprint);
								smart_str_free(&fprint_conn);
								zend_llist_del_element(l, element, mysqlnd_ms_random_remove_conn);
								if (use_lb_context) {
									total_weight -= lb_weight_context->lb_weight->weight;
									zend_llist_del_element(&sort_list, element, mysqlnd_ms_random_sort_list_remove_conn);
								}
								DBG_INF("Skipping previously failed connection");
								continue;
							}
						}

						if (CONN_GET_STATE(connection) > CONN_ALLOCED || PASS == mysqlnd_ms_lazy_connect(element, FALSE TSRMLS_CC)) {
							MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_SLAVE);
							SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(connection));
							if (TRUE == filter->sticky.once) {
								zend_hash_update(&filter->sticky.slave_context, fprint.c, fprint.len /*\0 counted*/, &connection,
												sizeof(MYSQLND *), NULL);
							}
							smart_str_free(&fprint);
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

						if ((SERVER_FAILOVER_LOOP == stgy->failover_strategy)  &&
								((0 == stgy->failover_max_retries) || (retry_count <= stgy->failover_max_retries))) {
							/* drop failed server from list, test remaining slaves before fall-through to master */
							DBG_INF("Trying next slave, if any");
							zend_llist_del_element(l, element, mysqlnd_ms_random_remove_conn);
							if (use_lb_context) {
								total_weight -= lb_weight_context->lb_weight->weight;
								zend_llist_del_element(&sort_list, element, mysqlnd_ms_random_sort_list_remove_conn);
							}
							continue;
						}

						if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
							/* no failover */
							DBG_INF("Failover disabled");
							smart_str_free(&fprint);
							DBG_RETURN(connection);
						}
						/* falling-through */
						break;
					}
				} /* while */
				smart_str_free(&fprint);
				if ((SERVER_FAILOVER_LOOP == stgy->failover_strategy) && (0 == zend_llist_count(master_connections))) {
					DBG_INF("No masters to continue search");
					/* must not fall through as we'll loose the connection error */
					DBG_RETURN(connection);
				}
				break;
		}/* switch (zend_hash_find) */

		if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
			/*
			We may get here with remember_failed but no failover strategy set.
			TODO: Is this a valid configuration at all?
			*/
			DBG_INF("Failover disabled");
			DBG_RETURN(connection);
		}
	} while (0);
	*which_server = USE_MASTER;
	DBG_RETURN(NULL);
}
/* }}} */


/* {{{ mysqlnd_ms_choose_connection_random_use_master */
static MYSQLND_CONN_DATA *
mysqlnd_ms_choose_connection_random_use_master(zend_llist * master_connections,
											   MYSQLND_MS_FILTER_RANDOM_DATA * filter,
											   struct mysqlnd_ms_lb_strategies * stgy,
											   zend_bool forced_tx_master,
											   MYSQLND_ERROR_INFO * error_info TSRMLS_DC)
{
	zend_llist_position	pos;
	zend_llist * l = master_connections;
	MYSQLND_MS_LIST_DATA * element = NULL, ** element_pp = NULL;
	unsigned long rnd_idx;
	unsigned int i = 0;
	MYSQLND_CONN_DATA * connection = NULL;
	MYSQLND_CONN_DATA ** context_pos;
	MYSQLND_MS_FILTER_RANDOM_LB_CONTEXT * lb_context = NULL;
	zend_bool use_lb_context = FALSE;
	MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT * lb_weight_context, ** lb_weight_context_pp;
	zend_llist sort_list;
	unsigned int total_weight;
	smart_str fprint = {0};
	unsigned int retry_count = 0;

	DBG_ENTER("mysqlnd_ms_choose_connection_random_use_master");
	DBG_INF_FMT("%d masters to choose from", zend_llist_count(l));
	do {
		DBG_INF_FMT("%d masters to choose from", zend_llist_count(l));
		if (0 == zend_llist_count(l)) {
			mysqlnd_ms_client_n_php_error(error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, E_WARNING TSRMLS_CC,
														MYSQLND_MS_ERROR_PREFIX " Couldn't find the appropriate master connection. "
														"%d masters to choose from. Something is wrong", zend_llist_count(l));
			DBG_RETURN(NULL);
		}
		mysqlnd_ms_get_fingerprint(&fprint, l TSRMLS_CC);

		/* LOCK on context ??? */
		switch (zend_hash_find(&filter->sticky.master_context, fprint.c, fprint.len /*\0 counted*/, (void **) &context_pos)) {
			case SUCCESS:
				connection = context_pos? *context_pos : NULL;
				smart_str_free(&fprint);
				if (!connection) {
					mysqlnd_ms_client_n_php_error(error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, E_WARNING TSRMLS_CC,
												  MYSQLND_MS_ERROR_PREFIX " Something is very wrong for master random/once.");
					DBG_RETURN(NULL);
				} else {
					DBG_INF_FMT("Using already selected master connection "MYSQLND_LLU_SPEC, connection->thread_id);
					MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_MASTER);
					SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(connection));
					DBG_RETURN(connection);
				}
				break;
			case FAILURE:
				if (zend_hash_num_elements(&filter->lb_weight)) {
					/* TODO: cleanup - master and slave code are identical,use function to avoid code duplication */
					DBG_INF("Weighted load balancing");
					use_lb_context = TRUE;
					if (FAILURE == zend_hash_find(&filter->weight_context.master_context, fprint.c, fprint.len /*\0 counted*/, (void **)&lb_context)) {
						/* build sort list for weighted load balancing */
						if (SUCCESS != mysqlnd_ms_init_sort_context(&filter->weight_context.master_context, &fprint, l, &filter->lb_weight, &sort_list, &total_weight TSRMLS_CC)) {
							break;
						}
					} else {
						zend_llist_copy(&sort_list, &lb_context->sort_list);
						total_weight = lb_context->total_weight;
					}
					DBG_INF_FMT("Sort list has %d elements, total_weight = %d", zend_llist_count(&sort_list), total_weight);
					if (0 == zend_llist_count(&sort_list)) {
						mysqlnd_ms_client_n_php_error(error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, E_WARNING TSRMLS_CC,
												  MYSQLND_MS_ERROR_PREFIX " Something is very wrong for master random/once. The sort list is empty.");
						break;
					}
				}
				while (zend_llist_count(l) > 0) {
					retry_count++;

					rnd_idx = php_rand(TSRMLS_C);
					if (use_lb_context) {
						RAND_RANGE(rnd_idx, 1, total_weight, PHP_RAND_MAX);
						DBG_INF_FMT("USE_MASTER weighted, rnd_idx=%lu", rnd_idx);
						for (i = 0, lb_weight_context_pp = (MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT **)zend_llist_get_first_ex(&sort_list, &pos);
								(lb_weight_context_pp) && (lb_weight_context = *lb_weight_context_pp);
								(lb_weight_context_pp = (MYSQLND_MS_FILTER_LB_WEIGHT_IN_CONTEXT **)zend_llist_get_next_ex(&sort_list, &pos))) {
							i += lb_weight_context->lb_weight->weight;
							if (i >= rnd_idx)
								break;
						}
						connection = (lb_weight_context && (element = lb_weight_context->element) && element->conn) ? element->conn : NULL;
					} else {
						RAND_RANGE(rnd_idx, 0, zend_llist_count(l) - 1, PHP_RAND_MAX);
						DBG_INF_FMT("USE_MASTER rnd_idx=%lu", rnd_idx);
						element_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(l, &pos);
						while (i++ < rnd_idx) {
							element_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(l, &pos);
						}
						connection = (element_pp && (element = *element_pp) && element->conn) ? element->conn : NULL;
					}

					if (connection) {
						smart_str fprint_conn = {0};

						if (stgy->failover_remember_failed) {
							zend_bool * failed;
							mysqlnd_ms_get_fingerprint_connection(&fprint_conn, &element TSRMLS_CC);
							if (SUCCESS == zend_hash_find(&stgy->failed_hosts, fprint_conn.c, fprint_conn.len /*\0 counted*/, (void **) &failed)) {
								smart_str_free(&fprint_conn);
								zend_llist_del_element(l, element, mysqlnd_ms_random_remove_conn);
								DBG_INF("Skipping previously failed connection");
								if (use_lb_context) {
									total_weight -= lb_weight_context->lb_weight->weight;
									zend_llist_del_element(&sort_list, element, mysqlnd_ms_random_sort_list_remove_conn);
								}
								continue;
							}
						}

						if (CONN_GET_STATE(connection) > CONN_ALLOCED || PASS == mysqlnd_ms_lazy_connect(element, TRUE TSRMLS_CC)) {
							MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_MASTER);
							SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(connection));
							if (TRUE == filter->sticky.once) {
								zend_hash_update(&filter->sticky.master_context, fprint.c, fprint.len /*\0 counted*/, &connection,
												sizeof(MYSQLND *), NULL);
							}
							smart_str_free(&fprint);
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

						if ((SERVER_FAILOVER_LOOP == stgy->failover_strategy) && (zend_llist_count(l) > 1) &&
							((0 == stgy->failover_max_retries) || (retry_count <= stgy->failover_max_retries))) {
							/* drop failed server from list, test remaining masters before giving up */
							DBG_INF("Trying next master");
							zend_llist_del_element(l, element, mysqlnd_ms_random_remove_conn);
							if (use_lb_context) {
								total_weight -= lb_weight_context->lb_weight->weight;
								zend_llist_del_element(&sort_list, element, mysqlnd_ms_random_sort_list_remove_conn);
							}
							continue;
						}
						smart_str_free(&fprint);
						DBG_INF("Failover disabled");
					} else {
						smart_str_free(&fprint);
						if ((SERVER_FAILOVER_LOOP == stgy->failover_strategy) && (zend_llist_count(l) > 1) &&
							((0 == stgy->failover_max_retries) || (retry_count <= stgy->failover_max_retries))) {
							/* drop failed server from list, test remaining slaves before fall-through to master */
							DBG_INF("Trying next master");
							zend_llist_del_element(l, element, mysqlnd_ms_random_remove_conn);
							if (use_lb_context) {
								total_weight -= lb_weight_context->lb_weight->weight;
								zend_llist_del_element(&sort_list, element, mysqlnd_ms_random_sort_list_remove_conn);
							}
							continue;
						}
						mysqlnd_ms_client_n_php_error(error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, E_WARNING TSRMLS_CC,
													MYSQLND_MS_ERROR_PREFIX " Couldn't find the appropriate master connection. "
													"%d masters to choose from. Something is wrong", zend_llist_count(l));
						DBG_RETURN(NULL);
					}
					DBG_RETURN(connection);
				}
				break;
		}/* switch (zend_hash_find) */
		break;
	} while (0);
	DBG_RETURN(NULL);
}
/* }}} */


/* {{{ mysqlnd_ms_choose_connection_random */
MYSQLND_CONN_DATA *
mysqlnd_ms_choose_connection_random(void * f_data, const char * const query, const size_t query_len,
									struct mysqlnd_ms_lb_strategies * stgy, MYSQLND_ERROR_INFO * error_info,
									zend_llist * master_connections, zend_llist * slave_connections,
									enum enum_which_server * which_server TSRMLS_DC)
{
	MYSQLND_MS_FILTER_RANDOM_DATA * filter = (MYSQLND_MS_FILTER_RANDOM_DATA *) f_data;
	zend_bool forced;
	enum enum_which_server tmp_which;
	zend_bool forced_tx_master = FALSE;
	DBG_ENTER("mysqlnd_ms_choose_connection_random");

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
		{
			MYSQLND_CONN_DATA * conn;
			conn = mysqlnd_ms_choose_connection_random_use_slave(master_connections, slave_connections, filter, stgy, which_server, error_info TSRMLS_CC);
			if (NULL != conn || USE_MASTER != *which_server) {
				DBG_RETURN(conn);
			}
		}
		DBG_INF("FAIL-OVER");
		/* fall-through */
		case USE_MASTER:
			DBG_RETURN(mysqlnd_ms_choose_connection_random_use_master(master_connections, filter, stgy, forced_tx_master, error_info TSRMLS_CC));
		case USE_LAST_USED:
			DBG_INF("Using last used connection");
			if (!stgy->last_used_conn) {
				mysqlnd_ms_client_n_php_error(error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, E_WARNING TSRMLS_CC,
														MYSQLND_MS_ERROR_PREFIX " Last used SQL hint cannot be used because last "
														"used connection has not been set yet. Statement will fail");
			} else {
				SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(stgy->last_used_conn));
			}
			DBG_RETURN(stgy->last_used_conn);
		default:
			/* error */
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
