/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2008 The PHP Group                                |
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


/* {{{ rr_filter_dtor */
static void
rr_filter_dtor(struct st_mysqlnd_ms_filter_data * pDest TSRMLS_DC)
{
	MYSQLND_MS_FILTER_RR_DATA * filter = (MYSQLND_MS_FILTER_RR_DATA *) pDest;
	DBG_ENTER("rr_filter_dtor");

	zend_hash_destroy(&filter->master_context);
	zend_hash_destroy(&filter->slave_context);
	mnd_pefree(filter, filter->parent.persistent);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_rr_filter_ctor */
MYSQLND_MS_FILTER_DATA *
mysqlnd_ms_rr_filter_ctor(struct st_mysqlnd_ms_config_json_entry * section, MYSQLND_ERROR_INFO * error_info, zend_bool persistent TSRMLS_DC)
{
	MYSQLND_MS_FILTER_RR_DATA * ret;
	DBG_ENTER("mysqlnd_ms_rr_filter_ctor");
	DBG_INF_FMT("section=%p", section);
	/* section could be NULL! */
	ret = mnd_pecalloc(1, sizeof(MYSQLND_MS_FILTER_RR_DATA), persistent);
	if (ret) {
		ret->parent.filter_dtor = rr_filter_dtor;
		zend_hash_init(&ret->master_context, 4, NULL/*hash*/, NULL/*dtor*/, persistent);
		zend_hash_init(&ret->slave_context, 4, NULL/*hash*/, NULL/*dtor*/, persistent);
	}
	DBG_RETURN((MYSQLND_MS_FILTER_DATA *) ret);
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
		{
			unsigned int tmp_pos = 0;
			unsigned int * pos;
			zend_llist * l = slave_connections;
			if (0 == zend_llist_count(l) && SERVER_FAILOVER_MASTER == stgy->failover_strategy) {
				goto use_master;
			}
			/* LOCK on context ??? */
			{
				smart_str fprint = {0};
				mysqlnd_ms_get_fingerprint(&fprint, l TSRMLS_CC);
				DBG_INF_FMT("fingerprint=%*s", fprint.len, fprint.c);
				if (SUCCESS != zend_hash_find(&filter->slave_context, fprint.c, fprint.len /*\0 counted*/, (void **) &pos)) {
					int retval;
					DBG_INF("Init the slave context");
					retval = zend_hash_add(&filter->slave_context, fprint.c, fprint.len /*\0 counted*/, &tmp_pos, sizeof(uint), NULL);
					/* fetch ptr to the data inside the HT */
					if (SUCCESS == retval) {
						zend_hash_find(&filter->slave_context, fprint.c, fprint.len /*\0 counted*/, (void **) &pos);
					}
					smart_str_free(&fprint);
					if (SUCCESS != retval) {
						break;
					}
				} else {
					smart_str_free(&fprint);
				}
				DBG_INF_FMT("look under pos %u", *pos);
			}
			{
				unsigned int i = 0;
				MYSQLND_MS_LIST_DATA * element = NULL;
				MYSQLND_CONN_DATA * connection = NULL;

				BEGIN_ITERATE_OVER_SERVER_LIST(element, l);
					if (i == *pos) {
						break;
					}
					i++;
				END_ITERATE_OVER_SERVER_LIST;
				DBG_INF_FMT("i=%u pos=%u", i, *pos);
				if (!element) {
					char error_buf[256];
					snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Couldn't find the appropriate slave connection. %d slaves to choose from. Something is wrong", zend_llist_count(l));
					error_buf[sizeof(error_buf) - 1] = '\0';
					SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
					DBG_ERR_FMT("%s", error_buf);
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_buf);
					break;
				}
				/* time to increment the position */
				*pos = ((*pos) + 1) % zend_llist_count(l);
				DBG_INF_FMT("pos is now %u", *pos);
				if (element->conn) {
					connection = element->conn;
				}
				if (connection) {
					DBG_INF_FMT("Using slave connection "MYSQLND_LLU_SPEC"", connection->thread_id);

					if (CONN_GET_STATE(connection) > CONN_ALLOCED || PASS == mysqlnd_ms_lazy_connect(element, FALSE TSRMLS_CC)) {
						MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_SLAVE);
						SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(connection));
						DBG_RETURN(connection);
					} else if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
						DBG_INF("Failover disabled");
						DBG_RETURN(connection);
					}
					DBG_INF("Falling back to the master");
				} else {
					if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
						DBG_INF("Failover disabled");
						DBG_RETURN(NULL);
					}
				}
			}
			/* UNLOCK of context ??? */
		}
use_master:
		DBG_INF("FAIL-OVER");
		/* fall-through */
		case USE_MASTER:
		{
			unsigned int tmp_pos = 0;
			zend_llist * l = master_connections;
			unsigned int * pos;
			/* LOCK on context ??? */
			{
				smart_str fprint = {0};
				mysqlnd_ms_get_fingerprint(&fprint, l TSRMLS_CC);
				if (SUCCESS != zend_hash_find(&filter->master_context, fprint.c, fprint.len /*\0 counted*/, (void **) &pos)) {
					int retval;
					DBG_INF("Init the master context");
					retval = zend_hash_add(&filter->master_context, fprint.c, fprint.len /*\0 counted*/, &tmp_pos, sizeof(uint), NULL);
					/* fetch ptr to the data inside the HT */
					if (SUCCESS == retval) {
						zend_hash_find(&filter->master_context, fprint.c, fprint.len /*\0 counted*/, (void **) &pos);
					}
					smart_str_free(&fprint);
					if (SUCCESS != retval) {
						break;
					}
				} else {
					smart_str_free(&fprint);
				}
			}
			{
				unsigned int i = 0;
				MYSQLND_MS_LIST_DATA * element = NULL;
				MYSQLND_CONN_DATA * connection = NULL;

				BEGIN_ITERATE_OVER_SERVER_LIST(element, l);
					if (i == *pos) {
						break;
					}
					i++;
				END_ITERATE_OVER_SERVER_LIST;

				if (!element) {
					char error_buf[256];
					snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Couldn't find the appropriate master connection. %d masters to choose from. Something is wrong", zend_llist_count(l));
					error_buf[sizeof(error_buf) - 1] = '\0';
					SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
					DBG_ERR_FMT("%s", error_buf);
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_buf);
					break;
				}
				if (FALSE == forced_tx_master) {
					/* time to increment the position */
					*pos = ((*pos) + 1) % zend_llist_count(l);
				}
				if (element->conn) {
					connection = element->conn;
				}
				DBG_INF("Using master connection");
				if (connection &&
					(CONN_GET_STATE(connection) > CONN_ALLOCED || PASS == mysqlnd_ms_lazy_connect(element, TRUE TSRMLS_CC)))
				{
					MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_MASTER);
					SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(connection));
				}
				DBG_RETURN(connection);
			}
			break;
		}
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
