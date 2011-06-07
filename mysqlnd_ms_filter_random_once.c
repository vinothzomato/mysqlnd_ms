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
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_priv.h"
#ifndef mnd_emalloc
#include "ext/mysqlnd/mysqlnd_alloc.h"
#endif
#include "mysqlnd_ms.h"
#include "ext/standard/php_rand.h"
#include "mysqlnd_ms_switch.h"
#include "mysqlnd_ms_enum_n_def.h"
#include "mysqlnd_ms_filter_random.h"


/* {{{ mysqlnd_ms_select_servers_random_once */
enum_func_status
mysqlnd_ms_select_servers_random_once(enum php_mysqlnd_server_command command,
									  struct mysqlnd_ms_lb_strategies * stgy,
									  zend_llist * master_list, zend_llist * slave_list,
									  zend_llist * selected_masters, zend_llist * selected_slaves TSRMLS_DC)
{
	enum_func_status ret = FAIL;
	zend_llist_position	pos;
	MYSQLND_MS_LIST_DATA * el, ** el_pp;
	DBG_ENTER("mysqlnd_ms_select_servers_random_once");
	DBG_INF_FMT("random_once_slave=%p", stgy->random_once_slave);
	if (!stgy->random_once_slave) {
		ret = mysqlnd_ms_select_servers_all(master_list, slave_list, selected_masters, selected_slaves TSRMLS_CC);
		DBG_RETURN(ret);
	}

	/* search the list of easy handles hanging off the multi-handle */
	for (el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(master_list, &pos); el_pp && (el = *el_pp) && el->conn;
			el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(master_list, &pos))
	{
		/*
		  This will copy the whole structure, not the pointer.
		  This is wanted!!
		*/
		zend_llist_add_element(selected_masters, el);
	}

	for (el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(slave_list, &pos); el_pp && (el = *el_pp) && el->conn;
			el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(slave_list, &pos))
	{
		DBG_INF_FMT("[slave] el->conn=%p", el->conn);
		if (el->conn == stgy->random_once_slave) {
			/*
			  This will copy the whole structure, not the pointer.
			  This is wanted!!
			*/
			zend_llist_add_element(selected_slaves, el);
			ret = PASS;
			DBG_INF("bingo!");
			break;
		}
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_choose_connection_random_once */
MYSQLND *
mysqlnd_ms_choose_connection_random_once(const char * const query, const size_t query_len,
										 struct mysqlnd_ms_lb_strategies * stgy,
										 zend_llist * master_connections, zend_llist * slave_connections,
										 enum enum_which_server * which_server TSRMLS_DC)
{
	enum enum_which_server tmp_which;
	MYSQLND * ret;
	DBG_ENTER("mysqlnd_ms_choose_connection_random_once");

	if (!which_server) {
		which_server = &tmp_which;
	}
	ret = mysqlnd_ms_choose_connection_random(query, query_len, stgy, master_connections, slave_connections, which_server TSRMLS_CC);
	switch (*which_server) {
		case USE_SLAVE:
			if (!stgy->random_once_slave) {
				stgy->random_once_slave = ret;
			}
			DBG_INF_FMT("Using last random connection "MYSQLND_LLU_SPEC, stgy->random_once_slave->thread_id);
			DBG_RETURN(stgy->random_once_slave);
		case USE_MASTER:
		case USE_LAST_USED:
		default:
			DBG_RETURN(ret);
			break;
	}
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
