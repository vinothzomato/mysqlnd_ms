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


/* {{{ mysqlnd_ms_select_servers_random_once */
enum_func_status
mysqlnd_ms_select_servers_random_once(enum php_mysqlnd_server_command command,
									  struct mysqlnd_ms_lb_strategies * stgy,
									  zend_llist * master_list, zend_llist * slave_list,
									  zend_llist * selected_masters, zend_llist * selected_slaves TSRMLS_DC)
{
	enum_func_status ret = FAIL;
	zend_llist_position	pos;
	MYSQLND_MS_LIST_DATA * el;
	DBG_ENTER("mysqlnd_ms_select_servers_random_once");
	DBG_INF_FMT("random_once_slave=%p", stgy->random_once_slave);
	if (!stgy->random_once_slave) {
		ret = mysqlnd_ms_select_servers_all(command, stgy, master_list, slave_list, selected_masters, selected_slaves TSRMLS_CC);
		DBG_RETURN(ret);
	}

	/* search the list of easy handles hanging off the multi-handle */
	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(master_list, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(master_list, &pos))
	{
		/*
		  This will copy the whole structure, not the pointer.
		  This is wanted!!
		*/
		zend_llist_add_element(selected_masters, el);
	}

	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(slave_list, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(slave_list, &pos))
	{
		DBG_INF_FMT("[slave] el->conn=%p", el->conn);
		if (el->conn == stgy->random_once_slave) {
			/*
			  This will copy the whole structure, not the pointer.
			  This is wanted!!
			*/
			zend_llist_add_element(selected_slaves, el);
			ret = PASS;
			DBG_INF("bingo!");
			break;
		}
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_choose_connection_rr */
static MYSQLND *
mysqlnd_ms_choose_connection_rr(const char * const query, const size_t query_len,
								struct mysqlnd_ms_lb_strategies * stgy,
								zend_llist * master_connections, zend_llist * slave_connections TSRMLS_DC)
{
	zend_bool forced;
	enum enum_which_server which_server = mysqlnd_ms_query_is_select(query, query_len, &forced TSRMLS_CC);
	DBG_ENTER("mysqlnd_ms_choose_connection_rr");

	if ((stgy->trx_stickiness_strategy == TRX_STICKINESS_STRATEGY_MASTER) && stgy->in_transaction && !forced) {
		DBG_INF("Enforcing use of master while in transaction");
		which_server = USE_MASTER;
		MYSQLND_MS_INC_STATISTIC(MS_STAT_TRX_MASTER_FORCED);
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
		{
			zend_llist * l = slave_connections;
			MYSQLND_MS_LIST_DATA * element = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next(l);
			MYSQLND * connection = (element && element->conn)? element->conn : (((element = zend_llist_get_first(l)) && element->conn)? element->conn : NULL);
			if (connection) {
				DBG_INF_FMT("Using slave connection "MYSQLND_LLU_SPEC"", connection->thread_id);

				if (CONN_GET_STATE(connection) == CONN_ALLOCED) {
					DBG_INF("Lazy connection, trying to connect...");
					/* lazy connection, connect now */
					if (PASS == ms_orig_mysqlnd_conn_methods->connect(connection, element->host, element->user,
																   element->passwd, element->passwd_len,
																   element->db, element->db_len,
																   element->port, element->socket, element->connect_flags TSRMLS_CC))
					{
						DBG_INF("Connected");
						MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_SUCCESS);
						DBG_RETURN(connection);
					}
					MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_FAILURE);

					if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
					  DBG_INF("Failover disabled");
					  DBG_RETURN(connection);
					}
					DBG_INF("Connect failed, falling back to the master");
				} else {
					DBG_RETURN(element->conn);
				}
			} else {
				if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
					DBG_INF("Failover disabled");
					DBG_RETURN(connection);
				}
			}
		}
		/* fall-through */
		case USE_MASTER:
		{
			zend_llist * l = master_connections;
			MYSQLND_MS_LIST_DATA * element = zend_llist_get_next(l);
			MYSQLND * connection = (element && element->conn)? element->conn : (((element = zend_llist_get_first(l)) && element->conn)? element->conn : NULL);
			DBG_INF("Using master connection");
			if (connection) {
				if (CONN_GET_STATE(connection) == CONN_ALLOCED) {
					DBG_INF("Lazy connection, trying to connect...");
					/* lazy connection, connect now */
					if (PASS == ms_orig_mysqlnd_conn_methods->connect(connection, element->host, element->user,
																   element->passwd, element->passwd_len,
																   element->db, element->db_len,
																   element->port, element->socket, element->connect_flags TSRMLS_CC))
					{
						DBG_INF("Connected");
						MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_MASTER_SUCCESS);
						DBG_RETURN(connection);
					}
					MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_MASTER_FAILURE);
				}
			}
			DBG_RETURN(connection);
			break;
		}

		case USE_LAST_USED:
			DBG_INF("Using last used connection");
			DBG_RETURN(stgy->last_used_conn);
		default:
			/* error */
			DBG_RETURN(NULL);
			break;
	}
}
/* }}} */


/* {{{ mysqlnd_ms_choose_connection_random_once */
static MYSQLND *
mysqlnd_ms_choose_connection_random_once(const char * const query, const size_t query_len,
										 struct mysqlnd_ms_lb_strategies * stgy,
										 zend_llist * master_connections, zend_llist * slave_connections TSRMLS_DC)
{
	zend_bool forced;
	enum enum_which_server which_server = mysqlnd_ms_query_is_select(query, query_len, &forced TSRMLS_CC);
	DBG_ENTER("mysqlnd_ms_choose_connection_random_once");

	if ((stgy->trx_stickiness_strategy == TRX_STICKINESS_STRATEGY_MASTER) && stgy->in_transaction && !forced) {
		DBG_INF("Enforcing use of master while in transaction");
		which_server = USE_MASTER;
		MYSQLND_MS_INC_STATISTIC(MS_STAT_TRX_MASTER_FORCED);
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
			if (stgy->random_once_slave) {
				DBG_INF_FMT("Using last random connection "MYSQLND_LLU_SPEC, stgy->random_once_slave->thread_id);
				DBG_RETURN(stgy->random_once_slave);
			} else {
				zend_llist_position	pos;
				zend_llist * l = slave_connections;
				MYSQLND_MS_LIST_DATA * element;
				unsigned long rnd_idx;
				uint i = 0;
				MYSQLND * connection = NULL;

				rnd_idx = php_rand(TSRMLS_C);
				RAND_RANGE(rnd_idx, 0, zend_llist_count(l) - 1, PHP_RAND_MAX);
				DBG_INF_FMT("USE_SLAVE rnd_idx=%lu", rnd_idx);

				element = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(l, &pos);
				while (i++ < rnd_idx) {
					element = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(l, &pos);
				}
				connection = (element && element->conn)? element->conn : NULL;
				if (connection) {
					DBG_INF_FMT("Using slave connection "MYSQLND_LLU_SPEC"", connection->thread_id);

					if (CONN_GET_STATE(connection) == CONN_ALLOCED) {
						DBG_INF("Lazy connection, trying to connect...");
						/* lazy connection, connect now */
						if (PASS == ms_orig_mysqlnd_conn_methods->connect(connection, element->host, element->user,
																	   element->passwd, element->passwd_len,
																	   element->db, element->db_len,
																	   element->port, element->socket,
																	   element->connect_flags TSRMLS_CC))
						{
							DBG_INF("Connected");
							stgy->random_once_slave = connection;
							MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_SUCCESS);
							DBG_RETURN(connection);
						}
						MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_FAILURE);
						if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
						  DBG_INF("Failover disabled");
						  DBG_RETURN(connection);
						}
						DBG_INF("Connect failed, falling back to the master");
					} else {
						stgy->random_once_slave = connection;
						DBG_RETURN(connection);
					}
				} else {
					if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
						DBG_INF("Failover disabled");
						DBG_RETURN(connection);
					}
				}
			}
		/* fall-through */
		case USE_MASTER:
		{
			zend_llist_position	pos;
			zend_llist * l = master_connections;
			MYSQLND_MS_LIST_DATA * element;
			unsigned long rnd_idx;
			uint i = 0;
			MYSQLND * connection = NULL;

			rnd_idx = php_rand(TSRMLS_C);
			RAND_RANGE(rnd_idx, 0, zend_llist_count(l) - 1, PHP_RAND_MAX);
			DBG_INF_FMT("USE_MASTER rnd_idx=%lu", rnd_idx);

			element = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(l, &pos);
			while (i++ < rnd_idx) {
				element = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(l, &pos);
			}

			connection = (element && element->conn)? element->conn : NULL;
			DBG_INF("Using master connection");
			if (connection) {
				if (CONN_GET_STATE(connection) == CONN_ALLOCED) {
					DBG_INF("Lazy connection, trying to connect...");
					/* lazy connection, connect now */
					if (PASS == ms_orig_mysqlnd_conn_methods->connect(connection, element->host, element->user,
																   element->passwd, element->passwd_len,
																   element->db, element->db_len,
																   element->port, element->socket,
																   element->connect_flags TSRMLS_CC))
					{
						DBG_INF("Connected");
						DBG_INF_FMT("Using master connection "MYSQLND_LLU_SPEC"", connection->thread_id);
						MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_MASTER_SUCCESS);
						DBG_RETURN(connection);
					}
					MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_MASTER_FAILURE);
				}
			}
			DBG_RETURN(connection);
			break;
		}
		case USE_LAST_USED:
			DBG_INF("Using last used connection");
			DBG_RETURN(stgy->last_used_conn);
		default:
			/* error */
			DBG_RETURN(NULL);
			break;
	}
}
/* }}} */


/* {{{ mysqlnd_ms_choose_connection_partition_by_filter */
static enum_func_status
mysqlnd_ms_choose_connection_partition_by_filter(MYSQLND * conn, const char * query, unsigned int query_len,
												 struct mysqlnd_ms_lb_strategies * stgy,
												 zend_llist ** master_list, zend_llist ** slave_list TSRMLS_DC)
{
	struct st_mysqlnd_query_parser * parser;
	DBG_ENTER("mysqlnd_ms_choose_connection_partition_by_filter");
	parser = mysqlnd_qp_create_parser(TSRMLS_C);
	if (parser) {
		int ret = mysqlnd_qp_start_parser(parser, query, query_len TSRMLS_CC);
		DBG_INF_FMT("mysqlnd_qp_start_parser=%d", ret);
		DBG_INF_FMT("db=%s table=%s org_table=%s statement_type=%d",
				parser->parse_info.db? parser->parse_info.db:"n/a",
				parser->parse_info.table? parser->parse_info.table:"n/a",
				parser->parse_info.org_table? parser->parse_info.org_table:"n/a",
				parser->parse_info.statement
			);

		zend_llist_init(*master_list, sizeof(MYSQLND_MS_LIST_DATA), (llist_dtor_func_t) mysqlnd_ms_conn_list_dtor, conn->persistent);
		zend_llist_init(*slave_list, sizeof(MYSQLND_MS_LIST_DATA), (llist_dtor_func_t) mysqlnd_ms_conn_list_dtor, conn->persistent);
#if 0
		{
			MYSQLND_MS_LIST_DATA new_element = {0};
			new_element.conn = conn;
			new_element.host = host? mnd_pestrdup(host, persistent) : NULL;
			new_element.persistent = persistent;
			new_element.port = port;
			new_element.socket = socket? mnd_pestrdup(socket, conn->persistent) : NULL;
			new_element.emulated_scheme_len = mysqlnd_ms_get_scheme_from_list_data(&new_element, &new_element.emulated_scheme,
																			   persistent TSRMLS_CC);
			zend_llist_add_element(*master_list, &new_element);
		
		}
#endif
		mysqlnd_qp_free_parser(parser TSRMLS_CC);
		DBG_RETURN(PASS);
	}
	DBG_RETURN(FAIL);
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
