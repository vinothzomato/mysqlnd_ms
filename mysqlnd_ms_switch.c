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


#define MS_STRING(vl, a)				\
{											\
	MAKE_STD_ZVAL((a));						\
	ZVAL_STRING((a), (char *)(vl), 1);	\
}

#define MS_STRINGL(vl, ln, a)				\
{											\
	MAKE_STD_ZVAL((a));						\
	ZVAL_STRINGL((a), (char *)(vl), (ln), 1);	\
}

#define MS_ARRAY(a)		\
{						\
	MAKE_STD_ZVAL((a));	\
	array_init((a));	\
}

/* {{{ mysqlnd_ms_call_handler */
static zval *
mysqlnd_ms_call_handler(zval *func, int argc, zval **argv, zend_bool destroy_args TSRMLS_DC)
{
	int i;
	zval * retval;
	DBG_ENTER("mysqlnd_ms_call_handler");

	MAKE_STD_ZVAL(retval);
	if (call_user_function(EG(function_table), NULL, func, retval, argc, argv TSRMLS_CC) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " %s Failed to call '%s'", MYSQLND_MS_ERROR_PREFIX, Z_STRVAL_P(func));
		zval_ptr_dtor(&retval);
		retval = NULL;
	}

	if (destroy_args == TRUE) {
		for (i = 0; i < argc; i++) {
			zval_ptr_dtor(&argv[i]);
		}
	}

	DBG_RETURN(retval);
}
/* }}} */


/* {{{ mysqlnd_ms_user_pick_server */
static MYSQLND *
mysqlnd_ms_user_pick_server(MYSQLND * conn, const char * query, size_t query_len,
							zend_llist * master_list, zend_llist * slave_list, zend_bool * use_all TSRMLS_DC)
{
	MYSQLND_MS_CONN_DATA ** conn_data = (MYSQLND_MS_CONN_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	zval * args[7];
	zval * retval;
	MYSQLND * ret = NULL;

	DBG_ENTER("mysqlnd_ms_user_pick_server");
	DBG_INF_FMT("query(50bytes)=%*s query_is_select=%p", MIN(50, query_len), query, MYSQLND_MS_G(user_pick_server));

	*use_all = 0;
	if (master_list && MYSQLND_MS_G(user_pick_server)) {
		uint param = 0;
#ifdef ALL_SERVER_DISPATCH
		uint use_all_pos = 0;
#endif
		/* connect host */
		MS_STRING((char *) (*conn_data)->connect_host, args[param]);

		/* query */
		param++;
		MS_STRINGL((char *) query, query_len, args[param]);
		{
			MYSQLND_MS_LIST_DATA * el;
			zend_llist_position	pos;
			/* master list */
			param++;
			MS_ARRAY(args[param]);
			for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(master_list, &pos); el && el->conn;
					el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(master_list, &pos))
			{
				if (CONN_GET_STATE(el->conn) == CONN_ALLOCED) {
					/* lazy */
					add_next_index_stringl(args[param], el->emulated_scheme, el->emulated_scheme_len, 1);
				} else {
					add_next_index_stringl(args[param], el->conn->scheme, el->conn->scheme_len, 1);
				}
			}

			/* slave list*/
			param++;
			MS_ARRAY(args[param]);
			if (slave_list) {
				for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(slave_list, &pos); el && el->conn;
						el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(slave_list, &pos))
				{
					if (CONN_GET_STATE(el->conn) == CONN_ALLOCED) {
						/* lazy */
						add_next_index_stringl(args[param], el->emulated_scheme, el->emulated_scheme_len, 1);
					} else {
						add_next_index_stringl(args[param], el->conn->scheme, el->conn->scheme_len, 1);
					}
				}
			}
			/* last used connection */
			param++;
			MAKE_STD_ZVAL(args[param]);
			if ((*conn_data)->last_used_connection) {
				ZVAL_STRING(args[param], ((*conn_data)->last_used_connection)->scheme, 1);
			} else {
				ZVAL_NULL(args[param]);
			}

			/* in transaction */
			param++;
			MAKE_STD_ZVAL(args[param]);
			if ((*conn_data)->stgy.in_transaction) {
				ZVAL_TRUE(args[param]);
			} else {
				ZVAL_FALSE(args[param]);
			}
#ifdef ALL_SERVER_DISPATCH
			/* use all */
			use_all_pos = ++param;
			MAKE_STD_ZVAL(args[param]);
			Z_ADDREF_P(args[param]);
			ZVAL_FALSE(args[param]);
#endif
		}

		retval = mysqlnd_ms_call_handler(MYSQLND_MS_G(user_pick_server), param + 1, args, FALSE /* we will destroy later */ TSRMLS_CC);
		if (retval) {
			if (Z_TYPE_P(retval) == IS_STRING) {
				do {
					MYSQLND_MS_LIST_DATA * el;
					zend_llist_position	pos;

					for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(master_list, &pos); !ret && el && el->conn;
							el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(master_list, &pos))
					{
						if (CONN_GET_STATE(el->conn) == CONN_ALLOCED) {
							/* lazy */
						} else {
							if (!strncasecmp(el->conn->scheme, Z_STRVAL_P(retval), MIN(Z_STRLEN_P(retval), el->conn->scheme_len))) {
								ret = el->conn;
								DBG_INF_FMT("Userfunc chose master host : [%*s]", el->conn->scheme_len, el->conn->scheme);
								MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_MASTER_CALLBACK);
							}
						}
					}
					if (slave_list) {
						for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(slave_list, &pos); !ret && el && el->conn;
								el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(slave_list, &pos))
						{
							if (CONN_GET_STATE(el->conn) == CONN_ALLOCED) {
								/* lazy */
								if (!strncasecmp(el->emulated_scheme, Z_STRVAL_P(retval), MIN(Z_STRLEN_P(retval), el->emulated_scheme_len))) {
									DBG_INF_FMT("Userfunc chose LAZY slave host : [%*s]", el->emulated_scheme_len, el->emulated_scheme);
									MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_SLAVE_CALLBACK);
									if (PASS == ms_orig_mysqlnd_conn_methods->connect(el->conn, el->host, el->user,
																				   el->passwd, el->passwd_len,
																				   el->db, el->db_len,
																				   el->port, el->socket,
																				   el->connect_flags TSRMLS_CC))
									{
										ret = el->conn;
										DBG_INF("Connected");
										MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_SUCCESS);
									} else {
										php_error_docref(NULL TSRMLS_CC, E_WARNING, "Callback chose %s but connection failed", el->emulated_scheme);
										DBG_ERR("Connect failed, forwarding error to the user");
										ret = el->conn; /* no automatic action: leave it to the user to decide! */
										MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_FAILURE);
									}
								}
							} else {
								if (!strncasecmp(el->conn->scheme, Z_STRVAL_P(retval), MIN(Z_STRLEN_P(retval), el->conn->scheme_len))) {
									MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_SLAVE_CALLBACK);
									ret = el->conn;
									DBG_INF_FMT("Userfunc chose slave host : [%*s]", el->conn->scheme_len, el->conn->scheme);
								}
							}
						}
					}
				} while (0);
			}
			zval_ptr_dtor(&retval);
		}
#ifdef ALL_SERVER_DISPATCH
		convert_to_boolean(args[use_all_pos]);
		*use_all = Z_BVAL_P(args[use_all_pos]);
		Z_DELREF_P(args[use_all_pos]);
#endif
		/* destroy the params */
		{
			unsigned int i;
			for (i = 0; i <= param; i++) {
				zval_ptr_dtor(&args[i]);
			}
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_choose_connection_rr */
static MYSQLND *
mysqlnd_ms_choose_connection_rr(MYSQLND * conn, const char * const query, const size_t query_len,
								zend_llist * master_connections, zend_llist * slave_connections,
								enum enum_which_server which_server, zend_bool forced, zend_bool * use_all TSRMLS_DC)
{
	MYSQLND_MS_CONN_DATA ** conn_data = (MYSQLND_MS_CONN_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	DBG_ENTER("mysqlnd_ms_choose_connection_rr");

	*use_all = 0;
	if (!conn_data || !*conn_data) {
		DBG_RETURN(conn);
	}
	if (((*conn_data)->stgy.trx_stickiness_strategy == TRX_STICKINESS_STRATEGY_MASTER) &&
		(*conn_data)->stgy.in_transaction && !forced)
	{
		DBG_INF("Enforcing use of master while in transaction");
		which_server = USE_MASTER;
		MYSQLND_MS_INC_STATISTIC(MS_STAT_TRX_MASTER_FORCED);
	} else if ((*conn_data)->stgy.mysqlnd_ms_flag_master_on_write) {
		if (which_server != USE_MASTER) {
			if ((*conn_data)->stgy.master_used && !forced) {
				switch (which_server) {
					case USE_MASTER:
					case USE_LAST_USED:
						break;
					case USE_ALL:
						*use_all = 1;
					case USE_SLAVE:
					default:
						DBG_INF("Enforcing use of master after write");
						which_server = USE_MASTER;
						break;
				}
			}
		} else {
			DBG_INF("Use of master detected");
			(*conn_data)->stgy.master_used = TRUE;
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

					if (SERVER_FAILOVER_DISABLED == (*conn_data)->stgy.failover_strategy) {
					  DBG_INF("Failover disabled");
					  DBG_RETURN(connection);
					}
					DBG_INF("Connect failed, falling back to the master");
				} else {
					DBG_RETURN(element->conn);
				}
			} else {
				if (SERVER_FAILOVER_DISABLED == (*conn_data)->stgy.failover_strategy) {
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
			DBG_RETURN((*conn_data)->last_used_connection);
		case USE_ALL:
			*use_all = 1;
			DBG_INF("Dispatch to all connections");
			DBG_RETURN(conn);
		default:
			/* error */
			DBG_RETURN(conn);
			break;
	}
}
/* }}} */


/* {{{ mysqlnd_ms_choose_connection_random */
static MYSQLND *
mysqlnd_ms_choose_connection_random(MYSQLND * conn, const char * const query, const size_t query_len,
									zend_llist * master_connections, zend_llist * slave_connections,
									enum enum_which_server which_server, zend_bool forced, zend_bool * use_all TSRMLS_DC)
{
	MYSQLND_MS_CONN_DATA ** conn_data = (MYSQLND_MS_CONN_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	DBG_ENTER("mysqlnd_ms_choose_connection_random");

	*use_all = 0;
	if (!conn_data || !*conn_data) {
		DBG_RETURN(conn);
	}
	which_server = mysqlnd_ms_query_is_select(query, query_len, &forced TSRMLS_CC);
	if (((*conn_data)->stgy.trx_stickiness_strategy == TRX_STICKINESS_STRATEGY_MASTER) &&
		(*conn_data)->stgy.in_transaction && !forced)
	{
		DBG_INF("Enforcing use of master while in transaction");
		which_server = USE_MASTER;
		MYSQLND_MS_INC_STATISTIC(MS_STAT_TRX_MASTER_FORCED);
	} else if ((*conn_data)->stgy.mysqlnd_ms_flag_master_on_write) {
		if (which_server != USE_MASTER) {
			if ((*conn_data)->stgy.master_used && !forced) {
				switch (which_server) {
					case USE_MASTER:
					case USE_LAST_USED:
						break;
					case USE_ALL:
						*use_all = 1;
					case USE_SLAVE:
					default:
						DBG_INF("Enforcing use of master after write");
						which_server = USE_MASTER;
						break;
				}
			}
		} else {
			DBG_INF("Use of master detected");
			(*conn_data)->stgy.master_used = TRUE;
		}
	}

	switch (which_server) {
		case USE_SLAVE:
		{
			zend_llist_position	pos;
			zend_llist * l = slave_connections;
			MYSQLND_MS_LIST_DATA * element;
			unsigned long rnd_idx;
			uint i = 0;
			MYSQLND * connection;

			rnd_idx = php_rand(TSRMLS_C);
			RAND_RANGE(rnd_idx, 0, zend_llist_count(l) - 1, PHP_RAND_MAX);
			DBG_INF_FMT("USE_SLAVE rnd_idx=%lu", rnd_idx);

			element = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(l, &pos);
			while (i++ < rnd_idx) {
				element = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(l, &pos);
			}
			connection = (element && element->conn) ? element->conn : NULL;
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
					if (SERVER_FAILOVER_DISABLED == (*conn_data)->stgy.failover_strategy) {
					  DBG_INF("Failover disabled");
					  DBG_RETURN(connection);
					}
					DBG_INF("Connect failed, falling back to the master");
				} else {
					DBG_RETURN(connection);
				}
			} else {
				if (SERVER_FAILOVER_DISABLED == (*conn_data)->stgy.failover_strategy) {
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
			DBG_RETURN((*conn_data)->last_used_connection);
		case USE_ALL:
			*use_all = 1;
			DBG_INF("Dispatch to all connections");
			DBG_RETURN(conn);
		default:
			/* error */
			DBG_RETURN(conn);
			break;
	}
}
/* }}} */


/* {{{ mysqlnd_ms_choose_connection_random_once */
static MYSQLND *
mysqlnd_ms_choose_connection_random_once(MYSQLND * conn, const char * const query, const size_t query_len,
										 zend_llist * master_connections, zend_llist * slave_connections,
										 enum enum_which_server which_server, zend_bool forced, zend_bool * use_all TSRMLS_DC)
{
	MYSQLND_MS_CONN_DATA ** conn_data = (MYSQLND_MS_CONN_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	DBG_ENTER("mysqlnd_ms_choose_connection_random_once");

	*use_all = 0;
	if (!conn_data || !*conn_data) {
		DBG_RETURN(conn);
	}
	if (((*conn_data)->stgy.trx_stickiness_strategy == TRX_STICKINESS_STRATEGY_MASTER) &&
		(*conn_data)->stgy.in_transaction && !forced)
	{
		DBG_INF("Enforcing use of master while in transaction");
		which_server = USE_MASTER;
		MYSQLND_MS_INC_STATISTIC(MS_STAT_TRX_MASTER_FORCED);
	} else if ((*conn_data)->stgy.mysqlnd_ms_flag_master_on_write) {
		if (which_server != USE_MASTER) {
			if ((*conn_data)->stgy.master_used && !forced) {
				switch (which_server) {
					case USE_MASTER:
					case USE_LAST_USED:
						break;
					case USE_ALL:
						*use_all = 1;
					case USE_SLAVE:
					default:
						DBG_INF("Enforcing use of master after write");
						which_server = USE_MASTER;
						break;
				}
			}
		} else {
			DBG_INF("Use of master detected");
			(*conn_data)->stgy.master_used = TRUE;
		}
	}
	switch (which_server) {
		case USE_SLAVE:
			if ((*conn_data)->random_once) {
				DBG_INF_FMT("Using last random connection "MYSQLND_LLU_SPEC, (*conn_data)->random_once->thread_id);
				DBG_RETURN((*conn_data)->random_once);
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
							(*conn_data)->random_once = connection;
							MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_SUCCESS);
							DBG_RETURN(connection);
						}
						MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_FAILURE);
						if (SERVER_FAILOVER_DISABLED == (*conn_data)->stgy.failover_strategy) {
						  DBG_INF("Failover disabled");
						  DBG_RETURN(connection);
						}
						DBG_INF("Connect failed, falling back to the master");
					} else {
						(*conn_data)->random_once = connection;
						DBG_RETURN(connection);
					}
				} else {
					if (SERVER_FAILOVER_DISABLED == (*conn_data)->stgy.failover_strategy) {
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
			DBG_RETURN((*conn_data)->last_used_connection);
		case USE_ALL:
			*use_all = 1;
			DBG_INF("Dispatch to all connections");
			DBG_RETURN(conn);
		default:
			/* error */
			DBG_RETURN(conn);
			break;
	}
}
/* }}} */


/* {{{ mysqlnd_ms_choose_connection_partition_by_filter */
static enum_func_status
mysqlnd_ms_choose_connection_partition_by_filter(MYSQLND * conn, const char * query, unsigned int query_len,
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
mysqlnd_ms_pick_server(MYSQLND * conn, const char * const query, const size_t query_len, zend_bool * use_all TSRMLS_DC)
{
	MYSQLND_MS_CONN_DATA ** conn_data = (MYSQLND_MS_CONN_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	MYSQLND * connection = conn;
	DBG_ENTER("mysqlnd_ms_pick_server");

	*use_all = 0;
	if (conn_data && *conn_data) {
		enum mysqlnd_ms_server_pick_strategy strategy = (*conn_data)->stgy.pick_strategy;
		zend_llist * master_list = &(*conn_data)->master_connections;
		zend_llist * slave_list = &(*conn_data)->slave_connections;

		connection = NULL;
		if (SERVER_PICK_USER == strategy) {
			connection = mysqlnd_ms_user_pick_server(conn, query, query_len, master_list, slave_list, use_all TSRMLS_CC);
			if (!connection) {
				strategy = (*conn_data)->stgy.fallback_pick_strategy;
			}
		}
		if (!connection) {
			zend_bool forced;
			enum enum_which_server which_server = mysqlnd_ms_query_is_select(query, query_len, &forced TSRMLS_CC);

			if (SERVER_PICK_RANDOM == strategy) {
				connection = mysqlnd_ms_choose_connection_random(conn, query, query_len, master_list, slave_list,
																 which_server, forced, use_all TSRMLS_CC);
			} else if (SERVER_PICK_RANDOM_ONCE == strategy) {
				connection = mysqlnd_ms_choose_connection_random_once(conn, query, query_len, master_list, slave_list,
																	  which_server, forced, use_all TSRMLS_CC);
			} else {
				connection = mysqlnd_ms_choose_connection_rr(conn, query, query_len, master_list, slave_list, 
															 which_server, forced, use_all TSRMLS_CC);
			}
		}
		if (!connection) {
			connection = conn;
		}
		(*conn_data)->last_used_connection = connection;
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
	zend_llist_position	pos;
	MYSQLND_MS_LIST_DATA * el;
	MYSQLND_MS_CONN_DATA ** conn_data = (MYSQLND_MS_CONN_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	DBG_ENTER("mysqlnd_ms_query_all");
	if (!conn_data || !*conn_data) {
		DBG_RETURN(ms_orig_mysqlnd_conn_methods->query(proxy_conn, query, query_len TSRMLS_CC));
	}
	/* search the list of easy handles hanging off the multi-handle */
	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(master_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(master_connections, &pos))
	{
		if (PASS != ms_orig_mysqlnd_conn_methods->query(el->conn, query, query_len TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(slave_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(slave_connections, &pos))
	{
		if (PASS != ms_orig_mysqlnd_conn_methods->query(el->conn, query, query_len TSRMLS_CC)) {
			ret = FAIL;
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
