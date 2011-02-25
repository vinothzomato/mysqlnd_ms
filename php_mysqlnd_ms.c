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

ZEND_DECLARE_MODULE_GLOBALS(mysqlnd_ms)

unsigned int mysqlnd_ms_plugin_id;

static zend_bool mysqlns_ms_global_config_loaded = FALSE;
HashTable mysqlnd_ms_config;


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


/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("mysqlnd_ms.enable", "0", PHP_INI_SYSTEM, OnUpdateBool, enable, zend_mysqlnd_ms_globals, mysqlnd_ms_globals)
	STD_PHP_INI_ENTRY("mysqlnd_ms.force_config_usage", "0", PHP_INI_SYSTEM, OnUpdateBool, force_config_usage, zend_mysqlnd_ms_globals, mysqlnd_ms_globals)
	STD_PHP_INI_ENTRY("mysqlnd_ms.ini_file", NULL, PHP_INI_SYSTEM, OnUpdateString, ini_file, zend_mysqlnd_ms_globals, mysqlnd_ms_globals)
PHP_INI_END()
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
#ifdef ALL_SERVER_DISPATCH
	REGISTER_STRING_CONSTANT("MYSQLND_MS_ALL_SERVER_SWITCH", ALL_SERVER_SWITCH, CONST_CS | CONST_PERSISTENT);
#endif
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
