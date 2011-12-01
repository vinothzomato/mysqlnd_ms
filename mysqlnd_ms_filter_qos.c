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
  +----------------------------------------------------------------------+
*/

/* $Id: mysqlnd_ms.c 311179 2011-05-18 11:26:22Z andrey $ */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_priv.h"
#ifndef mnd_emalloc
#include "ext/mysqlnd/mysqlnd_alloc.h"
#endif
#if PHP_VERSION_ID >= 50400
#include "ext/mysqlnd/mysqlnd_ext_plugin.h"
#endif

#include "mysqlnd_ms.h"
#include "mysqlnd_ms_config_json.h"
#include "mysqlnd_ms_enum_n_def.h"
#include "mysqlnd_ms_switch.h"


/* {{{ mysqlnd_ms_qos_get_gtid */
static enum_func_status mysqlnd_ms_qos_server_has_gtid(MYSQLND_CONN_DATA * conn,
		MYSQLND_MS_CONN_DATA ** conn_data, char *sql, size_t sql_len, MYSQLND_ERROR_INFO * tmp_error_info TSRMLS_DC) {
	MYSQLND_RES * res = NULL;
	enum_func_status ret = FAIL;
	/* TODO Andrey */
#if MYSQLND_VERSION_ID >= 50010
	MYSQLND_ERROR_INFO * org_error_info = NULL;
#endif

	DBG_ENTER("mysqlnd_ms_qos_server_has_gtid");

#if MYSQLND_VERSION_ID >= 50010
	/* hide errors from user */
	org_error_info = conn->error_info;
	conn->error_info = tmp_error_info;
#endif
	(*conn_data)->skip_ms_calls = TRUE;
	if ((PASS == MS_CALL_ORIGINAL_CONN_DATA_METHOD(send_query)(conn, sql, sql_len TSRMLS_CC)) &&
		(PASS ==  MS_CALL_ORIGINAL_CONN_DATA_METHOD(reap_query)(conn TSRMLS_CC)) &&
		(res = MS_CALL_ORIGINAL_CONN_DATA_METHOD(store_result)(conn TSRMLS_CC))) {
		ret = (MYSQLND_MS_UPSERT_STATUS(conn).affected_rows) ? PASS : FAIL;
	}
	(*conn_data)->skip_ms_calls = FALSE;
#if MYSQLND_VERSION_ID >= 50010
	conn->error_info = org_error_info;
#endif

	if (res) {
		res->m.free_result(res, FALSE TSRMLS_CC);
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_qos_get_lag */
static long mysqlnd_ms_qos_server_get_lag(MYSQLND_CONN_DATA * conn,
		MYSQLND_MS_CONN_DATA ** conn_data, MYSQLND_ERROR_INFO * tmp_error_info TSRMLS_DC) {
	MYSQLND_RES * res = NULL;
	long lag = -1L;
	/* TODO ANDREY - fix PHP 5.3 */
#if MYSQLND_VERSION_ID >= 50010
	MYSQLND_ERROR_INFO * org_error_info = NULL;
#endif

	DBG_ENTER("mysqlnd_ms_qos_server_get_lag");

#if MYSQLND_VERSION_ID >= 50010
	/* hide errors from user */
	org_error_info = conn->error_info;
	conn->error_info = tmp_error_info;
#endif
	(*conn_data)->skip_ms_calls = TRUE;

	if ((PASS == MS_CALL_ORIGINAL_CONN_DATA_METHOD(send_query)(conn, "SHOW SLAVE STATUS", sizeof("SHOW SLAVE STATUS") - 1 TSRMLS_CC)) &&
		(PASS ==  MS_CALL_ORIGINAL_CONN_DATA_METHOD(reap_query)(conn TSRMLS_CC)) &&
		(res = MS_CALL_ORIGINAL_CONN_DATA_METHOD(store_result)(conn TSRMLS_CC))) {

	  zval * row;
		zval ** seconds_behind_master;
		zval ** io_running;
		zval ** sql_running;

		MAKE_STD_ZVAL(row);
		mysqlnd_fetch_into(res, MYSQLND_FETCH_ASSOC, row, MYSQLND_MYSQL);
		if (Z_TYPE_P(row) == IS_ARRAY) {
			/* TODO: make test incasesensitive */
			if (FAILURE == zend_hash_find(Z_ARRVAL_P(row), "Slave_IO_Running", (uint)sizeof("Slave_IO_Running"), (void**)&io_running)) {
				SET_CLIENT_ERROR((*tmp_error_info),  CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, "Failed to extract Slave_IO_Running");
				goto getlagsqlerror;
			}

			if ((Z_TYPE_PP(io_running) != IS_STRING) ||
				(0 != strncmp(Z_STRVAL_PP(io_running), "Yes", Z_STRLEN_PP(io_running))))
			{
				SET_CLIENT_ERROR((*tmp_error_info),  CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, "Slave_IO_Running is not 'Yes'");
				goto getlagsqlerror;
			}

			if (FAILURE == zend_hash_find(Z_ARRVAL_P(row), "Slave_SQL_Running", (uint)sizeof("Slave_SQL_Running"), (void**)&sql_running))
			{
				SET_CLIENT_ERROR((*tmp_error_info),  CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, "Failed to extract Slave_SQL_Running");
				goto getlagsqlerror;
			}

			if ((Z_TYPE_PP(io_running) != IS_STRING) ||
				(0 != strncmp(Z_STRVAL_PP(sql_running), "Yes", Z_STRLEN_PP(sql_running))))
			{
				SET_CLIENT_ERROR((*tmp_error_info),  CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, "Slave_SQL_Running is not 'Yes'");
				goto getlagsqlerror;
			}

			if (FAILURE == zend_hash_find(Z_ARRVAL_P(row), "Seconds_Behind_Master", (uint)sizeof("Seconds_Behind_Master"), (void**)&seconds_behind_master))
			{
				SET_CLIENT_ERROR((*tmp_error_info),  CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, "Failed to extract Seconds_Behind_Master");
				goto getlagsqlerror;
			}

			lag = Z_LVAL_PP(seconds_behind_master);
		}

getlagsqlerror:
		zval_ptr_dtor(&row);
	}


	(*conn_data)->skip_ms_calls = FALSE;
#if MYSQLND_VERSION_ID >= 50010
	conn->error_info = org_error_info;
#endif

	if (res) {
		res->m.free_result(res, FALSE TSRMLS_CC);
	}

	DBG_RETURN(lag);
}
/* }}} */


/* {{{ mysqlnd_ms_qos_which_server */
static enum enum_which_server
mysqlnd_ms_qos_which_server(const char * query, size_t query_len, struct mysqlnd_ms_lb_strategies * stgy TSRMLS_DC)
{
	zend_bool forced;
	enum enum_which_server which_server = mysqlnd_ms_query_is_select(query, query_len, &forced TSRMLS_CC);
	DBG_ENTER("mysqlnd_ms_qos_which_server");

	if ((stgy->trx_stickiness_strategy == TRX_STICKINESS_STRATEGY_MASTER) && stgy->in_transaction && !forced) {
		DBG_INF("Enforcing use of master while in transaction");
		which_server = USE_MASTER;
	} else if (stgy->mysqlnd_ms_flag_master_on_write) {
		if (which_server != USE_MASTER) {
			if (stgy->master_used && !forced) {
				switch (which_server) {
					case USE_MASTER:
					case USE_LAST_USED:
						break;
					case USE_SLAVE:
					default:
						DBG_INF("Enforcing use of master after write");
						which_server = USE_MASTER;
						break;
				}
			}
		} else {
			DBG_INF("Use of master detected");
			stgy->master_used = TRUE;
		}
	}

	switch (which_server) {
		case USE_SLAVE:
		case USE_MASTER:
			break;
		case USE_LAST_USED:
			DBG_INF("Using last used connection");
			if (stgy->last_used_conn) {
				/*	TODO: move is_master flag from global trx struct to CONN_DATA */
			} else {
				/* TODO: handle error at this level? */
			}
			break;
		case USE_ALL:
		default:
			break;
	}

	return which_server;
}
/* }}} */


/* {{{ mysqlnd_ms_qos_pick_server */
enum_func_status mysqlnd_ms_qos_pick_server(void * f_data, const char * connect_host, const char * query, size_t query_len,
									 zend_llist * master_list, zend_llist * slave_list,
									 zend_llist * selected_masters, zend_llist * selected_slaves,
									 struct mysqlnd_ms_lb_strategies * stgy, MYSQLND_ERROR_INFO * error_info
									 TSRMLS_DC)
{
	enum_func_status ret = PASS;
	MYSQLND_MS_FILTER_QOS_DATA * filter_data = (MYSQLND_MS_FILTER_QOS_DATA *) f_data;

	DBG_ENTER("mysqlnd_ms_qos_pick_server");
	DBG_INF_FMT("query(50bytes)=%*s", MIN(50, query_len), query);

	switch (filter_data->consistency)
	{
		case CONSISTENCY_SESSION:
			/*
			For now...
				 We may be able to use selected slaves which have replicated
				 the last write on the line, e.g. using global transaction ID.

				 We may be able to use slaves which have replicated certain,
				 "tagged" writes. For example, the user could have a relaxed
				 definition of session consistency and require only consistent
				 reads from one table. In that case, we may use master and
				 all slaves which have replicated the latest updates on the
				 table in question.
			*/
			if ((QOS_OPTION_GTID == filter_data->option) &&
				(USE_MASTER != mysqlnd_ms_qos_which_server(query, query_len, stgy TSRMLS_CC)))
			{
				unsigned int i = 0;
				MYSQLND_MS_LIST_DATA * element = NULL;
				MYSQLND_CONN_DATA * connection = NULL;
				MYSQLND_MS_CONN_DATA ** conn_data = NULL;
				MYSQLND_ERROR_INFO *tmp_error_info = mnd_ecalloc(1,sizeof(MYSQLND_ERROR_INFO));
				smart_str sql = {0};
				zend_bool exit_loop = FALSE;

				BEGIN_ITERATE_OVER_SERVER_LIST(element, slave_list);
					if (element->conn && !exit_loop) {
						connection = element->conn;
						MS_LOAD_CONN_DATA(conn_data, connection);
						if (conn_data && (*conn_data) && (*conn_data)->global_trx.check_for_gtid &&
							(CONN_GET_STATE(connection) != CONN_QUIT_SENT) &&
							((CONN_GET_STATE(connection) > CONN_ALLOCED) ||	(PASS == mysqlnd_ms_lazy_connect(element, TRUE TSRMLS_CC))))
						{

							 DBG_INF_FMT("Checking slave connection "MYSQLND_LLU_SPEC"", connection->thread_id);

							if (!sql.c) {
								char * pos;
								char buf[32];
								pos = strstr((*conn_data)->global_trx.check_for_gtid, "#GTID");
								if (pos) {
								  	smart_str_appendl(&sql, (*conn_data)->global_trx.check_for_gtid, pos - ((*conn_data)->global_trx.check_for_gtid));
									snprintf(buf, sizeof(buf), "%ld", filter_data->option_data.age_or_gtid);
									smart_str_appends(&sql, buf);
									smart_str_appendc(&sql, '\0');
								} else {
									char error_buf[512];
									snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Failed parse SQL for checking GTID. Cannot find #GTID placeholder");
									error_buf[sizeof(error_buf) - 1] = '\0';
									php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_buf);
									exit_loop = TRUE;
								}
							}
							if (sql.c) {
								tmp_error_info->error_no = 0;
								if (PASS == mysqlnd_ms_qos_server_has_gtid(connection, conn_data, sql.c, sql.len - 1, tmp_error_info TSRMLS_CC)) {
									zend_llist_add_element(selected_slaves, &element);
								} else if (tmp_error_info->error_no) {
									char error_buf[512];
									snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " SQL error while checking slave for GTID: %d/'%s'", tmp_error_info->error_no, tmp_error_info->error);
									error_buf[sizeof(error_buf) - 1] = '\0';
									php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_buf);
								}
							}
						}
					}
					i++;
				END_ITERATE_OVER_SERVER_LIST;

				if (sql.c) {
					smart_str_free(&sql);
				}
				mnd_efree(tmp_error_info);

				zend_llist_copy(selected_masters, master_list);
				break;
			}

		case CONSISTENCY_STRONG:
			/*
			For now and forever...
				... use masters, no slaves.

				This is our master_on_write replacement. All the other filters
				don't need to take care in the future.
			*/
			DBG_INF("using masters only for strong consistency");
			zend_llist_copy(selected_masters, master_list);
			break;

		case CONSISTENCY_EVENTUAL:
			/*
			For now...
				Either all masters and slaves or
				slaves filtered by SHOW SLAVE STATUS replication lag

			For later...

				We may inject mysqlnd_qc per-query TTL SQL hints here to
				replace a slave access with a call access.
			*/
			if ((QOS_OPTION_AGE == filter_data->option) &&
				(USE_MASTER != mysqlnd_ms_qos_which_server(query, query_len, stgy TSRMLS_CC)))
			{
				unsigned int i = 0;
				MYSQLND_MS_LIST_DATA * element = NULL;
				MYSQLND_CONN_DATA * connection = NULL;
				MYSQLND_MS_CONN_DATA ** conn_data = NULL;
				MYSQLND_ERROR_INFO *tmp_error_info = mnd_ecalloc(1,sizeof(MYSQLND_ERROR_INFO));
				zend_bool exit_loop = FALSE;
				long lag;

				BEGIN_ITERATE_OVER_SERVER_LIST(element, slave_list);
					if (element->conn && !exit_loop) {
						connection = element->conn;
						MS_LOAD_CONN_DATA(conn_data, connection);
						if (conn_data && (*conn_data) &&
							(CONN_GET_STATE(connection) != CONN_QUIT_SENT) &&
							((CONN_GET_STATE(connection) > CONN_ALLOCED) ||	(PASS == mysqlnd_ms_lazy_connect(element, TRUE TSRMLS_CC))))
						{

								DBG_INF_FMT("Checking slave connection "MYSQLND_LLU_SPEC"", connection->thread_id);
								tmp_error_info->error_no = 0;
								lag = mysqlnd_ms_qos_server_get_lag(connection, conn_data, tmp_error_info TSRMLS_CC);
								if ((lag > 0) && (lag <= filter_data->option_data.age_or_gtid)) {
									zend_llist_add_element(selected_slaves, &element);
								} else if (tmp_error_info->error_no) {
									char error_buf[512];
									snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " SQL error while checking slave for GTID: %d/'%s'", tmp_error_info->error_no, tmp_error_info->error);
									error_buf[sizeof(error_buf) - 1] = '\0';
									php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_buf);
								}
						}
					}
					i++;
				END_ITERATE_OVER_SERVER_LIST;

				mnd_efree(tmp_error_info);

			} else {
				zend_llist_copy(selected_slaves, slave_list);
			}
			zend_llist_copy(selected_masters, master_list);
			break;

		default:
			DBG_ERR("Invalid filter data, we should never get here");
			ret = FAIL;
			break;

	}

	DBG_RETURN(ret);
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
