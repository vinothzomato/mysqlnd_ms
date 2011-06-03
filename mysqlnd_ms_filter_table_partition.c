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
#include "mysqlnd_ms_switch.h"
#include "mysqlnd_ms_enum_n_def.h"


/* {{{ mysqlnd_ms_choose_connection_table_filter */
enum_func_status
mysqlnd_ms_choose_connection_table_filter(MYSQLND * conn, const char * query, unsigned int query_len,
												 struct mysqlnd_ms_lb_strategies * stgy,
												 zend_llist ** master_list, zend_llist ** slave_list TSRMLS_DC)
{
	struct st_mysqlnd_query_parser * parser;
	DBG_ENTER("mysqlnd_ms_choose_connection_table_filter");
	parser = mysqlnd_qp_create_parser(TSRMLS_C);
	if (parser) {
		int ret = mysqlnd_qp_start_parser(parser, query, query_len TSRMLS_CC);
		DBG_INF_FMT("mysqlnd_qp_start_parser=%d", ret);
		DBG_INF_FMT("db=%s table=%s org_table=%s statement_type=%d",
				parser->parse_info.db? parser->parse_info.db:"n/a",
				parser->parse_info.table? parser->parse_info.table:"n/a",
				parser->parse_info.org_table? parser->parse_info.org_table:"n/a",
				parser->parse_info.statement
			);

		zend_llist_init(*master_list, sizeof(MYSQLND_MS_LIST_DATA), (llist_dtor_func_t) mysqlnd_ms_conn_list_dtor, conn->persistent);
		zend_llist_init(*slave_list, sizeof(MYSQLND_MS_LIST_DATA), (llist_dtor_func_t) mysqlnd_ms_conn_list_dtor, conn->persistent);
#if 0
		{
			MYSQLND_MS_LIST_DATA new_element = {0};
			new_element.conn = conn;
			new_element.host = host? mnd_pestrdup(host, persistent) : NULL;
			new_element.persistent = persistent;
			new_element.port = port;
			new_element.socket = socket? mnd_pestrdup(socket, conn->persistent) : NULL;
			new_element.emulated_scheme_len = mysqlnd_ms_get_scheme_from_list_data(&new_element, &new_element.emulated_scheme,
																			   persistent TSRMLS_CC);
			zend_llist_add_element(*master_list, &new_element);
		
		}
#endif
		mysqlnd_qp_free_parser(parser TSRMLS_CC);
		DBG_RETURN(PASS);
	}
	DBG_RETURN(FAIL);
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
