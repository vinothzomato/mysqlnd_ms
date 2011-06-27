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


/* {{{ mysqlnd_ms_choose_connection_random */
MYSQLND *
mysqlnd_ms_choose_connection_random(const char * const query, const size_t query_len,
									struct mysqlnd_ms_lb_strategies * stgy,
									zend_llist * master_connections, zend_llist * slave_connections,
									enum enum_which_server * which_server TSRMLS_DC)
{
	zend_bool forced;
	enum enum_which_server tmp_which;
	DBG_ENTER("mysqlnd_ms_choose_connection_random");

	if (!which_server) {
		which_server = &tmp_which;
	}
	*which_server = mysqlnd_ms_query_is_select(query, query_len, &forced TSRMLS_CC);
	if ((stgy->trx_stickiness_strategy == TRX_STICKINESS_STRATEGY_MASTER) && stgy->in_transaction && !forced) {
		DBG_INF("Enforcing use of master while in transaction");
		*which_server = USE_MASTER;
		MYSQLND_MS_INC_STATISTIC(MS_STAT_TRX_MASTER_FORCED);
	} else if (stgy->mysqlnd_ms_flag_master_on_write) {
		if (*which_server != USE_MASTER) {
			if (stgy->master_used && !forced) {
				switch (*which_server) {
					case USE_MASTER:
					case USE_LAST_USED:
						break;
					case USE_SLAVE:
					default:
						DBG_INF("Enforcing use of master after write");
						*which_server = USE_MASTER;
						break;
				}
			}
		} else {
			DBG_INF("Use of master detected");
			stgy->master_used = TRUE;
		}
	}

	switch (*which_server) {
		case USE_SLAVE:
		{
			zend_llist_position	pos;
			zend_llist * l = slave_connections;
			MYSQLND_MS_LIST_DATA * element, ** element_pp;
			unsigned long rnd_idx;
			uint i = 0;
			MYSQLND * connection;

			DBG_INF_FMT("%d slaves to choose from", zend_llist_count(l));
			rnd_idx = php_rand(TSRMLS_C);
			RAND_RANGE(rnd_idx, 0, zend_llist_count(l) - 1, PHP_RAND_MAX);
			DBG_INF_FMT("USE_SLAVE rnd_idx=%lu", rnd_idx);

			element_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(l, &pos);
			while (i++ < rnd_idx) {
				element_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(l, &pos);
			}
			connection = (element_pp && (element = *element_pp) && element->conn) ? element->conn : NULL;
			if (connection) {
				DBG_INF_FMT("Using slave connection "MYSQLND_LLU_SPEC"", connection->thread_id);

				if (CONN_GET_STATE(connection) == CONN_ALLOCED) {
					DBG_INF("Lazy connection, trying to connect...");
					/* lazy connection, connect now */
					if (PASS == ms_orig_mysqlnd_conn_methods->connect(connection, element->host, element->user,
																   element->passwd, element->passwd_len,
																   element->db, element->db_len,
																   element->port, element->socket, element->connect_flags TSRMLS_CC))
					{
						DBG_INF("Connected");
						MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_SUCCESS);
						DBG_RETURN(connection);
					}
					MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_FAILURE);
					if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
					  DBG_INF("Failover disabled");
					  DBG_RETURN(connection);
					}
					DBG_INF("Connect failed, falling back to the master");
				} else {
					DBG_RETURN(connection);
				}
			} else {
				if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
					DBG_INF("Failover disabled");
					DBG_RETURN(connection);
				}
			}
		}
		/* fall-through */
		case USE_MASTER:
		{
			zend_llist_position	pos;
			zend_llist * l = master_connections;
			MYSQLND_MS_LIST_DATA * element, ** element_pp;
			unsigned long rnd_idx;
			uint i = 0;
			MYSQLND * connection = NULL;

			DBG_INF_FMT("%d masters to choose from", zend_llist_count(l));

			rnd_idx = php_rand(TSRMLS_C);
			RAND_RANGE(rnd_idx, 0, zend_llist_count(l) - 1, PHP_RAND_MAX);
			DBG_INF_FMT("USE_MASTER rnd_idx=%lu", rnd_idx);

			element_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(l, &pos);
			while (i++ < rnd_idx) {
				element_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(l, &pos);
			}
			connection = (element_pp && (element = *element_pp) && element->conn) ? element->conn : NULL;
			DBG_INF("Using master connection");
			if (connection) {
				if (CONN_GET_STATE(connection) == CONN_ALLOCED) {
					DBG_INF("Lazy connection, trying to connect...");
					/* lazy connection, connect now */
					if (PASS == ms_orig_mysqlnd_conn_methods->connect(connection, element->host, element->user,
																   element->passwd, element->passwd_len,
																   element->db, element->db_len,
																   element->port, element->socket,
																   element->connect_flags TSRMLS_CC))
					{
						DBG_INF("Connected");
						DBG_INF_FMT("Using master connection "MYSQLND_LLU_SPEC"", connection->thread_id);
						MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_MASTER_SUCCESS);
						DBG_RETURN(connection);
					}
					MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_MASTER_FAILURE);
				}
			}
			DBG_RETURN(connection);
			break;
		}

		case USE_LAST_USED:
			DBG_INF("Using last used connection");
			DBG_RETURN(stgy->last_used_conn);
		default:
			/* error */
			DBG_RETURN(NULL);
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
