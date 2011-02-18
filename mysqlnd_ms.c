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
#include "mysqlnd_ms.h"
#include "mysqlnd_ms_ini.h"
#include "ext/standard/php_rand.h"

#define MYSQLND_MS_VERSION "1.0.0-prototype"
#define MYSQLND_MS_VERSION_ID 10000

#define MYSLQND_MS_HOTLOADING FALSE

#define MASTER_NAME		"master"
#define SLAVE_NAME		"slave"
#define PICK_NAME		"pick"
#define PICK_RANDOM		"random"
#define PICK_RROBIN		"roundrobin"
#define PICK_USER		"user"

ZEND_DECLARE_MODULE_GLOBALS(mysqlnd_ms)

static unsigned int mysqlnd_ms_plugin_id;

static struct st_mysqlnd_conn_methods my_mysqlnd_conn_methods;
static struct st_mysqlnd_conn_methods *orig_mysqlnd_conn_methods;

static zend_bool mysqlns_ms_global_config_loaded = FALSE;
static HashTable mysqlnd_ms_config;

static MYSQLND * mysqlnd_ms_choose_connection_rr(MYSQLND * conn, const char * const query, const size_t query_len TSRMLS_DC);
static MYSQLND * mysqlnd_ms_choose_connection_random(MYSQLND * conn, const char * const query, const size_t query_len TSRMLS_DC);
static void mysqlnd_ms_conn_free_plugin_data(MYSQLND *conn TSRMLS_DC);


enum mysqlnd_ms_server_pick_strategy
{
	SERVER_PICK_RROBIN,
	SERVER_PICK_RANDOM,
	SERVER_PICK_USER
};

typedef struct st_mysqlnd_ms_connection_data
{
	char * connect_host;
	zend_llist master_connections;
	zend_llist slave_connections;
	MYSQLND * last_used_connection;
	enum mysqlnd_ms_server_pick_strategy pick_strategy;
	enum mysqlnd_ms_server_pick_strategy fallback_pick_strategy;

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
	zend_bool persistent;
} MYSQLND_MS_LIST_DATA;

#define MYSQLND_MS_ERROR_PREFIX "(mysqlnd_ms)"

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
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Failed to call '%s'", MYSQLND_MS_ERROR_PREFIX, Z_STRVAL_P(func));
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
mysqlnd_ms_user_pick_server(MYSQLND * conn, const char * query, size_t query_len TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	zend_llist * master_list = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->master_connections : NULL;
	zend_llist * slave_list = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->slave_connections : NULL;
	zval * args[5];
	zval * retval;
	MYSQLND * ret = NULL;

	DBG_ENTER("mysqlnd_ms_user_pick_server");
	DBG_INF_FMT("query(50bytes)=%*s query_is_select=%p", MIN(50, query_len), query, MYSQLND_MS_G(user_pick_server));

	if (master_list && MYSQLND_MS_G(user_pick_server)) {
		uint param = 0;
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
				add_next_index_stringl(args[param], el->conn->scheme, el->conn->scheme_len, 1);
			}

			/* slave list*/
			param++;
			MS_ARRAY(args[param]);
			if (slave_list) {
				for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(slave_list, &pos); el && el->conn;
						el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(slave_list, &pos))
				{
					add_next_index_stringl(args[param], el->conn->scheme, el->conn->scheme_len, 1);
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
		}

		retval = mysqlnd_ms_call_handler(MYSQLND_MS_G(user_pick_server), param + 1, args, TRUE TSRMLS_CC);
		if (retval) {
			if (Z_TYPE_P(retval) == IS_STRING) {
				do {
					MYSQLND_MS_LIST_DATA * el;
					zend_llist_position	pos;

					for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(master_list, &pos); !ret && el && el->conn;
							el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(master_list, &pos))
					{
						if (!strncasecmp(el->conn->scheme, Z_STRVAL_P(retval), MIN(Z_STRLEN_P(retval), el->conn->scheme_len))) {
							ret = el->conn;
							DBG_INF_FMT("Userfunc chose master host : [%*s]", el->conn->scheme_len, el->conn->scheme);
						}
					}
					if (slave_list) {
						for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(slave_list, &pos); !ret && el && el->conn;
								el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(slave_list, &pos))
						{
							if (!strncasecmp(el->conn->scheme, Z_STRVAL_P(retval), MIN(Z_STRLEN_P(retval), el->conn->scheme_len))) {
								ret = el->conn;
								DBG_INF_FMT("Userfunc chose slave host : [%*s]", el->conn->scheme_len, el->conn->scheme);
							}
						}
					}
				} while (0);
			}
			zval_ptr_dtor(&retval);
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
	element->persistent = FALSE;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_ini_string_is_bool_true */
static zend_bool
mysqlnd_ms_ini_string_is_bool_true(const char * value)
{
	if (!value) {
		return FALSE;
	}
	if (!strncmp("1", value, sizeof("1") - 1)) {
		return TRUE;
	}
	if (!strncasecmp("true", value, sizeof("true") - 1)) {
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
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Exclusive usage of configuration enforced but did not find the correct INI file section (%s)", host);
		if (hotloading) {
			MYSQLND_MS_CONFIG_UNLOCK;
		}
		DBG_RETURN(FAIL);
	}
	mysqlnd_ms_conn_free_plugin_data(conn TSRMLS_CC);

	conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	*conn_data_pp = mnd_pecalloc(1, sizeof(MYSQLND_MS_CONNECTION_DATA), conn->persistent);
	zend_llist_init(&(*conn_data_pp)->master_connections, sizeof(MYSQLND_MS_LIST_DATA), (llist_dtor_func_t) mysqlnd_ms_conn_list_dtor, conn->persistent);
	zend_llist_init(&(*conn_data_pp)->slave_connections, sizeof(MYSQLND_MS_LIST_DATA), (llist_dtor_func_t) mysqlnd_ms_conn_list_dtor, conn->persistent);

	if (FALSE == section_found) {
		ret = orig_mysqlnd_conn_methods->connect(conn, host, user, passwd, passwd_len, db, db_len, port, socket, mysql_flags TSRMLS_CC);
		if (ret == PASS) {
			MYSQLND_MS_LIST_DATA new_element;
			new_element.conn = conn;
			new_element.host = mnd_pestrdup(host, conn->persistent);
			new_element.persistent = conn->persistent;
			zend_llist_add_element(&(*conn_data_pp)->master_connections, &new_element);
		}
	} else {
		do {
			zend_bool value_exists = FALSE, is_list_value = FALSE, use_lazy_connections = FALSE, use_lazy_connections_list_value = FALSE;
			char * lazy_connections;
			/* create master connection */
			char * master = mysqlnd_ms_ini_string(&mysqlnd_ms_config, host, host_len, MASTER_NAME, sizeof(MASTER_NAME) - 1,
												  &value_exists, &is_list_value, hotloading? FALSE:TRUE TSRMLS_CC);
			if (FALSE == value_exists) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot find master section in config");
				break;
			}

			lazy_connections = mysqlnd_ms_ini_string(&mysqlnd_ms_config, host, host_len, MASTER_NAME, sizeof(MASTER_NAME) - 1,
													 &use_lazy_connections, &use_lazy_connections_list_value, hotloading? FALSE:TRUE TSRMLS_CC);

			use_lazy_connections = use_lazy_connections && mysqlnd_ms_ini_string_is_bool_true(lazy_connections);
			if (lazy_connections) {
				mnd_efree(lazy_connections);
				lazy_connections = NULL;
			}

			ret = orig_mysqlnd_conn_methods->connect(conn, master, user, passwd, passwd_len, db, db_len, port, socket, mysql_flags TSRMLS_CC);
			if (ret != PASS) {
				mnd_efree(master);
				break;
			} else {
				MYSQLND_MS_LIST_DATA new_element;
				new_element.conn = conn;
				new_element.host = mnd_pestrdup(master, conn->persistent);
				new_element.persistent = conn->persistent;
				zend_llist_add_element(&(*conn_data_pp)->master_connections, &new_element);
			}
			mnd_efree(master);
			DBG_INF_FMT("Master connection "MYSQLND_LLU_SPEC" established", conn->m->get_thread_id(conn TSRMLS_CC));

			/* More master connections ? */
			if (is_list_value) {
				DBG_INF("We have more master connections. Connect...");
				do {
					master = mysqlnd_ms_ini_string(&mysqlnd_ms_config, host, host_len, MASTER_NAME, sizeof(MASTER_NAME) - 1,
												   &value_exists, &is_list_value, hotloading? FALSE:TRUE TSRMLS_CC);
					DBG_INF_FMT("value_exists=%d master=%s", value_exists, master);
					if (value_exists && master) {
						MYSQLND * tmp_conn = mysqlnd_init(conn->persistent);
						ret = orig_mysqlnd_conn_methods->connect(tmp_conn, master, user, passwd, passwd_len, db, db_len, port, socket, mysql_flags TSRMLS_CC);

						if (ret == PASS) {
							MYSQLND_MS_LIST_DATA new_element;
							new_element.conn = tmp_conn;
							new_element.host = mnd_pestrdup(master, conn->persistent);
							new_element.persistent = conn->persistent;
							zend_llist_add_element(&(*conn_data_pp)->master_connections, &new_element);
							DBG_INF_FMT("Further master connection "MYSQLND_LLU_SPEC" established", tmp_conn->m->get_thread_id(tmp_conn TSRMLS_CC));
						} else {
							tmp_conn->m->dtor(tmp_conn TSRMLS_CC);
						}
						mnd_efree(master);
						master = NULL;
					}
				} while (value_exists);
			}

			/* create slave slave_connections */
			do {
				char * slave = mysqlnd_ms_ini_string(&mysqlnd_ms_config, host, host_len, SLAVE_NAME, sizeof(SLAVE_NAME) - 1,
													 &value_exists, &is_list_value, hotloading? FALSE:TRUE TSRMLS_CC);
				if (value_exists && is_list_value && slave) {
					MYSQLND * tmp_conn = mysqlnd_init(conn->persistent);
					ret = orig_mysqlnd_conn_methods->connect(tmp_conn, slave, user, passwd, passwd_len, db, db_len, port, socket, mysql_flags TSRMLS_CC);

					if (ret == PASS) {
						MYSQLND_MS_LIST_DATA new_element;
						new_element.conn = tmp_conn;
						new_element.host = mnd_pestrdup(slave, conn->persistent);
						new_element.persistent = conn->persistent;
						zend_llist_add_element(&(*conn_data_pp)->slave_connections, &new_element);
						DBG_INF_FMT("Slave connection "MYSQLND_LLU_SPEC" established", tmp_conn->m->get_thread_id(tmp_conn TSRMLS_CC));
					} else {
						tmp_conn->m->dtor(tmp_conn TSRMLS_CC);
					}
					mnd_efree(slave);
					slave = NULL;
				}
			} while (value_exists);

			{
				char * pick_strategy = mysqlnd_ms_ini_string(&mysqlnd_ms_config, host, host_len, PICK_NAME, sizeof(PICK_NAME) - 1,
												  				&value_exists, &is_list_value, hotloading? FALSE:TRUE TSRMLS_CC);
				(*conn_data_pp)->pick_strategy = (*conn_data_pp)->fallback_pick_strategy = SERVER_PICK_RROBIN;

				if (value_exists && pick_strategy) {
					if (!strncasecmp(PICK_RANDOM, pick_strategy, sizeof(PICK_RANDOM) - 1)) {
						(*conn_data_pp)->pick_strategy = (*conn_data_pp)->fallback_pick_strategy = SERVER_PICK_RANDOM;
					} else if (!strncasecmp(PICK_USER, pick_strategy, sizeof(PICK_USER) - 1)) {
						(*conn_data_pp)->pick_strategy = SERVER_PICK_USER;
						if (is_list_value) {
							mnd_efree(pick_strategy);
							pick_strategy = mysqlnd_ms_ini_string(&mysqlnd_ms_config, host, host_len, PICK_NAME, sizeof(PICK_NAME) - 1,
												  				&value_exists, &is_list_value, hotloading? FALSE:TRUE TSRMLS_CC);
							if (pick_strategy) {
								if (!strncasecmp(PICK_RANDOM, pick_strategy, sizeof(PICK_RANDOM) - 1)) {
									(*conn_data_pp)->fallback_pick_strategy = SERVER_PICK_RANDOM;
								}
							}
						}
					}
				}
				if (pick_strategy) {
					mnd_efree(pick_strategy);
				}
			}
		} while (0);
	}
	if (hotloading) {
		MYSQLND_MS_CONFIG_UNLOCK;
	}
	if (ret == PASS) {
		(*conn_data_pp)->connect_host = mnd_pestrdup(host, conn->persistent);
	}
	DBG_RETURN(ret);
}
/* }}} */


#define MASTER_SWITCH "ms=master"
#define SLAVE_SWITCH "ms=slave"
#define LAST_USED_SWITCH "ms=last_used"

enum enum_which_server
{
	USE_MASTER,
	USE_SLAVE,
	USE_LAST_USED
};


/* {{{ mysqlnd_ms_query_is_select */
static enum enum_which_server
mysqlnd_ms_query_is_select(const char * query, size_t query_len TSRMLS_DC)
{
	enum enum_which_server ret = USE_MASTER;
	struct st_qc_token_and_value token;
	zend_bool forced = FALSE;
	struct st_mysqlnd_tok_scanner * scanner;
	DBG_ENTER("mysqlnd_ms_query_is_select");
	if (!query) {
		DBG_RETURN(USE_MASTER);
	}
	scanner = mysqlnd_tok_create_scanner(query, query_len TSRMLS_CC);
	token = mysqlnd_tok_get_token(scanner TSRMLS_CC);
	while (token.token == QC_TOKEN_COMMENT) {
		if (!strncasecmp(Z_STRVAL(token.value), MASTER_SWITCH, sizeof(MASTER_SWITCH) - 1)) {
			DBG_INF("forced master");
			ret = USE_MASTER;
			forced = TRUE;
		}
		if (!strncasecmp(Z_STRVAL(token.value), SLAVE_SWITCH, sizeof(SLAVE_SWITCH) - 1)) {
			DBG_INF("forced slave");
			ret = USE_SLAVE;
			forced = TRUE;
		}
		if (!strncasecmp(Z_STRVAL(token.value), LAST_USED_SWITCH, sizeof(LAST_USED_SWITCH) - 1)) {
			DBG_INF("forced last used");
			ret = USE_LAST_USED;
			forced = TRUE;
		}
		zval_dtor(&token.value);
		token = mysqlnd_tok_get_token(scanner TSRMLS_CC);
	}
	if (forced == FALSE && token.token == QC_TOKEN_SELECT) {
		ret = USE_SLAVE;
	}
	zval_dtor(&token.value);
	mysqlnd_tok_free_scanner(scanner TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_choose_connection_rr */
static MYSQLND *
mysqlnd_ms_choose_connection_rr(MYSQLND * conn, const char * const query, const size_t query_len TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	DBG_ENTER("mysqlnd_ms_choose_connection_rr");

	if (!conn_data_pp || !*conn_data_pp) {
		DBG_RETURN(conn);
	}
	switch (mysqlnd_ms_query_is_select(query, query_len TSRMLS_CC)) {
		case USE_SLAVE:
		{
			zend_llist * l = &(*conn_data_pp)->slave_connections;
			MYSQLND_MS_LIST_DATA * tmp = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next(l);
			MYSQLND * element = (tmp && tmp->conn)? tmp->conn : (((tmp = zend_llist_get_first(l)) && tmp->conn)? tmp->conn : NULL);
			if (element) {
				DBG_INF("Using slave connection");
				DBG_RETURN(element);
			}
		}
		/* fall-through */
		case USE_MASTER:
		{
			zend_llist * l = &(*conn_data_pp)->master_connections;
			MYSQLND_MS_LIST_DATA * tmp = zend_llist_get_next(l);
			MYSQLND * element = (tmp && tmp->conn)? tmp->conn : (((tmp = zend_llist_get_first(l)) && tmp->conn)? tmp->conn : NULL);
			DBG_INF("Using master connection");
			if (element) {
				DBG_RETURN(element);
			}
		}
		/* fall-through */
		case USE_LAST_USED:
			DBG_INF("Using last used connection");
			DBG_RETURN((*conn_data_pp)->last_used_connection);
		default:
			/* error */
			DBG_RETURN(conn);
			break;
	}
}
/* }}} */


/* {{{ mysqlnd_ms_choose_connection_random */
static MYSQLND *
mysqlnd_ms_choose_connection_random(MYSQLND * conn, const char * const query, const size_t query_len TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	DBG_ENTER("mysqlnd_ms_choose_connection_random");

	if (!conn_data_pp || !*conn_data_pp) {
		DBG_RETURN(conn);
	}
	switch (mysqlnd_ms_query_is_select(query, query_len TSRMLS_CC)) {
		case USE_SLAVE:
		{
			zend_llist_position	pos;
			zend_llist * l = &(*conn_data_pp)->slave_connections;
			MYSQLND_MS_LIST_DATA * element;
			unsigned long rnd_idx;
			uint i = 0;

			rnd_idx = php_rand(TSRMLS_C);
			RAND_RANGE(rnd_idx, 0, zend_llist_count(l) - 1, PHP_RAND_MAX);
			DBG_INF_FMT("USE_SLAVE rnd_idx=%lu", rnd_idx);

			element = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(l, &pos);
			while (i++ < rnd_idx) {
				element = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(l, &pos);
			}
			if (element && element->conn) {
				DBG_INF_FMT("Using slave connection "MYSQLND_LLU_SPEC"", element->conn->thread_id);
				DBG_RETURN(element->conn);
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

			rnd_idx = php_rand(TSRMLS_C);
			RAND_RANGE(rnd_idx, 0, zend_llist_count(l) - 1, PHP_RAND_MAX);
			DBG_INF_FMT("USE_MASTER rnd_idx=%lu", rnd_idx);

			element = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(l, &pos);
			while (i++ < rnd_idx) {
				element = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(l, &pos);
			}
			if (element && element->conn) {
				DBG_INF_FMT("Using master connection "MYSQLND_LLU_SPEC"", element->conn->thread_id);
				DBG_RETURN(element->conn);
			}
		}
		/* fall-through */
		case USE_LAST_USED:
			DBG_INF("Using last used connection");
			DBG_RETURN((*conn_data_pp)->last_used_connection);
		default:
			/* error */
			DBG_RETURN(conn);
			break;
	}
}
/* }}} */

/* {{{ mysqlnd_ms_pick_server */
static MYSQLND *
mysqlnd_ms_pick_server(MYSQLND * conn, const char * const query, const size_t query_len TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	MYSQLND * connection = conn;
	DBG_ENTER("mysqlnd_ms_pick_server");

	if (conn_data_pp && *conn_data_pp) {
		enum mysqlnd_ms_server_pick_strategy strategy = (*conn_data_pp)->pick_strategy;
		connection = NULL;
		if (SERVER_PICK_USER == strategy) {
			connection = mysqlnd_ms_user_pick_server(conn, query, query_len TSRMLS_CC);
			if (!connection) {
				strategy = (*conn_data_pp)->fallback_pick_strategy;
			}
		}
		if (!connection) {
			if (SERVER_PICK_RANDOM == strategy) {
				connection = mysqlnd_ms_choose_connection_random(conn, query, query_len TSRMLS_CC);
			} else {
				connection = mysqlnd_ms_choose_connection_rr(conn, query, query_len TSRMLS_CC);
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


/* {{{ MYSQLND_METHOD(mysqlnd_ms, query) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, query)(MYSQLND * conn, const char *query, unsigned int query_len TSRMLS_DC)
{
	MYSQLND * connection;
	enum_func_status ret;
	DBG_ENTER("mysqlnd_ms::query");

	connection = mysqlnd_ms_pick_server(conn, query, query_len TSRMLS_CC);

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


/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("mysqlnd_ms.enable", "0", PHP_INI_SYSTEM, OnUpdateBool, enable, zend_mysqlnd_ms_globals, mysqlnd_ms_globals)
	STD_PHP_INI_ENTRY("mysqlnd_ms.force_config_usage", "0", PHP_INI_SYSTEM, OnUpdateBool, force_config_usage, zend_mysqlnd_ms_globals, mysqlnd_ms_globals)
	STD_PHP_INI_ENTRY("mysqlnd_ms.ini_file", NULL, PHP_INI_SYSTEM, OnUpdateString, ini_file, zend_mysqlnd_ms_globals, mysqlnd_ms_globals)
PHP_INI_END()
/* }}} */


/* {{{ mysqlnd_ms_register_hooks*/
static void
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

	mysqlnd_conn_set_methods(&my_mysqlnd_conn_methods);
}
/* }}} */


/* {{{ php_mysqlnd_ms_init_globals */
static void
php_mysqlnd_ms_init_globals(zend_mysqlnd_ms_globals * mysqlnd_ms_globals)
{
	mysqlnd_ms_globals->enable = FALSE;
	mysqlnd_ms_globals->force_config_usage = FALSE;
	mysqlnd_ms_globals->ini_file = NULL;
	mysqlnd_ms_globals->user_pick_server = NULL;
}
/* }}} */


/* {{{ PHP_GINIT_FUNCTION */
static PHP_GINIT_FUNCTION(mysqlnd_ms)
{
	php_mysqlnd_ms_init_globals(mysqlnd_ms_globals);
}
/* }}} */


/* {{{ mysqlnd_ms_config_dtor */
static void
mysqlnd_ms_config_dtor(void * data)
{
	TSRMLS_FETCH();
	zend_hash_destroy(*(HashTable **)data);
	mnd_free(*(HashTable **)data);
}
/* }}} */


/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(mysqlnd_ms)
{
	if (MYSQLND_MS_G(enable)) {
		MYSQLND_MS_CONFIG_LOCK;
		if (FALSE == mysqlns_ms_global_config_loaded) {
			mysqlnd_ms_init_server_list(&mysqlnd_ms_config TSRMLS_CC);
			mysqlns_ms_global_config_loaded = TRUE;
		}
		MYSQLND_MS_CONFIG_UNLOCK;
	}
	return SUCCESS;
}
/* }}} */


/* {{{ PHP_RSHUTDOWN_FUNCTION */
PHP_RSHUTDOWN_FUNCTION(mysqlnd_ms)
{
	if (MYSQLND_MS_G(user_pick_server)) {
		zval_ptr_dtor(&MYSQLND_MS_G(user_pick_server));
		MYSQLND_MS_G(user_pick_server) = NULL;
	}
	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(mysqlnd_ms)
{
	ZEND_INIT_MODULE_GLOBALS(mysqlnd_ms, php_mysqlnd_ms_init_globals, NULL);
	REGISTER_INI_ENTRIES();

	if (MYSQLND_MS_G(enable)) {
		mysqlnd_ms_plugin_id = mysqlnd_plugin_register();
		mysqlnd_ms_register_hooks();
		zend_hash_init(&mysqlnd_ms_config, 2, NULL /* hash_func */, mysqlnd_ms_config_dtor /*dtor*/, 1 /* persistent */);
#ifdef ZTS
		LOCK_global_config_access = tsrm_mutex_alloc();
#endif
	}

	REGISTER_STRING_CONSTANT("MYSQLND_MS_VERSION", MYSQLND_MS_VERSION, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLND_MS_VERSION_ID", MYSQLND_MS_VERSION_ID, CONST_CS | CONST_PERSISTENT);

	REGISTER_STRING_CONSTANT("MYSQLND_MS_MASTER_SWITCH", MASTER_SWITCH, CONST_CS | CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("MYSQLND_MS_SLAVE_SWITCH", SLAVE_SWITCH, CONST_CS | CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("MYSQLND_MS_LAST_USED_SWITCH", LAST_USED_SWITCH, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("MYSQLND_MS_QUERY_USE_MASTER", USE_MASTER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLND_MS_QUERY_USE_SLAVE", USE_SLAVE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLND_MS_QUERY_USE_LAST_USED", USE_LAST_USED, CONST_CS | CONST_PERSISTENT);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(mysqlnd_ms)
{
	UNREGISTER_INI_ENTRIES();
	if (MYSQLND_MS_G(enable)) {
		zend_hash_destroy(&mysqlnd_ms_config);
#ifdef ZTS
		tsrm_mutex_free(LOCK_global_config_access);
#endif
	}
	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(mysqlnd_ms)
{
	char buf[64];

	php_info_print_table_start();
	php_info_print_table_header(2, "mysqlnd_ms support", "enabled");
	snprintf(buf, sizeof(buf), "%s (%d)", MYSQLND_MS_VERSION, MYSQLND_MS_VERSION_ID);
	php_info_print_table_row(2, "Mysqlnd master/slave plugin version", buf);
	php_info_print_table_row(2, "Plugin active", MYSQLND_MS_G(enable) ? "yes" : "no");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlnd_ms_set_user_pick_server, 0, 0, 1)
	ZEND_ARG_INFO(0, pick_server_cb)
ZEND_END_ARG_INFO()

/* {{{ proto bool mysqlnd_ms_set_user_pick_server(string is_select)
   Sets use_pick function callback */
static PHP_FUNCTION(mysqlnd_ms_set_user_pick_server)
{
	zval *arg = NULL;
	char *name;

	DBG_ENTER("zif_mysqlnd_ms_set_user_pick_server");

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &arg) == FAILURE) {
		DBG_VOID_RETURN;
	}

	if (!zend_is_callable(arg, 0, &name TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "Argument is not a valid callback");
		efree(name);
		RETVAL_FALSE;
		DBG_VOID_RETURN;
	}
	DBG_INF_FMT("name=%s", name);
	efree(name);

	if (MYSQLND_MS_G(user_pick_server) != NULL) {
		zval_ptr_dtor(&MYSQLND_MS_G(user_pick_server));
	}
	MYSQLND_MS_G(user_pick_server) = arg;
	Z_ADDREF_P(arg);

	RETVAL_TRUE;
	DBG_VOID_RETURN;
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlnd_ms_query_is_select, 0, 0, 1)
	ZEND_ARG_INFO(0, query)
ZEND_END_ARG_INFO()


/* {{{ proto long mysqlnd_ms_query_is_select(string query)
   Parse query and propose where to send it */
static PHP_FUNCTION(mysqlnd_ms_query_is_select)
{
	char * query;
	int query_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &query, &query_len) == FAILURE) {
		return;
	}

	RETURN_LONG(mysqlnd_ms_query_is_select(query, query_len TSRMLS_CC));
}
/* }}} */


/* {{{ mysqlnd_ms_deps[] */
static const zend_module_dep mysqlnd_ms_deps[] = {
	ZEND_MOD_REQUIRED("mysqlnd")
	ZEND_MOD_REQUIRED("standard")
	{NULL, NULL, NULL}
};
/* }}} */


/* {{{ mysqlnd_ms_functions */
static const zend_function_entry mysqlnd_ms_functions[] = {
	PHP_FE(mysqlnd_ms_set_user_pick_server,	arginfo_mysqlnd_ms_set_user_pick_server)
	PHP_FE(mysqlnd_ms_query_is_select,	arginfo_mysqlnd_ms_query_is_select)
	{NULL, NULL, NULL}	/* Must be the last line in mysqlnd_ms_functions[] */
};
/* }}} */


/* {{{ mysqlnd_ms_module_entry */
zend_module_entry mysqlnd_ms_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	mysqlnd_ms_deps,
	"mysqlnd_ms",
	mysqlnd_ms_functions,
	PHP_MINIT(mysqlnd_ms),
	PHP_MSHUTDOWN(mysqlnd_ms),
	PHP_RINIT(mysqlnd_ms),
	PHP_RSHUTDOWN(mysqlnd_ms),
	PHP_MINFO(mysqlnd_ms),
	"0.1",
	PHP_MODULE_GLOBALS(mysqlnd_ms),
	PHP_GINIT(mysqlnd_ms),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_MYSQLND_MS
ZEND_GET_MODULE(mysqlnd_ms)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
