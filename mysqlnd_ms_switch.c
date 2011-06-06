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
#include "ext/standard/info.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_priv.h"
#ifndef mnd_emalloc
#include "ext/mysqlnd/mysqlnd_alloc.h"
#endif
#ifndef mnd_sprintf
#define mnd_sprintf spprintf
#endif
#include "mysqlnd_ms.h"
#include "mysqlnd_ms_config_json.h"
#include "ext/standard/php_rand.h"

#include "mysqlnd_query_parser.h"
#include "mysqlnd_qp.h"

#include "mysqlnd_ms_enum_n_def.h"
#include "mysqlnd_ms_filter_user.h"
#include "mysqlnd_ms_filter_random.h"
#include "mysqlnd_ms_filter_random_once.h"
#include "mysqlnd_ms_filter_round_robin.h"
#include "mysqlnd_ms_switch.h"


/* {{{ mysqlnd_ms_lb_strategy_setup */
void
mysqlnd_ms_lb_strategy_setup(struct mysqlnd_ms_lb_strategies * strategies,
							 struct st_mysqlnd_ms_config_json_entry * the_section,
							 MYSQLND_ERROR_INFO * error_info TSRMLS_DC)
{
	zend_bool value_exists = FALSE, is_list_value = FALSE;

	DBG_ENTER("mysqlnd_ms_lb_strategy_setup");
	{
		char * pick_strategy = mysqlnd_ms_config_json_string_from_section(the_section, PICK_NAME, sizeof(PICK_NAME) - 1,
																		  &value_exists, &is_list_value TSRMLS_CC);

		strategies->pick_strategy = strategies->fallback_pick_strategy = DEFAULT_PICK_STRATEGY;
		strategies->select_servers = DEFAULT_SELECT_SERVERS;
		if (value_exists && pick_strategy) {
			/* random is a substing of random_once thus we check first for random_once */
			if (!strncasecmp(PICK_RROBIN, pick_strategy, sizeof(PICK_RROBIN) - 1)) {
				strategies->pick_strategy = strategies->fallback_pick_strategy = SERVER_PICK_RROBIN;
				strategies->select_servers = mysqlnd_ms_select_servers_all;
			} else if (!strncasecmp(PICK_RANDOM_ONCE, pick_strategy, sizeof(PICK_RANDOM_ONCE) - 1)) {
				strategies->pick_strategy = strategies->fallback_pick_strategy = SERVER_PICK_RANDOM_ONCE;
				strategies->select_servers = mysqlnd_ms_select_servers_random_once;
			} else if (!strncasecmp(PICK_RANDOM, pick_strategy, sizeof(PICK_RANDOM) - 1)) {
				strategies->pick_strategy = strategies->fallback_pick_strategy = SERVER_PICK_RANDOM;
				strategies->select_servers = mysqlnd_ms_select_servers_all;
			} else if (!strncasecmp(PICK_USER, pick_strategy, sizeof(PICK_USER) - 1)) {
				strategies->pick_strategy = SERVER_PICK_USER;
				/* XXX: HANDLE strategies->select_servers !!! */
				if (is_list_value) {
					mnd_efree(pick_strategy);
					pick_strategy =
						mysqlnd_ms_config_json_string_from_section(the_section, PICK_NAME, sizeof(PICK_NAME) - 1,
																   &value_exists, &is_list_value TSRMLS_CC);
					if (pick_strategy) {
						if (!strncasecmp(PICK_RANDOM_ONCE, pick_strategy, sizeof(PICK_RANDOM_ONCE) - 1)) {
							strategies->fallback_pick_strategy = SERVER_PICK_RANDOM_ONCE;
						} else if (!strncasecmp(PICK_RANDOM, pick_strategy, sizeof(PICK_RANDOM) - 1)) {
							strategies->fallback_pick_strategy = SERVER_PICK_RANDOM;
						} else if (!strncasecmp(PICK_RROBIN, pick_strategy, sizeof(PICK_RROBIN) - 1)) {
							strategies->fallback_pick_strategy = SERVER_PICK_RROBIN;
						}
					}
				}
			} else {
				char error_buf[128];
				snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX "Unknown pick strategy %s", pick_strategy);
				error_buf[sizeof(error_buf) - 1] = '\0';

				php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_buf);
				SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
			}
		}
		if (pick_strategy) {
			mnd_efree(pick_strategy);
		}
	}

	{
		char * failover_strategy =
			mysqlnd_ms_config_json_string_from_section(the_section, FAILOVER_NAME, sizeof(FAILOVER_NAME) - 1,
													   &value_exists, &is_list_value TSRMLS_CC);

		strategies->failover_strategy = DEFAULT_FAILOVER_STRATEGY;

		if (value_exists && failover_strategy) {
			if (!strncasecmp(FAILOVER_DISABLED, failover_strategy, sizeof(FAILOVER_DISABLED) - 1)) {
				strategies->failover_strategy = SERVER_FAILOVER_DISABLED;
			} else if (!strncasecmp(FAILOVER_MASTER, failover_strategy, sizeof(FAILOVER_MASTER) - 1)) {
				strategies->failover_strategy = SERVER_FAILOVER_MASTER;
			}
			mnd_efree(failover_strategy);
		}
	}

	{
		char * master_on_write =
			mysqlnd_ms_config_json_string_from_section(the_section, MASTER_ON_WRITE_NAME, sizeof(MASTER_ON_WRITE_NAME) - 1,
													   &value_exists, &is_list_value TSRMLS_CC);

		strategies->mysqlnd_ms_flag_master_on_write = FALSE;
		strategies->master_used = FALSE;

		if (value_exists && master_on_write) {
			DBG_INF("Master on write active");
			strategies->mysqlnd_ms_flag_master_on_write = !mysqlnd_ms_config_json_string_is_bool_false(master_on_write);
			mnd_efree(master_on_write);
		}
	}

	{
		char * trx_strategy =
			mysqlnd_ms_config_json_string_from_section(the_section, TRX_STICKINESS_NAME, sizeof(TRX_STICKINESS_NAME) - 1,
													   &value_exists, &is_list_value TSRMLS_CC);

		strategies->trx_stickiness_strategy = DEFAULT_TRX_STICKINESS_STRATEGY;
		strategies->in_transaction = FALSE;

		if (value_exists && trx_strategy) {
#if PHP_VERSION_ID >= 50399
			if (!strncasecmp(TRX_STICKINESS_MASTER, trx_strategy, sizeof(TRX_STICKINESS_MASTER) - 1)) {
				DBG_INF("Transaction strickiness strategy = master");
				strategies->trx_stickiness_strategy = TRX_STICKINESS_STRATEGY_MASTER;
			}
#else
			php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " trx_stickiness_strategy is not supported before PHP 5.3.99");
#endif
			mnd_efree(trx_strategy);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_query_is_select */
PHPAPI enum enum_which_server
mysqlnd_ms_query_is_select(const char * query, size_t query_len, zend_bool * forced TSRMLS_DC)
{
	enum enum_which_server ret = USE_MASTER;
	struct st_qc_token_and_value token = {0};
	struct st_mysqlnd_query_scanner * scanner;
	DBG_ENTER("mysqlnd_ms_query_is_select");
	*forced = FALSE;
	if (!query) {
		DBG_RETURN(USE_MASTER);
	}

	scanner = mysqlnd_qp_create_scanner(TSRMLS_C);
	mysqlnd_qp_set_string(scanner, query, query_len TSRMLS_CC);
	token = mysqlnd_qp_get_token(scanner TSRMLS_CC);
	DBG_INF_FMT("token=COMMENT? = %d", token.token == QC_TOKEN_COMMENT);
	while (token.token == QC_TOKEN_COMMENT) {
		if (!strncasecmp(Z_STRVAL(token.value), MASTER_SWITCH, sizeof(MASTER_SWITCH) - 1)) {
			DBG_INF("forced master");
			ret = USE_MASTER;
			*forced = TRUE;
			MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_MASTER_FORCED);
		} else if (!strncasecmp(Z_STRVAL(token.value), SLAVE_SWITCH, sizeof(SLAVE_SWITCH) - 1)) {
			DBG_INF("forced slave");
			ret = USE_SLAVE;
			*forced = TRUE;
			MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_SLAVE_FORCED);
		} else if (!strncasecmp(Z_STRVAL(token.value), LAST_USED_SWITCH, sizeof(LAST_USED_SWITCH) - 1)) {
			DBG_INF("forced last used");
			ret = USE_LAST_USED;
			*forced = TRUE;
			MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_LAST_USED_FORCED);
#ifdef ALL_SERVER_DISPATCH
		} else if (!strncasecmp(Z_STRVAL(token.value), ALL_SERVER_SWITCH, sizeof(ALL_SERVER_SWITCH) - 1)) {
			DBG_INF("forced all server");
			ret = USE_ALL;
			*forced = TRUE;
#endif
		}
		zval_dtor(&token.value);
		token = mysqlnd_qp_get_token(scanner TSRMLS_CC);
	}
	if (*forced == FALSE) {
		if (token.token == QC_TOKEN_SELECT) {
			ret = USE_SLAVE;
			MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_SLAVE);
#ifdef ALL_SERVER_DISPATCH
		} else if (token.token == QC_TOKEN_SET) {
			ret = USE_ALL;
#endif
		} else {
			MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_MASTER);
		}
	}
	zval_dtor(&token.value);
	mysqlnd_qp_free_scanner(scanner TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_select_servers_all */
enum_func_status
mysqlnd_ms_select_servers_all(enum php_mysqlnd_server_command command,
							  struct mysqlnd_ms_lb_strategies * stgy,
							  zend_llist * master_list, zend_llist * slave_list,
							  zend_llist * selected_masters, zend_llist * selected_slaves TSRMLS_DC)
{
	enum_func_status ret = PASS;
	DBG_ENTER("mysqlnd_ms_select_servers_all");

	{
		zend_llist_position	pos;
		MYSQLND_MS_LIST_DATA * el;
		if (zend_llist_count(master_list)) {
			for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(master_list, &pos); el && el->conn;
					el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(master_list, &pos))
			{
				/*
				  This will copy the whole structure, not the pointer.
				  This is wanted!!
				*/
				zend_llist_add_element(selected_masters, el);
			}
		}
		if (zend_llist_count(slave_list)) {
			for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(slave_list, &pos); el && el->conn;
					el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(slave_list, &pos))
			{
				/*
				  This will copy the whole structure, not the pointer.
				  This is wanted!!
				*/
				zend_llist_add_element(selected_slaves, el);
			}
		}
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_pick_server */
MYSQLND *
mysqlnd_ms_pick_server(MYSQLND * conn, const char * const query, const size_t query_len TSRMLS_DC)
{
	MYSQLND_MS_CONN_DATA ** conn_data = (MYSQLND_MS_CONN_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	MYSQLND * connection = conn;
	DBG_ENTER("mysqlnd_ms_pick_server");

	if (conn_data && *conn_data) {
		struct mysqlnd_ms_lb_strategies * stgy = &(*conn_data)->stgy;
		enum mysqlnd_ms_server_pick_strategy pick_strategy = stgy->pick_strategy;
		zend_llist user_selected_masters, user_selected_slaves;
		zend_llist * master_list = &(*conn_data)->master_connections;
		zend_llist * slave_list = &(*conn_data)->slave_connections;

		connection = NULL;
		if (SERVER_PICK_USER == pick_strategy) {
			if (FALSE == MYSQLND_MS_G(pick_server_is_multiple)) {
				connection = mysqlnd_ms_user_pick_server(conn, query, query_len, master_list, slave_list TSRMLS_CC);	
				if (!connection) {
					pick_strategy = (*conn_data)->stgy.fallback_pick_strategy;
				}
			} else {
				/*
				  No dtor for these lists, because mysqlnd_ms_user_pick_multiple_server()
				  does shallow (byte) copy and doesn't call any copy_ctor whatsoever on the data
				  it copies.
				*/
				zend_llist_init(&user_selected_masters, sizeof(MYSQLND_MS_LIST_DATA), NULL /*dtor*/, conn->persistent);
				zend_llist_init(&user_selected_slaves, sizeof(MYSQLND_MS_LIST_DATA), NULL /*dtor*/, conn->persistent);

				pick_strategy = (*conn_data)->stgy.fallback_pick_strategy;
				if (PASS == mysqlnd_ms_user_pick_multiple_server(conn, query, query_len, master_list, slave_list,
																 &user_selected_masters, &user_selected_slaves TSRMLS_CC))
				{
					master_list = &user_selected_masters;
					slave_list = &user_selected_slaves;
				}
			}
		}
		if (!connection) {
			DBG_INF_FMT("count(master_list)=%d", zend_llist_count(master_list));
			DBG_INF_FMT("count(slave_list)=%d", zend_llist_count(slave_list));

			if (SERVER_PICK_RANDOM == pick_strategy) {
				connection = mysqlnd_ms_choose_connection_random(query, query_len, stgy, master_list, slave_list TSRMLS_CC);
			} else if (SERVER_PICK_RANDOM_ONCE == pick_strategy) {
				connection = mysqlnd_ms_choose_connection_random_once(query, query_len, stgy, master_list, slave_list TSRMLS_CC);
			} else {
				connection = mysqlnd_ms_choose_connection_rr(query, query_len, stgy, master_list, slave_list TSRMLS_CC);
			}
		}
		/* cleanup if we used multiple pick server */
		if (&user_selected_masters == master_list) {
			zend_llist_destroy(&user_selected_masters);
		}
		if (&user_selected_slaves == slave_list) {
			zend_llist_destroy(&user_selected_slaves);
		}
		if (!connection) {
			connection = conn;
		}
		stgy->last_used_conn = connection;
	}

	DBG_RETURN(connection);
}
/* }}} */


#ifdef ALL_SERVER_DISPATCH
/* {{{ mysqlnd_ms_query_all */
static enum_func_status
mysqlnd_ms_query_all(MYSQLND * const proxy_conn, const char * query, unsigned int query_len,
					 zend_llist * master_connections, zend_llist * slave_connections TSRMLS_DC)
{
	enum_func_status ret = PASS;
	MYSQLND_MS_CONN_DATA ** conn_data = (MYSQLND_MS_CONN_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);

	DBG_ENTER("mysqlnd_ms_query_all");
	if (!conn_data || !*conn_data) {
		DBG_RETURN(ms_orig_mysqlnd_conn_methods->query(proxy_conn, query, query_len TSRMLS_CC));
	} else {
		zend_llist * lists[] = {NULL, &(*conn_data)->master_connections, &(*conn_data)->slave_connections, NULL};
		zend_llist ** list = lists;
		while (*++list) {
			zend_llist_position	pos;
			MYSQLND_MS_LIST_DATA * el;
			/* search the list of easy handles hanging off the multi-handle */
			for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(*list, &pos); el && el->conn;
					el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(*list, &pos))
			{
				if (PASS != ms_orig_mysqlnd_conn_methods->query(el->conn, query, query_len TSRMLS_CC)) {
					ret = FAIL;
				}
			}
		}
	}

	DBG_RETURN(ret);
}
/* }}} */
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
