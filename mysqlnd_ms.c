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
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_priv.h"
#include "Zend/zend_ini.h"
#include "Zend/zend_ini_scanner.h"
#include "mysqlnd_ms.h"
#include "mysqlnd_ms_ini.h"

#define MASTER_NAME "master"
#define SLAVE_NAME "slave"

ZEND_DECLARE_MODULE_GLOBALS(mysqlnd_ms)

static unsigned int mysqlnd_ms_plugin_id;

static struct st_mysqlnd_conn_methods my_mysqlnd_conn_methods;
static struct st_mysqlnd_conn_methods *orig_mysqlnd_conn_methods;

static zend_bool mysqlns_ms_global_config_loaded = FALSE;
static HashTable mysqlnd_ms_config;


typedef struct st_mysqlnd_ms_connection_data
{
	zend_llist connections;
	MYSQLND * last_used_connection;
} MYSQLND_MS_CONNECTION_DATA;


/* {{{ mysqlnd_ms_conn_list_dtor */
static void
mysqlnd_ms_conn_list_dtor(void * pDest)
{
	MYSQLND * conn = *(MYSQLND **) pDest;
	TSRMLS_FETCH();
	DBG_ENTER("mysqlnd_ms_conn_list_dtor");
	DBG_INF_FMT("conn=%p", conn);
	mysqlnd_close(conn, TRUE);
	DBG_VOID_RETURN;
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
	size_t host_len = strlen(host);
	zend_bool section_found = mysqlnd_ms_ini_get_section(&mysqlnd_ms_config, host, host_len TSRMLS_CC);

	DBG_ENTER("mysqlnd_ms::connect");
	if (MYSQLND_MS_G(force_config_usage) && FALSE == section_found) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Exclusive usage of configuration enforced but did not find the correct INI file section");
		DBG_RETURN(FAIL);
	}

	if (FALSE == section_found) {
		ret = orig_mysqlnd_conn_methods->connect(conn, host, user, passwd, passwd_len, db, db_len, port, socket, mysql_flags TSRMLS_CC);
		if (ret == PASS) {
			conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
			if (!*conn_data_pp) {
				*conn_data_pp = mnd_pecalloc(1, sizeof(MYSQLND_MS_CONNECTION_DATA), conn->persistent);		
				zend_llist_init(&(*conn_data_pp)->connections, sizeof(MYSQLND *), (llist_dtor_func_t) mysqlnd_ms_conn_list_dtor, conn->persistent);
			}
		}
	} else {
		do {
			zend_bool value_exists = FALSE;
			char * master = mysqlnd_ms_ini_string(&mysqlnd_ms_config, host, host_len, MASTER_NAME, sizeof(MASTER_NAME) - 1, &value_exists TSRMLS_CC);

			ret = orig_mysqlnd_conn_methods->connect(conn, master, user, passwd, passwd_len, db, db_len, port, socket, mysql_flags TSRMLS_CC);
			mnd_efree(master);
			if (ret != PASS) {
				break;
			}
			conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
			if (!*conn_data_pp) {
				*conn_data_pp = mnd_pecalloc(1, sizeof(MYSQLND_MS_CONNECTION_DATA), conn->persistent);		
				zend_llist_init(&(*conn_data_pp)->connections, sizeof(MYSQLND *), (llist_dtor_func_t) mysqlnd_ms_conn_list_dtor, conn->persistent);
			}

			/* create master connection */
			DBG_INF_FMT("Master connection with thread_id "MYSQLND_LLU_SPEC" established", conn->m->get_thread_id(conn TSRMLS_CC));

			/* create slave connections */
			do {
				char * slave = mysqlnd_ms_ini_string(&mysqlnd_ms_config, host, host_len, SLAVE_NAME, sizeof(SLAVE_NAME) - 1, &value_exists TSRMLS_CC);
				if (value_exists && slave) {
					MYSQLND * tmp_conn = mysqlnd_init(conn->persistent);
					ret = orig_mysqlnd_conn_methods->connect(tmp_conn, slave, user, passwd, passwd_len, db, db_len, port, socket, mysql_flags TSRMLS_CC);
					mnd_efree(slave);
					slave = NULL;

					if (ret == PASS) {
						zend_llist_add_element(&(*conn_data_pp)->connections, &tmp_conn);
						DBG_INF_FMT("Slave connection with thread_id "MYSQLND_LLU_SPEC" established", tmp_conn->m->get_thread_id(tmp_conn TSRMLS_CC));
					} else {
						tmp_conn->m->dtor(tmp_conn TSRMLS_CC);
					}				
				}
			} while (value_exists);
		} while (0);	
	}

	DBG_RETURN(ret);
}
/* }}} */


#define MASTER_SWITCH "ms=master"
#define SLAVE_SWITCH "ms=slave"

enum enum_which_server
{
	USE_MASTER,
	USE_SLAVE
};


/* {{{ mysqlnd_ms_query_is_select */
static enum enum_which_server
mysqlnd_ms_query_is_select(const char * query, size_t query_len TSRMLS_DC)
{
	enum enum_which_server ret = USE_MASTER;
	const char * p = query;
	size_t len = query_len;
	struct st_qc_token_and_value token;
	const MYSQLND_CHARSET * cset = mysqlnd_find_charset_name("utf8");
	zend_bool forced = FALSE;

	DBG_ENTER("mysqlnd_ms_query_is_select");
	if (!query) {
		DBG_RETURN(USE_MASTER);
	}

	token = mysqlnd_ms_get_token(&p, &len, cset TSRMLS_CC);
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
		zval_dtor(&token.value);
		token = mysqlnd_ms_get_token(&p, &len, cset TSRMLS_CC);
	}
	if (forced == FALSE && token.token == QC_TOKEN_SELECT) {
		ret = USE_SLAVE;
	}
	zval_dtor(&token.value);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ MYSQLND_METHOD(mysqlnd_ms, query) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, query)(MYSQLND * conn, const char *query, unsigned int query_len TSRMLS_DC)
{
	enum_func_status ret;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	zend_llist * l = &(*conn_data_pp)->connections;
	MYSQLND ** tmp = zend_llist_get_next(l);
	MYSQLND ** element = tmp && *tmp? tmp : zend_llist_get_first(l);
	DBG_ENTER("mysqlnd_ms::query");

	if (mysqlnd_ms_query_is_select(query, query_len TSRMLS_CC) == USE_MASTER || !element || !*element) {
		DBG_INF_FMT("Using master. Thread "MYSQLND_LLU_SPEC, (conn)->m->get_thread_id(conn TSRMLS_CC));
		(*conn_data_pp)->last_used_connection = conn;
		ret = orig_mysqlnd_conn_methods->query(conn, query, query_len TSRMLS_CC);
		DBG_RETURN(ret);
	}
	DBG_INF_FMT("Using slave. Thread "MYSQLND_LLU_SPEC, (*element)->m->get_thread_id(*element TSRMLS_CC));
	(*conn_data_pp)->last_used_connection = *element;
	ret = orig_mysqlnd_conn_methods->query(*element, query, query_len TSRMLS_CC);
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


/* {{{ MYSQLND_METHOD(mysqlnd_ms, free_contents) */
static void
MYSQLND_METHOD(mysqlnd_ms, free_contents)(MYSQLND *conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** data_pp;
	DBG_ENTER("mysqlnd_ms::free_contents");

	data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	DBG_INF_FMT("data_pp=%p", data_pp);
	if (data_pp && *data_pp) {
		DBG_INF_FMT("cleaning the llist");
		zend_llist_clean(&(*data_pp)->connections);
		mnd_pefree(*data_pp, conn->persistent);
		*data_pp = NULL;
	}
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


/* {{{ mysqlnd_ms::set_charset */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, set_charset)(MYSQLND * const proxy_conn, const char * const csname TSRMLS_DC)
{
	enum_func_status ret;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	DBG_ENTER("mysqlnd_ms::set_charset");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, (conn)->m->get_thread_id(conn TSRMLS_CC));
	ret = orig_mysqlnd_conn_methods->set_charset(conn, csname TSRMLS_CC);
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


/* {{{ mysqlnd_ms_functions[] */
static const zend_function_entry mysqlnd_ms_functions[] = {
	{NULL, NULL, NULL}	/* Must be the last line in mysqlnd_ms_functions[] */
};
/* }}} */


/* {{{ mysqlnd_ms_deps[] */
static const zend_module_dep mysqlnd_ms_deps[] = {
	ZEND_MOD_REQUIRED("mysqlnd")
	ZEND_MOD_REQUIRED("standard")
	{NULL, NULL, NULL}
};
/* }}} */


/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("mysqlnd_ms.enable", "0", PHP_INI_SYSTEM, OnUpdateBool, enable, zend_mysqlnd_ms_globals, mysqlnd_ms_globals)
	STD_PHP_INI_ENTRY("mysqlnd_ms.force_config_usage", "0", PHP_INI_SYSTEM, OnUpdateBool, force_config_usage, zend_mysqlnd_ms_globals, mysqlnd_ms_globals)
	STD_PHP_INI_ENTRY("mysqlnd_ms.ini_file", NULL, PHP_INI_SYSTEM, OnUpdateString, ini_file, zend_mysqlnd_ms_globals, mysqlnd_ms_globals)
PHP_INI_END()
/* }}} */


/* {{{ mysqlnd_qc_register_hooks*/
static void
mysqlnd_ms_register_hooks()
{
	orig_mysqlnd_conn_methods = mysqlnd_conn_get_methods();

	memcpy(&my_mysqlnd_conn_methods, orig_mysqlnd_conn_methods, sizeof(struct st_mysqlnd_conn_methods));

	my_mysqlnd_conn_methods.connect			= MYSQLND_METHOD(mysqlnd_ms, connect);
	my_mysqlnd_conn_methods.query			= MYSQLND_METHOD(mysqlnd_ms, query);
	my_mysqlnd_conn_methods.use_result		= MYSQLND_METHOD(mysqlnd_ms, use_result);
	my_mysqlnd_conn_methods.store_result	= MYSQLND_METHOD(mysqlnd_ms, store_result);
	my_mysqlnd_conn_methods.free_contents	= MYSQLND_METHOD(mysqlnd_ms, free_contents);
	my_mysqlnd_conn_methods.escape_string	= MYSQLND_METHOD(mysqlnd_ms, escape_string);
	my_mysqlnd_conn_methods.set_charset		= MYSQLND_METHOD(mysqlnd_ms, set_charset);
	my_mysqlnd_conn_methods.next_result		= MYSQLND_METHOD(mysqlnd_ms, next_result);
	my_mysqlnd_conn_methods.more_results	= MYSQLND_METHOD(mysqlnd_ms, more_results);
	my_mysqlnd_conn_methods.get_error_no	= MYSQLND_METHOD(mysqlnd_ms, errno);
	my_mysqlnd_conn_methods.get_error_str	= MYSQLND_METHOD(mysqlnd_ms, error);
	my_mysqlnd_conn_methods.get_sqlstate	= MYSQLND_METHOD(mysqlnd_ms, sqlstate);


	my_mysqlnd_conn_methods.get_field_count		= MYSQLND_METHOD(mysqlnd_ms, field_count);
	my_mysqlnd_conn_methods.get_last_insert_id	= MYSQLND_METHOD(mysqlnd_ms, insert_id);
	my_mysqlnd_conn_methods.get_affected_rows	= MYSQLND_METHOD(mysqlnd_ms, affected_rows);
	my_mysqlnd_conn_methods.get_warning_count	= MYSQLND_METHOD(mysqlnd_ms, warning_count);
	my_mysqlnd_conn_methods.get_last_message	= MYSQLND_METHOD(mysqlnd_ms, info);

	mysqlnd_conn_set_methods(&my_mysqlnd_conn_methods);
}
/* }}} */


/* {{{ php_mysqlnd_qc_init_globals */
static void
php_mysqlnd_ms_init_globals(zend_mysqlnd_ms_globals * mysqlnd_ms_globals)
{
	mysqlnd_ms_globals->enable = FALSE;
	mysqlnd_ms_globals->force_config_usage = FALSE;
	mysqlnd_ms_globals->ini_file = NULL;
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
	MYSQLND_MS_CONFIG_LOCK;
	if (FALSE == mysqlns_ms_global_config_loaded) {
		mysqlnd_ms_init_server_list(&mysqlnd_ms_config TSRMLS_CC);
		mysqlns_ms_global_config_loaded = TRUE;
	}
	MYSQLND_MS_CONFIG_UNLOCK;
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
	php_info_print_table_start();
	php_info_print_table_header(2, "mysqlnd_ms support", "enabled");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */


/* {{{ mysqlnd_ms_module_entry
 */
zend_module_entry mysqlnd_ms_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	mysqlnd_ms_deps,
	"mysqlnd_ms",
	mysqlnd_ms_functions,
	PHP_MINIT(mysqlnd_ms),
	PHP_MSHUTDOWN(mysqlnd_ms),
	PHP_RINIT(mysqlnd_ms), /* RINIT */
	NULL, /* RSHUT */
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
