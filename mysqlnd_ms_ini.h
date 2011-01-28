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
#ifndef MYSQLND_MS_INI_H 
#define MYSQLND_MS_INI_H

#ifdef ZTS
extern MUTEX_T LOCK_global_config_access;
#define MYSQLND_MS_CONFIG_LOCK tsrm_mutex_lock(LOCK_global_config_access)
#define MYSQLND_MS_CONFIG_UNLOCK tsrm_mutex_unlock(LOCK_global_config_access)

#else
#define MYSQLND_MS_CONFIG_LOCK
#define MYSQLND_MS_CONFIG_UNLOCK
#endif

#define GLOBAL_SECTION_NAME "global"

enum_func_status mysqlnd_ms_init_server_list(HashTable * configuration TSRMLS_DC);
zend_bool mysqlnd_ms_ini_get_section(HashTable * config, const char * section, size_t section_len TSRMLS_DC);
char * mysqlnd_ms_ini_string(HashTable * config, const char * section, size_t section_len, const char * name, size_t name_len, zend_bool * exists TSRMLS_DC);


#endif	/* MYSQLND_MS_INI_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
