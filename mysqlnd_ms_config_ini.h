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
#ifndef MYSQLND_MS_INI_H
#define MYSQLND_MS_INI_H

struct st_mysqlnd_ms_ini_config;

struct st_mysqlnd_ms_ini_config * mysqlnd_ms_config_init(TSRMLS_D);
void mysqlnd_ms_config_free(struct st_mysqlnd_ms_ini_config * cfg TSRMLS_DC);
enum_func_status mysqlnd_ms_config_init_server_list(struct st_mysqlnd_ms_ini_config * cfg TSRMLS_DC);
zend_bool mysqlnd_ms_config_ini_section_exists(struct st_mysqlnd_ms_ini_config * cfg, const char * section, size_t section_len, zend_bool use_lock TSRMLS_DC);
char * mysqlnd_ms_config_ini_string(struct st_mysqlnd_ms_ini_config * cfg, const char * section, size_t section_len, const char * name, size_t name_len, zend_bool * exists, zend_bool * is_list_value, zend_bool use_lock TSRMLS_DC);
void mysqlnd_ms_config_ini_reset_section(struct st_mysqlnd_ms_ini_config * cfg, const char * section, size_t section_len, zend_bool use_lock TSRMLS_DC);

#ifdef ZTS
void mysqlnd_ms_config_ini_lock(struct st_mysqlnd_ms_ini_config * cfg, const char * const file, unsigned int line TSRMLS_DC);
void mysqlnd_ms_config_ini_unlock(struct st_mysqlnd_ms_ini_config * cfg, const char * const file, unsigned int line TSRMLS_DC);
#define MYSQLND_MS_CONFIG_LOCK(cfg) mysqlnd_ms_config_ini_lock((cfg), __FILE__, __LINE__ TSRMLS_CC)
#define MYSQLND_MS_CONFIG_UNLOCK(cfg) mysqlnd_ms_config_ini_unlock((cfg), __FILE__, __LINE__ TSRMLS_CC)
#else
#define MYSQLND_MS_CONFIG_LOCK(cfg)
#define MYSQLND_MS_CONFIG_UNLOCK(cfg)
#endif


#endif	/* MYSQLND_MS_INI_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
