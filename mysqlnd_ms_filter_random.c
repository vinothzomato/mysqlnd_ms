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
mysqlnd_ms_choose_connection_random(void * f_data, const char * const query, const size_t query_len,
									struct mysqlnd_ms_lb_strategies * stgy,
									zend_llist * master_connections, zend_llist * slave_connections,
									enum enum_which_server * which_server TSRMLS_DC)
{
	MYSQLND_MS_FILTER_RANDOM_DATA * filter = (MYSQLND_MS_FILTER_RANDOM_DATA *) f_data;
	zend_bool forced;
	enum enum_which_server tmp_which;
	smart_str fprint = {0};
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
			MYSQLND_MS_LIST_DATA * element = NULL, ** element_pp = NULL;
			unsigned long rnd_idx;
			uint i = 0;
			MYSQLND * connection = NULL;
			MYSQLND ** context_pos;
			mysqlnd_ms_get_fingerprint(&fprint, l TSRMLS_CC);

			DBG_INF_FMT("%d slaves to choose from", zend_llist_count(l));

			/* LOCK on context ??? */
			switch (zend_hash_find(&filter->sticky.slave_context, fprint.c, fprint.len /*\0 counted*/, (void **) &context_pos)) {
				case SUCCESS:
					smart_str_free(&fprint);
					connection = context_pos? *context_pos : NULL;
					if (!connection) {
						php_error_docref(NULL TSRMLS_CC, E_ERROR, MYSQLND_MS_ERROR_PREFIX " Something is very wrong for slave random_once.");
					} else {
						DBG_INF_FMT("Using already selected slave connection "MYSQLND_LLU_SPEC, connection->thread_id);
						DBG_RETURN(connection);
					}
					break;
				case FAILURE:
					rnd_idx = php_rand(TSRMLS_C);
					RAND_RANGE(rnd_idx, 0, zend_llist_count(l) - 1, PHP_RAND_MAX);
					DBG_INF_FMT("USE_SLAVE rnd_idx=%lu", rnd_idx);

					element_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(l, &pos);
					while (i++ < rnd_idx) {
						element_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(l, &pos);
					}
					connection = (element_pp && (element = *element_pp) && element->conn) ? element->conn : NULL;

					if (!connection) {
						smart_str_free(&fprint);
						if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
							/* TODO: connection error would be better */
							php_error_docref(NULL TSRMLS_CC, E_ERROR,
									 MYSQLND_MS_ERROR_PREFIX " Couldn't find the appropriate slave connection. %d slaves to choose from. Something is wrong", zend_llist_count(l));
							/* should be a very rare case to be here - connection shouldn't be NULL in first place */
							DBG_RETURN(connection);
						}
					} else {
						do {
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
								} else {
									MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_FAILURE);
									smart_str_free(&fprint);
									if (SERVER_FAILOVER_DISABLED == stgy->failover_strategy) {
									  /* no failover */
									  DBG_RETURN(connection);
									}
									goto fallthrough;
								}
							}
							if (TRUE == filter->sticky.once) {
								zend_hash_update(&filter->sticky.slave_context, fprint.c, fprint.len /*\0 counted*/, &connection,
												 sizeof(MYSQLND *), NULL);
							}
						} while (0);
						smart_str_free(&fprint);
						DBG_RETURN(connection);
					}
			}/* switch (zend_hash_find) */
		}
fallthrough:
		DBG_INF("FAIL-OVER");
		/* fall-through */
		case USE_MASTER:
		{
			zend_llist_position	pos;
			zend_llist * l = master_connections;
			MYSQLND_MS_LIST_DATA * element = NULL, ** element_pp = NULL;
			unsigned long rnd_idx;
			uint i = 0;
			MYSQLND * connection = NULL;
			MYSQLND ** context_pos;
			mysqlnd_ms_get_fingerprint(&fprint, l TSRMLS_CC);

			DBG_INF_FMT("%d masters to choose from", zend_llist_count(l));

			/* LOCK on context ??? */
			switch (zend_hash_find(&filter->sticky.master_context, fprint.c, fprint.len /*\0 counted*/, (void **) &context_pos)) {
				case SUCCESS:
					connection = context_pos? *context_pos : NULL;
					smart_str_free(&fprint);
					if (!connection) {
						php_error_docref(NULL TSRMLS_CC, E_ERROR, MYSQLND_MS_ERROR_PREFIX " Something is very wrong for master random_once.");
					} else {
						DBG_INF_FMT("Using already selected master connection "MYSQLND_LLU_SPEC, connection->thread_id);
						DBG_RETURN(connection);
					}
					break;
				case FAILURE:
					rnd_idx = php_rand(TSRMLS_C);
					RAND_RANGE(rnd_idx, 0, zend_llist_count(l) - 1, PHP_RAND_MAX);
					DBG_INF_FMT("USE_MASTER rnd_idx=%lu", rnd_idx);

					element_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(l, &pos);
					while (i++ < rnd_idx) {
						element_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(l, &pos);
					}
					connection = (element_pp && (element = *element_pp) && element->conn) ? element->conn : NULL;

					if (connection) {
						do {
							if (CONN_GET_STATE(connection) == CONN_ALLOCED) {
								DBG_INF("Lazy connection, trying to connect...");
								/* lazy connection, connect now */
								if (PASS == ms_orig_mysqlnd_conn_methods->connect(connection, element->host, element->user,
																   element->passwd, element->passwd_len,
																   element->db, element->db_len,
																   element->port, element->socket, element->connect_flags TSRMLS_CC))
								{
									DBG_INF("Connected");
									MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_MASTER_SUCCESS);
								} else {
									MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_MASTER_FAILURE);
									break;
								}
							}
							if (TRUE == filter->sticky.once) {
								zend_hash_update(&filter->sticky.master_context, fprint.c, fprint.len /*\0 counted*/, &connection,
												 sizeof(MYSQLND *), NULL);
							}
						} while (0);
					} else {
						php_error_docref(NULL TSRMLS_CC, E_ERROR,
									 MYSQLND_MS_ERROR_PREFIX " Couldn't find the appropriate master connection. Something is wrong");

					}
					smart_str_free(&fprint);
					DBG_RETURN(connection);
					break;
			}/* switch (zend_hash_find) */
			break;
		}
		case USE_LAST_USED:
			DBG_INF("Using last used connection");
			DBG_RETURN(stgy->last_used_conn);
		default:
			/* error */
			break;
	}

	DBG_RETURN(NULL);
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
