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

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_priv.h"


struct st_mysqlnd_ms_json_config {
	HashTable config;
#ifdef ZTS
	MUTEX_T LOCK_access;
#endif	
};


/* {{{ mysqlnd_ms_json_config_init */
PHPAPI struct st_mysqlnd_ms_json_config *
mysqlnd_ms_json_config_init(TSRMLS_D)
{
	DBG_ENTER("mysqlnd_ms_json_config_init");
	
	DBG_RETURN(NULL);
}
/* }}} */


/* {{{ mysqlnd_ms_json_config_free */
PHPAPI void
mysqlnd_ms_json_config_free(struct st_mysqlnd_ms_json_config * cfg TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms_json_config_free");

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_json_config_init_server_list */
PHPAPI enum_func_status
mysqlnd_ms_json_config_init_server_list(struct st_mysqlnd_ms_json_config * cfg TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms_json_config_init_server_list");

	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ mysqlnd_ms_config_json_section_exists */
PHPAPI zend_bool
mysqlnd_ms_config_json_section_exists(struct st_mysqlnd_ms_json_config * cfg, const char * section, size_t section_len,
									  zend_bool use_lock TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms_config_json_section_exists");

	DBG_RETURN(FALSE);
}
/* }}} */


/* {{{ mysqlnd_ms_config_json_string */
PHPAPI char *
mysqlnd_ms_config_json_string(struct st_mysqlnd_ms_json_config * cfg, const char * section, size_t section_len,
							  const char * name, size_t name_len,
							  zend_bool * exists, zend_bool * is_list_value, zend_bool use_lock TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms_config_json_string");

	DBG_RETURN(NULL);
}
/* }}} */


/* {{{ mysqlnd_ms_config_json_reset_section */
PHPAPI void
mysqlnd_ms_config_json_reset_section(struct st_mysqlnd_ms_json_config * cfg, const char * section, size_t section_len,
										 zend_bool use_lock TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms_config_json_reset_section");

	DBG_VOID_RETURN;
}
/* }}} */


#ifdef ZTS
/* {{{ mysqlnd_ms_config_json_lock */
PHPAPI void
mysqlnd_ms_config_json_lock(struct st_mysqlnd_ms_json_config * cfg, const char * const file, unsigned int line TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms_config_json_lock");
	DBG_INF_FMT("mutex=%p file=%s line=%u", cfg->LOCK_access, file, line);
	tsrm_mutex_lock(cfg->LOCK_access);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_config_json_unlock */
PHPAPI void
mysqlnd_ms_config_json_unlock(struct st_mysqlnd_ms_json_config * cfg, const char * const file, unsigned int line TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms_config_json_unlock");
	DBG_INF_FMT("mutex=%p file=%s line=%u", cfg->LOCK_access, file, line);
	tsrm_mutex_unlock(cfg->LOCK_access);
	DBG_VOID_RETURN;
}
/* }}} */
#endif
