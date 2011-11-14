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
#include "ext/standard/info.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_priv.h"
#ifndef mnd_emalloc
#include "ext/mysqlnd/mysqlnd_alloc.h"
#endif
#include "mysqlnd_ms.h"
#include "mysqlnd_ms_config_json.h"

#include "mysqlnd_ms_enum_n_def.h"

/* {{{ mysqlnd_ms_qos_pick_server */

enum_func_status mysqlnd_ms_qos_pick_server(void * f_data, const char * connect_host, const char * query, size_t query_len,
									 zend_llist * master_list, zend_llist * slave_list,
									 zend_llist * selected_masters, zend_llist * selected_slaves,
									 struct mysqlnd_ms_lb_strategies * stgy, MYSQLND_ERROR_INFO * error_info
									 TSRMLS_DC)
{
	enum_func_status ret = PASS;
	MYSQLND_MS_FILTER_QOS_DATA * filter_data = (MYSQLND_MS_FILTER_QOS_DATA *) f_data;

	DBG_ENTER("mysqlnd_ms_qos_pick_server");
	DBG_INF_FMT("query(50bytes)=%*s", MIN(50, query_len), query);

	switch (filter_data->consistency)
	{
		case CONSISTENCY_SESSION:
			/*
			For now...
				... fall-through
			For later...
				... consider use of selected slaves

				 We may be able to use selected slaves which have replicated
				 the last write on the line, e.g. using global transaction ID.

				 We may be able to use slaves which have replicated certain,
				 "tagged" writes. For example, the user could have a relaxed
				 definition of session consistency and require only consistent
				 reads from one table. In that case, we may use master and
				 all slaves which have replicated the latest updates on the
				 table in question. Not sure if that makes sense.
			*/
		case CONSISTENCY_STRONG:
			/*
			For now and forever...
				... use masters, no slaves.

				This is our master_on_write replacement. All the other filters
				don't need to take care in the future.
			*/
			DBG_INF("using masters only for strong consistency");
			zend_llist_copy(selected_masters, master_list);
			break;

		case CONSISTENCY_EVENTUAL:
			/*
			For now...
				... allow all masters and all slaves.
			For later...
				... optional filtering of slaves based on e.g. replication lag

				We may inject mysqlnd_qc per-query TTL SQL hints here to
				replace a slave access with a call access.
			*/
			zend_llist_copy(selected_masters, master_list);
			zend_llist_copy(selected_slaves, slave_list);
			break;

		default:
			DBG_ERR("Invalid filter data, we should never get here");
			ret = FAIL;
			break;

	}

	DBG_RETURN(ret);
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
