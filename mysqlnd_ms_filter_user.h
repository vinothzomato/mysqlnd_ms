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
  +----------------------------------------------------------------------+
*/

/* $Id: mysqlnd_ms.h 311510 2011-05-27 19:36:39Z andrey $ */
#ifndef MYSQLND_MS_FILTER_USER_H
#define MYSQLND_MS_FILTER_USER_H

MYSQLND * mysqlnd_ms_user_pick_server(MYSQLND * conn, const char * query, size_t query_len, zend_llist * master_list, zend_llist * slave_list TSRMLS_DC);
enum_func_status mysqlnd_ms_user_pick_multiple_server(MYSQLND * conn, const char * query, size_t query_len, zend_llist * master_list, zend_llist * slave_list, zend_llist * selected_masters, zend_llist * selected_slaves TSRMLS_DC);


#endif	/* MYSQLND_MS_FILTER_USER_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
