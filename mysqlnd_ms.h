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
#ifndef MYSQLND_MS_H
#define MYSQLND_MS_H

#define SMART_STR_START_SIZE 1024
#define SMART_STR_PREALLOC 256
#include "ext/standard/php_smart_str.h"

#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_statistics.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_priv.h"
#include "mysqlnd_ms_tokenize.h"


#ifdef ZTS
#include "TSRM.h"
#endif

PHP_FUNCTION(confirm_mysqlnd_ms_compiled);	/* For testing, remove later. */

ZEND_BEGIN_MODULE_GLOBALS(mysqlnd_ms)
	zend_bool enable;
	zend_bool force_config_usage;
	const char * ini_file;
	zval * user_pick_server;
ZEND_END_MODULE_GLOBALS(mysqlnd_ms)

/* In every utility function you add that needs to use variables
   in php_mysqlnd_ms_globals, call TSRMLS_FETCH(); after declaring other
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as MYSQLND_MS_G(variable).  You are
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define MYSQLND_MS_G(v) TSRMG(mysqlnd_ms_globals_id, zend_mysqlnd_ms_globals *, v)
#else
#define MYSQLND_MS_G(v) (mysqlnd_ms_globals.v)
#endif

#endif	/* MYSQLND_MS_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
