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

/* $Id: header 252479 2008-02-07 19:39:50Z iliaa $ */

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
#include "mysqlnd_ms_ini.h"
#include "ext/standard/php_rand.h"


#define MASTER_NAME				"master"
#define SLAVE_NAME				"slave"
#define PICK_NAME				"pick"
#define PICK_RANDOM				"random"
#define PICK_RANDOM_ONCE 		"random_once"
#define PICK_RROBIN				"roundrobin"
#define PICK_USER				"user"
#define LAZY_NAME				"lazy_connections"
#define FAILOVER_NAME			"failover"
#define FAILOVER_DISABLED 		"disabled"
#define FAILOVER_MASTER			"master"
#define MASTER_ON_WRITE_NAME    "master_on_write"

static struct st_mysqlnd_conn_methods my_mysqlnd_conn_methods;
static struct st_mysqlnd_conn_methods *orig_mysqlnd_conn_methods;

static MYSQLND * mysqlnd_ms_choose_connection_rr(MYSQLND * conn, const char * const query, const size_t query_len, zend_bool * use_all TSRMLS_DC);
static MYSQLND * mysqlnd_ms_choose_connection_random(MYSQLND * conn, const char * const query, const size_t query_len, zend_bool * use_all TSRMLS_DC);
static void mysqlnd_ms_conn_free_plugin_data(MYSQLND *conn TSRMLS_DC);


enum mysqlnd_ms_server_pick_strategy
{
	SERVER_PICK_RROBIN,
	SERVER_PICK_RANDOM,
	SERVER_PICK_RANDOM_ONCE,
	SERVER_PICK_USER
};

#define DEFAULT_PICK_STRATEGY SERVER_PICK_RANDOM_ONCE

enum mysqlnd_ms_server_failover_strategy
{
	SERVER_FAILOVER_DISABLED,
	SERVER_FAILOVER_MASTER
};

#define DEFAULT_FAILOVER_STRATEGY SERVER_FAILOVER_DISABLED

typedef struct st_mysqlnd_ms_connection_data
{
	char * connect_host;
	zend_llist master_connections;
	zend_llist slave_connections;
	MYSQLND * last_used_connection;
	enum mysqlnd_ms_server_pick_strategy pick_strategy;
	enum mysqlnd_ms_server_pick_strategy fallback_pick_strategy;
	enum mysqlnd_ms_server_failover_strategy failover_strategy;
	MYSQLND * random_once;
	zend_bool flag_master_on_write;
	zend_bool master_used;

	char * user;
	char * passwd;
	size_t passwd_len;
	char * db;
	size_t db_len;
	unsigned int port;
	char * socket;
	unsigned long mysql_flags;
} MYSQLND_MS_CONNECTION_DATA;

typedef struct st_mysqlnd_ms_list_data
{
	MYSQLND * conn;
	char * host;
	unsigned int port;
	char * socket;
	char * emulated_scheme;
	size_t emulated_scheme_len;
	zend_bool persistent;
} MYSQLND_MS_LIST_DATA;

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

MYSQLND_STATS * mysqlnd_ms_stats = NULL;

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


/* {{{ mysqlnd_ms_get_scheme_from_list_data */
static int
mysqlnd_ms_get_scheme_from_list_data(MYSQLND_MS_LIST_DATA * el, char ** scheme, zend_bool persistent TSRMLS_DC)
{
	char * tmp = NULL;
	int scheme_len;
	*scheme = NULL;
#ifndef PHP_WIN32
	if (el->host && !strcasecmp("localhost", el->host)) {
		scheme_len = mnd_sprintf(&tmp, 0, "unix://%s", el->socket? el->socket : "/tmp/mysql.sock");
#else
	if (el->host && !strcmp(".", host) {
		scheme_len = mnd_sprintf(&tmp, 0, "pipe://%s", el->socket? el->socket : "\\\\.\\pipe\\MySQL");
#endif
	} else {
		if (!el->port) {
			el->port = 3306;
		}
		scheme_len = mnd_sprintf(&tmp, 0, "tcp://%s:%u", el->host? el->host:"localhost", el->port);
	}
	if (tmp) {
		*scheme = mnd_pestrndup(tmp, scheme_len, persistent);
		efree(tmp); /* allocated by spprintf */
	}
	return scheme_len;
}
/* }}} */


/* {{{ mysqlnd_ms_user_pick_server */
static MYSQLND *
mysqlnd_ms_user_pick_server(MYSQLND * conn, const char * query, size_t query_len, zend_bool * use_all TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	zend_llist * master_list = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->master_connections : NULL;
	zend_llist * slave_list = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->slave_connections : NULL;
	zval * args[6];
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
		MS_STRING((char *) (*conn_data_pp)->connect_host, args[param]);

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
			if ((*conn_data_pp)->last_used_connection) {
				ZVAL_STRING(args[param], ((*conn_data_pp)->last_used_connection)->scheme, 1);
			} else {
				ZVAL_NULL(args[param]);
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
									if (PASS == orig_mysqlnd_conn_methods->connect(el->conn, el->host, (*conn_data_pp)->user,
																				   (*conn_data_pp)->passwd, (*conn_data_pp)->passwd_len,
																				   (*conn_data_pp)->db, (*conn_data_pp)->db_len,
																				   el->port, el->socket, (*conn_data_pp)->mysql_flags TSRMLS_CC))
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


/* {{{ mysqlnd_ms_conn_list_dtor */
static void
mysqlnd_ms_conn_list_dtor(void * pDest)
{
	MYSQLND_MS_LIST_DATA * element = (MYSQLND_MS_LIST_DATA *) pDest;
	TSRMLS_FETCH();
	DBG_ENTER("mysqlnd_ms_conn_list_dtor");
	DBG_INF_FMT("conn=%p", element->conn);
	if (element->conn) {
		mysqlnd_close(element->conn, TRUE);
		element->conn = NULL;
	}
	if (element->host) {
		mnd_pefree(element->host, element->persistent);
		element->host = NULL;
	}
	if (element->socket) {
		mnd_pefree(element->socket, element->persistent);
		element->socket = NULL;
	}
	if (element->emulated_scheme) {
		mnd_pefree(element->emulated_scheme, element->persistent);
	}
	element->persistent = FALSE;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_ini_string_is_bool_true */
static zend_bool
mysqlnd_ms_ini_string_is_bool_false(const char * value)
{
	if (!value) {
		return TRUE;
	}
	if (!strncmp("0", value, sizeof("0") - 1)) {
		return TRUE;
	}
	if (!strncasecmp("false", value, sizeof("false") - 1)) {
		return TRUE;
	}
	if (!strncasecmp("off", value, sizeof("off") - 1)) {
		return TRUE;
	}
	if (!strncasecmp("aus", value, sizeof("aus") - 1)) {
		return TRUE;
	}
	return FALSE;
}
/* }}} */


/* {{{ mysqlnd_ms::connect */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, connect)(MYSQLND * conn,
									const char * host,
									const char * user,
									const char * passwd,
									unsigned int passwd_len,
									const char * db,
									unsigned int db_len,
									unsigned int port,
									const char * socket,
									unsigned int mysql_flags TSRMLS_DC)
{
	enum_func_status ret = FAIL;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp;
	size_t host_len = host? strlen(host) : 0;
	zend_bool section_found;
	zend_bool hotloading = MYSLQND_MS_HOTLOADING;

	DBG_ENTER("mysqlnd_ms::connect");
	if (hotloading) {
		MYSQLND_MS_CONFIG_LOCK;
	}
	section_found = mysqlnd_ms_ini_section_exists(&mysqlnd_ms_config, host, host_len, hotloading? FALSE:TRUE TSRMLS_CC);
	if (MYSQLND_MS_G(force_config_usage) && FALSE == section_found) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Exclusive usage of configuration enforced but did not find the correct INI file section (%s)", host);
		if (hotloading) {
			MYSQLND_MS_CONFIG_UNLOCK;
		}
		SET_CLIENT_ERROR(conn->error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, MYSQLND_MS_ERROR_PREFIX " Exclusive usage of configuration enforced but did not find the correct INI file section");
		DBG_RETURN(FAIL);
	}
	mysqlnd_ms_conn_free_plugin_data(conn TSRMLS_CC);

	conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	*conn_data_pp = mnd_pecalloc(1, sizeof(MYSQLND_MS_CONNECTION_DATA), conn->persistent);
	zend_llist_init(&(*conn_data_pp)->master_connections, sizeof(MYSQLND_MS_LIST_DATA), (llist_dtor_func_t) mysqlnd_ms_conn_list_dtor, conn->persistent);
	zend_llist_init(&(*conn_data_pp)->slave_connections, sizeof(MYSQLND_MS_LIST_DATA), (llist_dtor_func_t) mysqlnd_ms_conn_list_dtor, conn->persistent);
	(*conn_data_pp)->user = user? mnd_pestrdup(user, conn->persistent) : NULL;
	(*conn_data_pp)->passwd = passwd? mnd_pestrndup(passwd, (*conn_data_pp)->passwd_len = passwd_len, conn->persistent) : NULL;
	(*conn_data_pp)->db = db? mnd_pestrndup(db, (*conn_data_pp)->db_len = db_len, conn->persistent) : NULL;
	(*conn_data_pp)->port = port;
	(*conn_data_pp)->socket = socket? mnd_pestrdup(socket, conn->persistent) : NULL;
	(*conn_data_pp)->mysql_flags = mysql_flags;

	if (FALSE == section_found) {
		ret = orig_mysqlnd_conn_methods->connect(conn, host, user, passwd, passwd_len, db, db_len, port, socket, mysql_flags TSRMLS_CC);
		if (ret == PASS) {
			MYSQLND_MS_LIST_DATA new_element = {0};
			new_element.conn = conn;
			new_element.host = host? mnd_pestrdup(host, conn->persistent) : NULL;
			new_element.persistent = conn->persistent;
			new_element.emulated_scheme_len = mysqlnd_ms_get_scheme_from_list_data(&new_element, &new_element.emulated_scheme,
																				   conn->persistent TSRMLS_CC);
			zend_llist_add_element(&(*conn_data_pp)->master_connections, &new_element);
		}
	} else {
		do {
			unsigned int port_to_use;
			const char * socket_to_use;
			zend_bool value_exists = FALSE, is_list_value = FALSE, use_lazy_connections = TRUE, use_lazy_connections_list_value = FALSE, have_slaves = FALSE;
			char * lazy_connections;
			/* create master connection */
			char * master;

			if (!hotloading) {
				MYSQLND_MS_CONFIG_LOCK;
			}

			master = mysqlnd_ms_ini_string(&mysqlnd_ms_config, host, host_len, MASTER_NAME, sizeof(MASTER_NAME) - 1,
												  &value_exists, &is_list_value, FALSE TSRMLS_CC);
			if (FALSE == value_exists) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Cannot find master section in config");
				SET_CLIENT_ERROR(conn->error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, MYSQLND_MS_ERROR_PREFIX " Cannot find master section in config");
				DBG_RETURN(FAIL);
			}

			lazy_connections = mysqlnd_ms_ini_string(&mysqlnd_ms_config, host, host_len, LAZY_NAME, sizeof(LAZY_NAME) - 1,
													 &use_lazy_connections, &use_lazy_connections_list_value, FALSE TSRMLS_CC);
			/* ignore if lazy_connections ini entry exists or not */
			use_lazy_connections = TRUE;
			if (lazy_connections) {
				/* lazy_connections ini entry exists, disabled? */
				use_lazy_connections = !mysqlnd_ms_ini_string_is_bool_false(lazy_connections);
				mnd_efree(lazy_connections);
				lazy_connections = NULL;
			}
			{
				char * colon_pos;
				port_to_use = port;
				socket_to_use = socket;

				if (NULL != (colon_pos = strchr(master, ':'))) {
					if (colon_pos[1] == '/') {
						/* unix path */
						socket_to_use = colon_pos + 1;
						DBG_INF_FMT("overwriting socket : %s", socket_to_use);
					} else if (isdigit(colon_pos[1])) {
						/* port */
						port_to_use = atoi(colon_pos+1);
						DBG_INF_FMT("overwriting port : %d", port_to_use);
					}
					*colon_pos = '\0'; /* strip the tail */
				}
			}

			if (use_lazy_connections) {
				DBG_INF("Lazy master connection");
				ret = PASS;
			} else {
				ret = orig_mysqlnd_conn_methods->connect(conn, master, user, passwd, passwd_len, db, db_len, port_to_use, socket_to_use, mysql_flags TSRMLS_CC);
			}

			if (ret != PASS) {
				mnd_efree(master);
				MYSQLND_MS_INC_STATISTIC(MS_STAT_NON_LAZY_CONN_MASTER_FAILURE);
				break;
			} else {
				if (!use_lazy_connections)
					MYSQLND_MS_INC_STATISTIC(MS_STAT_NON_LAZY_CONN_MASTER_SUCCESS);
				MYSQLND_MS_LIST_DATA new_element = {0};
				new_element.conn = conn;
				new_element.host = master? mnd_pestrdup(master, conn->persistent) : NULL;
				new_element.persistent = conn->persistent;
				new_element.port = port_to_use;
				new_element.socket = socket_to_use? mnd_pestrdup(socket_to_use, conn->persistent) : NULL;
				new_element.emulated_scheme_len = mysqlnd_ms_get_scheme_from_list_data(&new_element, &new_element.emulated_scheme,
																					   conn->persistent TSRMLS_CC);
				zend_llist_add_element(&(*conn_data_pp)->master_connections, &new_element);
			}
			mnd_efree(master);
			DBG_INF_FMT("Master connection "MYSQLND_LLU_SPEC" established", conn->m->get_thread_id(conn TSRMLS_CC));
#ifdef MYSQLND_MS_MULTIMASTER_ENABLED
			/* More master connections ? */
			if (is_list_value) {
				DBG_INF("We have more master connections. Connect...");
				do {
					master = mysqlnd_ms_ini_string(&mysqlnd_ms_config, host, host_len, MASTER_NAME, sizeof(MASTER_NAME) - 1,
												   &value_exists, &is_list_value, FALSE TSRMLS_CC);
					DBG_INF_FMT("value_exists=%d master=%s", value_exists, master);
					if (value_exists && master) {
						MYSQLND * tmp_conn = mysqlnd_init(conn->persistent);

						{
							char * colon_pos;
							port_to_use = port;
							socket_to_use = socket;

							if (NULL != (colon_pos = strchr(master, ':'))) {
								if (colon_pos[1] == '/') {
									/* unix path */
									socket_to_use = colon_pos + 1;
									DBG_INF_FMT("overwriting socket : %s", socket_to_use);
								} else if (isdigit(colon_pos[1])) {
									/* port */
									port_to_use = atoi(colon_pos+1);
									DBG_INF_FMT("overwriting port : %d", port_to_use);
								}
								*colon_pos = '\0'; /* strip the tail */
							}
						}

						if (use_lazy_connections)
							DBG_INF("Lazy master connections");
							ret = PASS;
						} else {
							ret = orig_mysqlnd_conn_methods->connect(tmp_conn, master, user, passwd, passwd_len, db, db_len, port_to_use, socket_to_use, mysql_flags TSRMLS_CC);
						}

						if (ret == PASS) {
							if (!use_lazy_connections)
								MYSQLND_MS_INC_STATISTIC(MS_STAT_NON_LAZY_CONN_MASTER_SUCCESS);
							MYSQLND_MS_LIST_DATA new_element = {0};
							new_element.conn = tmp_conn;
							new_element.host = master? mnd_pestrdup(master, conn->persistent) : NULL;
							new_element.persistent = conn->persistent;
							new_element.port = port_to_use;
							new_element.socket = socket_to_use? mnd_pestrdup(socket_to_use, conn->persistent) : NULL;
							new_element.emulated_scheme_len = mysqlnd_ms_get_scheme_from_list_data(&new_element, &new_element.emulated_scheme,
																								   conn->persistent TSRMLS_CC);
							zend_llist_add_element(&(*conn_data_pp)->master_connections, &new_element);
							DBG_INF_FMT("Further master connection "MYSQLND_LLU_SPEC" established", tmp_conn->m->get_thread_id(tmp_conn TSRMLS_CC));
						} else {
							MYSQLND_MS_INC_STATISTIC(MS_STAT_NON_LAZY_CONN_MASTER_FAILURE);
							tmp_conn->m->dtor(tmp_conn TSRMLS_CC);
						}
						mnd_efree(master);
						master = NULL;
					}
				} while (value_exists);
			}
#endif
			/* create slave slave_connections */
			do {
				char * slave = mysqlnd_ms_ini_string(&mysqlnd_ms_config, host, host_len, SLAVE_NAME, sizeof(SLAVE_NAME) - 1,
													 &value_exists, &is_list_value, FALSE TSRMLS_CC);
				if (value_exists && is_list_value && slave) {
					have_slaves = TRUE;
					MYSQLND * tmp_conn = mysqlnd_init(conn->persistent);

					{
						char * colon_pos;
						port_to_use = port;
						socket_to_use = socket;

						if (NULL != (colon_pos = strchr(slave, ':'))) {
							if (colon_pos[1] == '/') {
								/* unix path */
								socket_to_use = colon_pos + 1;
								DBG_INF_FMT("overwriting socket : %s", socket_to_use);
							} else if (isdigit(colon_pos[1])) {
								/* port */
								port_to_use = atoi(colon_pos+1);
								DBG_INF_FMT("overwriting port : %d", port_to_use);
							}
							*colon_pos = '\0'; /* strip the tail */
						}
					}

					if (use_lazy_connections) {
						DBG_INF("Lazy slave connections");
						ret = PASS;
					} else {
						ret = orig_mysqlnd_conn_methods->connect(tmp_conn, slave, user, passwd, passwd_len, db, db_len, port_to_use, socket_to_use, mysql_flags TSRMLS_CC);
					}

					if (ret == PASS) {
						if (!use_lazy_connections)
							MYSQLND_MS_INC_STATISTIC(MS_STAT_NON_LAZY_CONN_SLAVE_SUCCESS);
						MYSQLND_MS_LIST_DATA new_element = {0};
						new_element.conn = tmp_conn;
						new_element.host = slave? mnd_pestrdup(slave, conn->persistent) : NULL;
						new_element.persistent = conn->persistent;
						new_element.port = port_to_use;
						new_element.socket = socket_to_use? mnd_pestrdup(socket_to_use, conn->persistent) : NULL;
						new_element.emulated_scheme_len = mysqlnd_ms_get_scheme_from_list_data(&new_element, &(new_element.emulated_scheme),
																							   conn->persistent TSRMLS_CC);
						zend_llist_add_element(&(*conn_data_pp)->slave_connections, &new_element);
						DBG_INF_FMT("Slave connection "MYSQLND_LLU_SPEC" established", tmp_conn->m->get_thread_id(tmp_conn TSRMLS_CC));
					} else {
						MYSQLND_MS_INC_STATISTIC(MS_STAT_NON_LAZY_CONN_SLAVE_FAILURE);
						tmp_conn->m->dtor(tmp_conn TSRMLS_CC);
					}
					mnd_efree(slave);
					slave = NULL;
				}
			} while (value_exists);

			if (FALSE == have_slaves) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Cannot find slaves section in config");
				SET_CLIENT_ERROR(conn->error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, MYSQLND_MS_ERROR_PREFIX " Cannot find slaves section in config");
				DBG_RETURN(FAIL);
			}

			{
				char * pick_strategy = mysqlnd_ms_ini_string(&mysqlnd_ms_config, host, host_len, PICK_NAME, sizeof(PICK_NAME) - 1,
												  				&value_exists, &is_list_value, FALSE TSRMLS_CC);
				(*conn_data_pp)->pick_strategy = (*conn_data_pp)->fallback_pick_strategy = DEFAULT_PICK_STRATEGY;

				if (value_exists && pick_strategy) {
					/* random is a substing of random_once thus we check first for random_once */
					if (!strncasecmp(PICK_RROBIN, pick_strategy, sizeof(PICK_RROBIN) - 1)) {
						(*conn_data_pp)->pick_strategy = (*conn_data_pp)->fallback_pick_strategy = SERVER_PICK_RROBIN;
					} else if (!strncasecmp(PICK_RANDOM_ONCE, pick_strategy, sizeof(PICK_RANDOM_ONCE) - 1)) {
						(*conn_data_pp)->pick_strategy = (*conn_data_pp)->fallback_pick_strategy = SERVER_PICK_RANDOM_ONCE;
					} else if (!strncasecmp(PICK_RANDOM, pick_strategy, sizeof(PICK_RANDOM) - 1)) {
						(*conn_data_pp)->pick_strategy = (*conn_data_pp)->fallback_pick_strategy = SERVER_PICK_RANDOM;
					} else if (!strncasecmp(PICK_USER, pick_strategy, sizeof(PICK_USER) - 1)) {
						(*conn_data_pp)->pick_strategy = SERVER_PICK_USER;
						if (is_list_value) {
							mnd_efree(pick_strategy);
							pick_strategy = mysqlnd_ms_ini_string(&mysqlnd_ms_config, host, host_len, PICK_NAME, sizeof(PICK_NAME) - 1,
												  				&value_exists, &is_list_value, FALSE TSRMLS_CC);
							if (pick_strategy) {
								if (!strncasecmp(PICK_RANDOM_ONCE, pick_strategy, sizeof(PICK_RANDOM_ONCE) - 1)) {
									(*conn_data_pp)->fallback_pick_strategy = SERVER_PICK_RANDOM_ONCE;
								} else if (!strncasecmp(PICK_RANDOM, pick_strategy, sizeof(PICK_RANDOM) - 1)) {
									(*conn_data_pp)->fallback_pick_strategy = SERVER_PICK_RANDOM;
								} else if (!strncasecmp(PICK_RROBIN, pick_strategy, sizeof(PICK_RROBIN) - 1)) {
									(*conn_data_pp)->fallback_pick_strategy = SERVER_PICK_RROBIN;
								}
							}
						}
					}
				}
				if (pick_strategy) {
					mnd_efree(pick_strategy);
				}
			}

			{
				char * failover_strategy = mysqlnd_ms_ini_string(&mysqlnd_ms_config, host, host_len, FAILOVER_NAME, sizeof(FAILOVER_NAME) - 1,
												  				&value_exists, &is_list_value, FALSE TSRMLS_CC);
				(*conn_data_pp)->failover_strategy = DEFAULT_FAILOVER_STRATEGY;

				if (value_exists && failover_strategy) {
					if (!strncasecmp(FAILOVER_DISABLED, failover_strategy, sizeof(FAILOVER_DISABLED) - 1)) {
						(*conn_data_pp)->failover_strategy = SERVER_FAILOVER_DISABLED;
					} else if (!strncasecmp(FAILOVER_MASTER, failover_strategy, sizeof(FAILOVER_MASTER) - 1)) {
						(*conn_data_pp)->failover_strategy = SERVER_FAILOVER_MASTER;
					}
				}
				if (failover_strategy) {
					mnd_efree(failover_strategy);
				}
			}

			{
				char * master_on_write = mysqlnd_ms_ini_string(&mysqlnd_ms_config, host, host_len, MASTER_ON_WRITE_NAME, sizeof(MASTER_ON_WRITE_NAME) - 1,
													 &value_exists, &is_list_value, FALSE TSRMLS_CC);

				(*conn_data_pp)->flag_master_on_write = FALSE;
				(*conn_data_pp)->master_used = FALSE;

				if (value_exists && master_on_write) {
					DBG_INF("Master on write active");
					(*conn_data_pp)->flag_master_on_write = !mysqlnd_ms_ini_string_is_bool_false(master_on_write);
				}
				if (master_on_write) {
					mnd_efree(master_on_write);
				}
			}

		} while (0);
		mysqlnd_ms_ini_reset_section(&mysqlnd_ms_config, host, host_len, FALSE TSRMLS_CC);
		if (!hotloading) {
			MYSQLND_MS_CONFIG_UNLOCK;
		}
	}

	if (hotloading) {
		MYSQLND_MS_CONFIG_UNLOCK;
	}
	if (ret == PASS) {
		(*conn_data_pp)->connect_host = host? mnd_pestrdup(host, conn->persistent) : NULL;
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_query_is_select */
enum enum_which_server
mysqlnd_ms_query_is_select(const char * query, size_t query_len, zend_bool * forced TSRMLS_DC)
{
	enum enum_which_server ret = USE_MASTER;
	struct st_qc_token_and_value token;
	struct st_mysqlnd_tok_scanner * scanner;
	DBG_ENTER("mysqlnd_ms_query_is_select");
	*forced = FALSE;
	if (!query) {
		DBG_RETURN(USE_MASTER);
	}
	scanner = mysqlnd_tok_create_scanner(query, query_len TSRMLS_CC);
	token = mysqlnd_tok_get_token(scanner TSRMLS_CC);
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
		token = mysqlnd_tok_get_token(scanner TSRMLS_CC);
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
	mysqlnd_tok_free_scanner(scanner TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_choose_connection_rr */
static MYSQLND *
mysqlnd_ms_choose_connection_rr(MYSQLND * conn, const char * const query, const size_t query_len, zend_bool * use_all TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	zend_bool forced;
	enum enum_which_server which_server;
	DBG_ENTER("mysqlnd_ms_choose_connection_rr");

	*use_all = 0;
	if (!conn_data_pp || !*conn_data_pp) {
		DBG_RETURN(conn);
	}
	which_server = mysqlnd_ms_query_is_select(query, query_len, &forced TSRMLS_CC);
	if ((*conn_data_pp)->flag_master_on_write) {
		if (which_server != USE_MASTER) {
			if ((*conn_data_pp)->master_used && !forced) {
				switch (which_server) {
					case USE_MASTER:
					case USE_LAST_USED:
						break;
					case USE_SLAVE:
					case USE_ALL:
					default:
						DGB_INF("Enforcing use of master after write");
						which_server = USE_MASTER;
						break;
				}
			}
		} else {
			DGB_INF("Use of master detected");
			(*conn_data_pp)->master_used = TRUE;
		}
	}
	switch (which_server) {
		case USE_SLAVE:
		{
			zend_llist * l = &(*conn_data_pp)->slave_connections;
			MYSQLND_MS_LIST_DATA * element = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next(l);
			MYSQLND * connection = (element && element->conn)? element->conn : (((element = zend_llist_get_first(l)) && element->conn)? element->conn : NULL);
			if (connection) {
				DBG_INF_FMT("Using slave connection "MYSQLND_LLU_SPEC"", connection->thread_id);

				if (CONN_GET_STATE(connection) == CONN_ALLOCED) {
					DBG_INF("Lazy connection, trying to connect...");
					/* lazy connection, connect now */
					if (PASS == orig_mysqlnd_conn_methods->connect(connection, element->host, (*conn_data_pp)->user,
																	(*conn_data_pp)->passwd, (*conn_data_pp)->passwd_len,
																	(*conn_data_pp)->db, (*conn_data_pp)->db_len,
																	element->port, element->socket, (*conn_data_pp)->mysql_flags TSRMLS_CC))
					{
						DBG_INF("Connected");
						MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_SUCCESS);
						DBG_RETURN(connection);
					}
					MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_FAILURE);

					if (SERVER_FAILOVER_DISABLED == (*conn_data_pp)->failover_strategy) {
					  DBG_INF("Failover disabled");
					  DBG_RETURN(connection);
					}
					DBG_INF("Connect failed, falling back to the master");
				} else {
					DBG_RETURN(element->conn);
				}
			} else {
				if (SERVER_FAILOVER_DISABLED == (*conn_data_pp)->failover_strategy) {
					DBG_INF("Failover disabled");
					DBG_RETURN(connection);
				}
			}
		}
		/* fall-through */
		case USE_MASTER:
		{
			zend_llist * l = &(*conn_data_pp)->master_connections;
			MYSQLND_MS_LIST_DATA * element = zend_llist_get_next(l);
			MYSQLND * connection = (element && element->conn)? element->conn : (((element = zend_llist_get_first(l)) && element->conn)? element->conn : NULL);
			DBG_INF("Using master connection");
			if (connection) {
				if (CONN_GET_STATE(connection) == CONN_ALLOCED) {
					DBG_INF("Lazy connection, trying to connect...");
					/* lazy connection, connect now */
					if (PASS == orig_mysqlnd_conn_methods->connect(connection, element->host, (*conn_data_pp)->user,
																	(*conn_data_pp)->passwd, (*conn_data_pp)->passwd_len,
																	(*conn_data_pp)->db, (*conn_data_pp)->db_len,
																	element->port, element->socket, (*conn_data_pp)->mysql_flags TSRMLS_CC))
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
			DBG_RETURN((*conn_data_pp)->last_used_connection);
		case USE_ALL:
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
mysqlnd_ms_choose_connection_random(MYSQLND * conn, const char * const query, const size_t query_len, zend_bool * use_all TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	zend_bool forced;
	enum enum_which_server which_server;
	DBG_ENTER("mysqlnd_ms_choose_connection_random");

	*use_all = 0;
	if (!conn_data_pp || !*conn_data_pp) {
		DBG_RETURN(conn);
	}
	which_server = mysqlnd_ms_query_is_select(query, query_len, &forced TSRMLS_CC);
	if ((*conn_data_pp)->flag_master_on_write) {
		if (which_server != USE_MASTER) {
			if ((*conn_data_pp)->master_used && !forced) {
				switch (which_server) {
					case USE_MASTER:
					case USE_LAST_USED:
						break;
					case USE_SLAVE:
					case USE_ALL:
					default:
						DGB_INF("Enforcing use of master after write");
						which_server = USE_MASTER;
						break;
				}
			}
		} else {
			DGB_INF("Use of master detected");
			(*conn_data_pp)->master_used = TRUE;
		}
	}
	switch (which_server) {
		case USE_SLAVE:
		{
			zend_llist_position	pos;
			zend_llist * l = &(*conn_data_pp)->slave_connections;
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
					if (PASS == orig_mysqlnd_conn_methods->connect(connection, element->host, (*conn_data_pp)->user,
																	(*conn_data_pp)->passwd, (*conn_data_pp)->passwd_len,
																	(*conn_data_pp)->db, (*conn_data_pp)->db_len,
																	element->port, element->socket, (*conn_data_pp)->mysql_flags TSRMLS_CC))
					{
						DBG_INF("Connected");
						MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_SUCCESS);
						DBG_RETURN(connection);
					}
					MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_FAILURE);
					if (SERVER_FAILOVER_DISABLED == (*conn_data_pp)->failover_strategy) {
					  DBG_INF("Failover disabled");
					  DBG_RETURN(connection);
					}
					DBG_INF("Connect failed, falling back to the master");
				} else {
					DBG_RETURN(connection);
				}
			} else {
				if (SERVER_FAILOVER_DISABLED == (*conn_data_pp)->failover_strategy) {
					DBG_INF("Failover disabled");
					DBG_RETURN(connection);
				}
			}
		}
		/* fall-through */
		case USE_MASTER:
		{
			zend_llist_position	pos;
			zend_llist * l = &(*conn_data_pp)->master_connections;
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
					if (PASS == orig_mysqlnd_conn_methods->connect(connection, element->host, (*conn_data_pp)->user,
																	(*conn_data_pp)->passwd, (*conn_data_pp)->passwd_len,
																	(*conn_data_pp)->db, (*conn_data_pp)->db_len,
																	element->port, element->socket, (*conn_data_pp)->mysql_flags TSRMLS_CC))
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
			DBG_RETURN((*conn_data_pp)->last_used_connection);
		case USE_ALL:
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
mysqlnd_ms_choose_connection_random_once(MYSQLND * conn, const char * const query, const size_t query_len, zend_bool * use_all TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	zend_bool forced;
	enum enum_which_server which_server;
	DBG_ENTER("mysqlnd_ms_choose_connection_random_once");

	*use_all = 0;
	if (!conn_data_pp || !*conn_data_pp) {
		DBG_RETURN(conn);
	}
	which_server = mysqlnd_ms_query_is_select(query, query_len, &forced TSRMLS_CC);
	if ((*conn_data_pp)->flag_master_on_write) {
		if (which_server != USE_MASTER) {
			if ((*conn_data_pp)->master_used && !forced) {
				switch (which_server) {
					case USE_MASTER:
					case USE_LAST_USED:
						break;
					case USE_SLAVE:
					case USE_ALL:
					default:
						DBG_INF("Enforcing use of master after write");
						which_server = USE_MASTER;
						break;
				}
			}
		} else {
			DBG_INF("Use of master detected");
			(*conn_data_pp)->master_used = TRUE;
		}
	}
	switch (which_server) {
		case USE_SLAVE:
			if ((*conn_data_pp)->random_once) {
				DBG_INF_FMT("Using last random connection "MYSQLND_LLU_SPEC, (*conn_data_pp)->random_once->thread_id);
				DBG_RETURN((*conn_data_pp)->random_once);
			} else {
				zend_llist_position	pos;
				zend_llist * l = &(*conn_data_pp)->slave_connections;
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
						if (PASS == orig_mysqlnd_conn_methods->connect(connection, element->host, (*conn_data_pp)->user,
																		(*conn_data_pp)->passwd, (*conn_data_pp)->passwd_len,
																		(*conn_data_pp)->db, (*conn_data_pp)->db_len,
																		element->port, element->socket, (*conn_data_pp)->mysql_flags TSRMLS_CC))
						{
							DBG_INF("Connected");
							(*conn_data_pp)->random_once = connection;
							MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_SUCCESS);
							DBG_RETURN(connection);
						}
						MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_FAILURE);
						if (SERVER_FAILOVER_DISABLED == (*conn_data_pp)->failover_strategy) {
						  DBG_INF("Failover disabled");
						  DBG_RETURN(connection);
						}
						DBG_INF("Connect failed, falling back to the master");
					} else {
						(*conn_data_pp)->random_once = connection;
						DBG_RETURN(connection);
					}
				} else {
					if (SERVER_FAILOVER_DISABLED == (*conn_data_pp)->failover_strategy) {
						DBG_INF("Failover disabled");
						DBG_RETURN(connection);
					}
				}
			}
		/* fall-through */
		case USE_MASTER:
		{
			zend_llist_position	pos;
			zend_llist * l = &(*conn_data_pp)->master_connections;
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
					if (PASS == orig_mysqlnd_conn_methods->connect(connection, element->host, (*conn_data_pp)->user,
																	(*conn_data_pp)->passwd, (*conn_data_pp)->passwd_len,
																	(*conn_data_pp)->db, (*conn_data_pp)->db_len,
																	element->port, element->socket, (*conn_data_pp)->mysql_flags TSRMLS_CC))
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
			DBG_RETURN((*conn_data_pp)->last_used_connection);
		case USE_ALL:
			DBG_INF("Dispatch to all connections");
			DBG_RETURN(conn);
		default:
			/* error */
			DBG_RETURN(conn);
			break;
	}
}
/* }}} */


/* {{{ mysqlnd_ms_pick_server */
static MYSQLND *
mysqlnd_ms_pick_server(MYSQLND * conn, const char * const query, const size_t query_len, zend_bool * use_all TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	MYSQLND * connection = conn;
	DBG_ENTER("mysqlnd_ms_pick_server");

	*use_all = 0;
	if (conn_data_pp && *conn_data_pp) {
		enum mysqlnd_ms_server_pick_strategy strategy = (*conn_data_pp)->pick_strategy;
		connection = NULL;
		if (SERVER_PICK_USER == strategy) {
			connection = mysqlnd_ms_user_pick_server(conn, query, query_len, use_all TSRMLS_CC);
			if (!connection) {
				strategy = (*conn_data_pp)->fallback_pick_strategy;
			}
		}
		if (!connection) {
			if (SERVER_PICK_RANDOM == strategy) {
				connection = mysqlnd_ms_choose_connection_random(conn, query, query_len, use_all TSRMLS_CC);
			} else if (SERVER_PICK_RANDOM_ONCE == strategy) {
				connection = mysqlnd_ms_choose_connection_random_once(conn, query, query_len, use_all TSRMLS_CC);
			} else {
				connection = mysqlnd_ms_choose_connection_rr(conn, query, query_len, use_all TSRMLS_CC);
			}
		}
		if (!connection) {
			connection = conn;
		}
		(*conn_data_pp)->last_used_connection = connection;
	}

	DBG_RETURN(connection);
}
/* }}} */


#ifdef ALL_SERVER_DISPATCH
/* {{{ mysqlnd_ms_query_all */
static enum_func_status
mysqlnd_ms_query_all(MYSQLND * const proxy_conn, const char * query, unsigned int query_len TSRMLS_DC)
{
	enum_func_status ret = PASS;
	zend_llist_position	pos;
	MYSQLND_MS_LIST_DATA * el;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	DBG_ENTER("mysqlnd_ms_query_all");
	if (!conn_data_pp || !*conn_data_pp) {
		DBG_RETURN(orig_mysqlnd_conn_methods->query(proxy_conn, query, query_len TSRMLS_CC));
	}
	/* search the list of easy handles hanging off the multi-handle */
	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(&(*conn_data_pp)->master_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(&(*conn_data_pp)->master_connections, &pos))
	{
		if (PASS != orig_mysqlnd_conn_methods->query(el->conn, query, query_len TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(&(*conn_data_pp)->slave_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(&(*conn_data_pp)->slave_connections, &pos))
	{
		if (PASS != orig_mysqlnd_conn_methods->query(el->conn, query, query_len TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	DBG_RETURN(ret);
}
/* }}} */
#endif


/* {{{ MYSQLND_METHOD(mysqlnd_ms, query) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, query)(MYSQLND * conn, const char * query, unsigned int query_len TSRMLS_DC)
{
	MYSQLND * connection;
	enum_func_status ret = FAIL;
	zend_bool use_all = 0;
	DBG_ENTER("mysqlnd_ms::query");

	connection = mysqlnd_ms_pick_server(conn, query, query_len, &use_all TSRMLS_CC);
	DBG_INF_FMT("Connection %p", connection);
	if (connection->error_info.error_no)
		  DBG_RETURN(ret);

#ifdef ALL_SERVER_DISPATCH
	if (use_all) {
		mysqlnd_ms_query_all(conn, query, query_len TSRMLS_CC);
	}
#endif

	ret = orig_mysqlnd_conn_methods->query(connection, query, query_len TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::use_result */
static MYSQLND_RES *
MYSQLND_METHOD(mysqlnd_ms, use_result)(MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_RES * result;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	MYSQLND * conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	DBG_ENTER("mysqlnd_ms::use_result");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, (conn)->m->get_thread_id(conn TSRMLS_CC));
	result = orig_mysqlnd_conn_methods->use_result(conn TSRMLS_CC);
	DBG_RETURN(result);
}
/* }}} */


/* {{{ mysqlnd_ms::store_result */
static MYSQLND_RES *
MYSQLND_METHOD(mysqlnd_ms, store_result)(MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_RES * result;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	MYSQLND * conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	DBG_ENTER("mysqlnd_ms::store_result");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, (conn)->m->get_thread_id(conn TSRMLS_CC));
	result = orig_mysqlnd_conn_methods->store_result(conn TSRMLS_CC);
	DBG_RETURN(result);
}
/* }}} */


/* {{{ mysqlnd_ms_conn_free_plugin_data */
static void
mysqlnd_ms_conn_free_plugin_data(MYSQLND *conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** data_pp;
	DBG_ENTER("mysqlnd_ms_conn_free_plugin_data");

	data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	if (data_pp && *data_pp) {
		if ((*data_pp)->connect_host) {
			mnd_pefree((*data_pp)->connect_host, conn->persistent);
			(*data_pp)->connect_host = NULL;
		}
		if ((*data_pp)->user) {
			mnd_pefree((*data_pp)->user, conn->persistent);
			(*data_pp)->user = NULL;
		}

		if ((*data_pp)->passwd) {
			mnd_pefree((*data_pp)->passwd, conn->persistent);
			(*data_pp)->passwd = NULL;
		}
		(*data_pp)->passwd_len = 0;

		if ((*data_pp)->db) {
			mnd_pefree((*data_pp)->db, conn->persistent);
			(*data_pp)->db = NULL;
		}
		(*data_pp)->db_len = 0;

		if ((*data_pp)->socket) {
			mnd_pefree((*data_pp)->socket, conn->persistent);
			(*data_pp)->socket = NULL;
		}

		(*data_pp)->port = 0;
		(*data_pp)->mysql_flags = 0;

		DBG_INF_FMT("cleaning the llists");
		zend_llist_clean(&(*data_pp)->master_connections);
		zend_llist_clean(&(*data_pp)->slave_connections);
		mnd_pefree(*data_pp, conn->persistent);
		*data_pp = NULL;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms::free_contents */
static void
MYSQLND_METHOD(mysqlnd_ms, free_contents)(MYSQLND *conn TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms::free_contents");
	if (conn->refcount > 0)
		DBG_VOID_RETURN;

	mysqlnd_ms_conn_free_plugin_data(conn TSRMLS_CC);
	orig_mysqlnd_conn_methods->free_contents(conn TSRMLS_CC);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms::escape_string */
static ulong
MYSQLND_METHOD(mysqlnd_ms, escape_string)(const MYSQLND * const proxy_conn, char *newstr, const char *escapestr, size_t escapestr_len TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	DBG_ENTER("mysqlnd_ms::escape_string");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, (conn)->m->get_thread_id(conn TSRMLS_CC));
	DBG_RETURN(orig_mysqlnd_conn_methods->escape_string(conn, newstr, escapestr, escapestr_len TSRMLS_CC));
}
/* }}} */




/* {{{ mysqlnd_ms::change_user */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, change_user)(MYSQLND * const proxy_conn,
										  const char *user,
										  const char *passwd,
										  const char *db,
										  zend_bool silent
#if PHP_VERSION_ID >= 50399
										  ,size_t passwd_len
#endif
										  TSRMLS_DC)
{
	enum_func_status ret = PASS;
	zend_llist_position	pos;
	MYSQLND_MS_LIST_DATA * el;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	DBG_ENTER("mysqlnd_ms::select_db");
	if (!conn_data_pp || !*conn_data_pp) {
#if PHP_VERSION_ID >= 50399
		DBG_RETURN(orig_mysqlnd_conn_methods->change_user(proxy_conn, user, passwd, db, silent, passwd_len TSRMLS_CC));
#else
		DBG_RETURN(orig_mysqlnd_conn_methods->change_user(proxy_conn, user, passwd, db, silent TSRMLS_CC));
#endif
	}
	/* search the list of easy handles hanging off the multi-handle */
	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(&(*conn_data_pp)->master_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(&(*conn_data_pp)->master_connections, &pos))
	{
		if (PASS != orig_mysqlnd_conn_methods->change_user(el->conn, user, passwd, db, silent
#if PHP_VERSION_ID >= 50399
															,passwd_len
#endif
															TSRMLS_CC))
		{
			ret = FAIL;
		}
	}

	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(&(*conn_data_pp)->slave_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(&(*conn_data_pp)->slave_connections, &pos))
	{
		if (PASS != orig_mysqlnd_conn_methods->change_user(el->conn, user, passwd, db, silent
#if PHP_VERSION_ID >= 50399
															,passwd_len
#endif
															TSRMLS_CC))
		{
			ret = FAIL;
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::ping */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, ping)(MYSQLND * const proxy_conn TSRMLS_DC)
{
	enum_func_status ret;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	DBG_ENTER("mysqlnd_ms::ping");
	ret = orig_mysqlnd_conn_methods->ping(conn TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::kill */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, kill)(MYSQLND * proxy_conn, unsigned int pid TSRMLS_DC)
{
	enum_func_status ret;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	DBG_ENTER("mysqlnd_ms::kill");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, (conn)->m->get_thread_id(conn TSRMLS_CC));
	ret = orig_mysqlnd_conn_methods->kill_connection(conn, pid TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::select_db */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, select_db)(MYSQLND * const proxy_conn, const char * const db, unsigned int db_len TSRMLS_DC)
{
	enum_func_status ret = PASS;
	zend_llist_position	pos;
	MYSQLND_MS_LIST_DATA * el;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	DBG_ENTER("mysqlnd_ms::select_db");
	if (!conn_data_pp || !*conn_data_pp) {
		DBG_RETURN(orig_mysqlnd_conn_methods->select_db(proxy_conn, db, db_len TSRMLS_CC));
	}
	/* search the list of easy handles hanging off the multi-handle */
	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(&(*conn_data_pp)->master_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(&(*conn_data_pp)->master_connections, &pos))
	{
		if (PASS != orig_mysqlnd_conn_methods->select_db(el->conn, db, db_len TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(&(*conn_data_pp)->slave_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(&(*conn_data_pp)->slave_connections, &pos))
	{
		if (PASS != orig_mysqlnd_conn_methods->select_db(el->conn, db, db_len TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::set_charset */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, set_charset)(MYSQLND * const proxy_conn, const char * const csname TSRMLS_DC)
{
	enum_func_status ret = PASS;
	zend_llist_position	pos;
	MYSQLND_MS_LIST_DATA * el;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	DBG_ENTER("mysqlnd_ms::set_charset");
	if (!conn_data_pp || !*conn_data_pp) {
		DBG_RETURN(orig_mysqlnd_conn_methods->set_charset(proxy_conn, csname TSRMLS_CC));
	}
	/* search the list of easy handles hanging off the multi-handle */
	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(&(*conn_data_pp)->master_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(&(*conn_data_pp)->master_connections, &pos))
	{
		if (PASS != orig_mysqlnd_conn_methods->set_charset(el->conn, csname TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(&(*conn_data_pp)->slave_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(&(*conn_data_pp)->slave_connections, &pos))
	{
		if (PASS != orig_mysqlnd_conn_methods->set_charset(el->conn, csname TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::set_server_option */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, set_server_option)(MYSQLND * const proxy_conn, enum_mysqlnd_server_option option TSRMLS_DC)
{
	enum_func_status ret = PASS;
	zend_llist_position	pos;
	MYSQLND_MS_LIST_DATA * el;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	DBG_ENTER("mysqlnd_ms::set_server_option");
	if (!conn_data_pp || !*conn_data_pp) {
		DBG_RETURN(orig_mysqlnd_conn_methods->set_server_option(proxy_conn, option TSRMLS_CC));
	}
	/* search the list of easy handles hanging off the multi-handle */
	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(&(*conn_data_pp)->master_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(&(*conn_data_pp)->master_connections, &pos))
	{
		if (PASS != orig_mysqlnd_conn_methods->set_server_option(el->conn, option TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(&(*conn_data_pp)->slave_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(&(*conn_data_pp)->slave_connections, &pos))
	{
		if (PASS != orig_mysqlnd_conn_methods->set_server_option(el->conn, option TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::set_client_option */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, set_client_option)(MYSQLND * const proxy_conn, enum_mysqlnd_option option, const char * const value TSRMLS_DC)
{
	enum_func_status ret = PASS;
	zend_llist_position	pos;
	MYSQLND_MS_LIST_DATA * el;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	DBG_ENTER("mysqlnd_ms::set_server_option");
	if (!conn_data_pp || !*conn_data_pp) {
		DBG_RETURN(orig_mysqlnd_conn_methods->set_client_option(proxy_conn, option, value TSRMLS_CC));
	}
	/* search the list of easy handles hanging off the multi-handle */
	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(&(*conn_data_pp)->master_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(&(*conn_data_pp)->master_connections, &pos))
	{
		if (PASS != orig_mysqlnd_conn_methods->set_client_option(el->conn, option, value TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(&(*conn_data_pp)->slave_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(&(*conn_data_pp)->slave_connections, &pos))
	{
		if (PASS != orig_mysqlnd_conn_methods->set_client_option(el->conn, option, value TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::next_result */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, next_result)(MYSQLND * const proxy_conn TSRMLS_DC)
{
	enum_func_status ret;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	DBG_ENTER("mysqlnd_ms::next_result");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, (conn)->m->get_thread_id(conn TSRMLS_CC));
	ret = orig_mysqlnd_conn_methods->next_result(conn TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::more_results */
static zend_bool
MYSQLND_METHOD(mysqlnd_ms, more_results)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	zend_bool ret;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	DBG_ENTER("mysqlnd_ms::more_results");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, (conn)->m->get_thread_id(conn TSRMLS_CC));
	ret = orig_mysqlnd_conn_methods->more_results(conn TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::errno */
static unsigned int
MYSQLND_METHOD(mysqlnd_ms, errno)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->error_info.error_no;
}
/* }}} */


/* {{{ mysqlnd_ms::error */
static const char *
MYSQLND_METHOD(mysqlnd_ms, error)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->error_info.error;
}
/* }}} */


/* {{{ mysqlnd_conn::sqlstate */
static const char *
MYSQLND_METHOD(mysqlnd_ms, sqlstate)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->error_info.sqlstate[0] ? conn->error_info.sqlstate:MYSQLND_SQLSTATE_NULL;
}
/* }}} */


/* {{{ mysqlnd_ms::field_count */
static unsigned int
MYSQLND_METHOD(mysqlnd_ms, field_count)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->field_count;
}
/* }}} */


/* {{{ mysqlnd_conn::thread_id */
static uint64_t
MYSQLND_METHOD(mysqlnd_ms, thread_id)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->thread_id;
}
/* }}} */


/* {{{ mysqlnd_ms::insert_id */
static uint64_t
MYSQLND_METHOD(mysqlnd_ms, insert_id)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->upsert_status.last_insert_id;
}
/* }}} */


/* {{{ mysqlnd_ms::affected_rows */
static uint64_t
MYSQLND_METHOD(mysqlnd_ms, affected_rows)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->upsert_status.affected_rows;
}
/* }}} */


/* {{{ mysqlnd_ms::warning_count */
static unsigned int
MYSQLND_METHOD(mysqlnd_ms, warning_count)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->upsert_status.warning_count;
}
/* }}} */


/* {{{ mysqlnd_ms::info */
static const char *
MYSQLND_METHOD(mysqlnd_ms, info)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->last_message;
}
/* }}} */


#if MYSQLND_VERSION_ID >= 50009
/* {{{ MYSQLND_METHOD(mysqlnd_ms, set_autocommit) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, set_autocommit)(MYSQLND * proxy_conn, unsigned int mode TSRMLS_DC)
{
	enum_func_status ret = PASS;
	zend_llist_position	pos;
	MYSQLND_MS_LIST_DATA * el;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	DBG_ENTER("mysqlnd_ms::set_autocommit");
	if (!conn_data_pp || !*conn_data_pp) {
		DBG_RETURN(orig_mysqlnd_conn_methods->set_autocommit(proxy_conn, mode TSRMLS_CC));
	}
	/* search the list of easy handles hanging off the multi-handle */
	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(&(*conn_data_pp)->master_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(&(*conn_data_pp)->master_connections, &pos))
	{
		/* lazy connection ? */
		if ((CONN_GET_STATE(el->conn) != CONN_ALLOCED) &&
			(PASS != orig_mysqlnd_conn_methods->set_autocommit(el->conn, mode TSRMLS_CC))) {
			ret = FAIL;
		}

	}

	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(&(*conn_data_pp)->slave_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(&(*conn_data_pp)->slave_connections, &pos))
	{
		/* lazy connection ? */
		if ((CONN_GET_STATE(el->conn) != CONN_ALLOCED) &&
			(PASS != orig_mysqlnd_conn_methods->set_autocommit(el->conn, mode TSRMLS_CC))) {
			ret = FAIL;
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ MYSQLND_METHOD(mysqlnd_ms, tx_commit) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, tx_commit)(MYSQLND * conn TSRMLS_DC)
{
	enum_func_status ret;
	DBG_ENTER("mysqlnd_ms::tx_commit");
	ret = orig_mysqlnd_conn_methods->tx_commit(conn TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ MYSQLND_METHOD(mysqlnd_ms, tx_rollback) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, tx_rollback)(MYSQLND * conn TSRMLS_DC)
{
	enum_func_status ret;
	DBG_ENTER("mysqlnd_ms::tx_rollback");
	ret = orig_mysqlnd_conn_methods->tx_rollback(conn TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */
#endif


/* {{{ mysqlnd_ms_register_hooks*/
void
mysqlnd_ms_register_hooks()
{
	orig_mysqlnd_conn_methods = mysqlnd_conn_get_methods();

	memcpy(&my_mysqlnd_conn_methods, orig_mysqlnd_conn_methods, sizeof(struct st_mysqlnd_conn_methods));

	my_mysqlnd_conn_methods.connect				= MYSQLND_METHOD(mysqlnd_ms, connect);
	my_mysqlnd_conn_methods.query				= MYSQLND_METHOD(mysqlnd_ms, query);
	my_mysqlnd_conn_methods.use_result			= MYSQLND_METHOD(mysqlnd_ms, use_result);
	my_mysqlnd_conn_methods.store_result		= MYSQLND_METHOD(mysqlnd_ms, store_result);
	my_mysqlnd_conn_methods.free_contents		= MYSQLND_METHOD(mysqlnd_ms, free_contents);
	my_mysqlnd_conn_methods.escape_string		= MYSQLND_METHOD(mysqlnd_ms, escape_string);
	my_mysqlnd_conn_methods.change_user			= MYSQLND_METHOD(mysqlnd_ms, change_user);
	my_mysqlnd_conn_methods.ping				= MYSQLND_METHOD(mysqlnd_ms, ping);
	my_mysqlnd_conn_methods.kill_connection		= MYSQLND_METHOD(mysqlnd_ms, kill);
	my_mysqlnd_conn_methods.select_db			= MYSQLND_METHOD(mysqlnd_ms, select_db);
	my_mysqlnd_conn_methods.set_charset			= MYSQLND_METHOD(mysqlnd_ms, set_charset);
	my_mysqlnd_conn_methods.set_server_option	= MYSQLND_METHOD(mysqlnd_ms, set_server_option);
	my_mysqlnd_conn_methods.set_client_option	= MYSQLND_METHOD(mysqlnd_ms, set_client_option);
	my_mysqlnd_conn_methods.next_result			= MYSQLND_METHOD(mysqlnd_ms, next_result);
	my_mysqlnd_conn_methods.more_results		= MYSQLND_METHOD(mysqlnd_ms, more_results);
	my_mysqlnd_conn_methods.get_error_no		= MYSQLND_METHOD(mysqlnd_ms, errno);
	my_mysqlnd_conn_methods.get_error_str		= MYSQLND_METHOD(mysqlnd_ms, error);
	my_mysqlnd_conn_methods.get_sqlstate		= MYSQLND_METHOD(mysqlnd_ms, sqlstate);


	my_mysqlnd_conn_methods.get_thread_id		= MYSQLND_METHOD(mysqlnd_ms, thread_id);
	my_mysqlnd_conn_methods.get_field_count		= MYSQLND_METHOD(mysqlnd_ms, field_count);
	my_mysqlnd_conn_methods.get_last_insert_id	= MYSQLND_METHOD(mysqlnd_ms, insert_id);
	my_mysqlnd_conn_methods.get_affected_rows	= MYSQLND_METHOD(mysqlnd_ms, affected_rows);
	my_mysqlnd_conn_methods.get_warning_count	= MYSQLND_METHOD(mysqlnd_ms, warning_count);
	my_mysqlnd_conn_methods.get_last_message	= MYSQLND_METHOD(mysqlnd_ms, info);
#if MYSQLND_VERSION_ID >= 50009
	my_mysqlnd_conn_methods.set_autocommit		= MYSQLND_METHOD(mysqlnd_ms, set_autocommit);
	my_mysqlnd_conn_methods.tx_commit			= MYSQLND_METHOD(mysqlnd_ms, tx_commit);
	my_mysqlnd_conn_methods.tx_rollback			= MYSQLND_METHOD(mysqlnd_ms, tx_rollback);
#endif
	mysqlnd_conn_set_methods(&my_mysqlnd_conn_methods);
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
