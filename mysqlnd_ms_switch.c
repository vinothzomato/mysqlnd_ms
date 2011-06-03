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
