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
#ifndef mnd_sprintf
#define mnd_sprintf spprintf
#endif
#include "mysqlnd_ms.h"
#include "mysqlnd_ms_config_json.h"
#include "ext/standard/php_rand.h"

#include "mysqlnd_query_parser.h"
#include "mysqlnd_qp.h"

#include "mysqlnd_ms_enum_n_def.h"

#define MS_STRING(vl, a)				\
{											\
	MAKE_STD_ZVAL((a));						\
	ZVAL_STRING((a), (char *)(vl), 1);	\
}

#define MS_STRINGL(vl, ln, a)				\
{											\
	MAKE_STD_ZVAL((a));						\
	ZVAL_STRINGL((a), (char *)(vl), (ln), 1);	\
}

#define MS_ARRAY(a)		\
{						\
	MAKE_STD_ZVAL((a));	\
	array_init((a));	\
}

/* {{{ mysqlnd_ms_call_handler */
static zval *
mysqlnd_ms_call_handler(zval *func, int argc, zval **argv, zend_bool destroy_args TSRMLS_DC)
{
	int i;
	zval * retval;
	DBG_ENTER("mysqlnd_ms_call_handler");

	MAKE_STD_ZVAL(retval);
	if (call_user_function(EG(function_table), NULL, func, retval, argc, argv TSRMLS_CC) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " %s Failed to call '%s'", MYSQLND_MS_ERROR_PREFIX, Z_STRVAL_P(func));
		zval_ptr_dtor(&retval);
		retval = NULL;
	}

	if (destroy_args == TRUE) {
		for (i = 0; i < argc; i++) {
			zval_ptr_dtor(&argv[i]);
		}
	}

	DBG_RETURN(retval);
}
/* }}} */


/* {{{ mysqlnd_ms_user_pick_server */
MYSQLND *
mysqlnd_ms_user_pick_server(void * f_data, const char * connect_host, const char * query, size_t query_len,
							zend_llist * master_list, zend_llist * slave_list,
							struct mysqlnd_ms_lb_strategies * stgy TSRMLS_DC)
{
	MYSQLND_MS_FILTER_USER_DATA * filter_data = (MYSQLND_MS_FILTER_USER_DATA *) f_data;
	zval * args[7];
	zval * retval;
	MYSQLND * ret = NULL;

	DBG_ENTER("mysqlnd_ms_user_pick_server");
	DBG_INF_FMT("query(50bytes)=%*s query_is_select=%p", MIN(50, query_len), query, MYSQLND_MS_G(user_pick_server));

	if (master_list && filter_data && filter_data->user_callback) {
		uint param = 0;
#ifdef ALL_SERVER_DISPATCH
		uint use_all_pos = 0;
#endif
		/* connect host */
		MS_STRING((char *) connect_host, args[param]);

		/* query */
		param++;
		MS_STRINGL((char *) query, query_len, args[param]);
		{
			MYSQLND_MS_LIST_DATA * el, ** el_pp;
			zend_llist_position	pos;
			/* master list */
			param++;
			MS_ARRAY(args[param]);
			for (el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(master_list, &pos); el_pp && (el = *el_pp) && el->conn;
					el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(master_list, &pos))
			{
				if (CONN_GET_STATE(el->conn) == CONN_ALLOCED) {
					/* lazy */
					add_next_index_stringl(args[param], el->emulated_scheme, el->emulated_scheme_len, 1);
				} else {
					add_next_index_stringl(args[param], el->conn->scheme, el->conn->scheme_len, 1);
				}
			}

			/* slave list*/
			param++;
			MS_ARRAY(args[param]);
			if (slave_list) {
				for (el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(slave_list, &pos); el_pp && (el = *el_pp) && el->conn;
						el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(slave_list, &pos))
				{
					if (CONN_GET_STATE(el->conn) == CONN_ALLOCED) {
						/* lazy */
						add_next_index_stringl(args[param], el->emulated_scheme, el->emulated_scheme_len, 1);
					} else {
						add_next_index_stringl(args[param], el->conn->scheme, el->conn->scheme_len, 1);
					}
				}
			}
			/* last used connection */
			param++;
			MAKE_STD_ZVAL(args[param]);
			if (stgy->last_used_conn) {
				ZVAL_STRING(args[param], stgy->last_used_conn->scheme, 1);
			} else {
				ZVAL_NULL(args[param]);
			}

			/* in transaction */
			param++;
			MAKE_STD_ZVAL(args[param]);
			if (stgy->in_transaction) {
				ZVAL_TRUE(args[param]);
			} else {
				ZVAL_FALSE(args[param]);
			}
#ifdef ALL_SERVER_DISPATCH
			/* use all */
			use_all_pos = ++param;
			MAKE_STD_ZVAL(args[param]);
			Z_ADDREF_P(args[param]);
			ZVAL_FALSE(args[param]);
#endif
		}

		retval = mysqlnd_ms_call_handler(filter_data->user_callback, param + 1, args, FALSE /* we will destroy later */ TSRMLS_CC);
		if (retval) {
			if (Z_TYPE_P(retval) == IS_STRING) {
				do {
					MYSQLND_MS_LIST_DATA * el, ** el_pp;
					zend_llist_position	pos;

					for (el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(master_list, &pos);
						 !ret && el_pp && (el = *el_pp) && el->conn;
						 el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(master_list, &pos))
					{
						if (CONN_GET_STATE(el->conn) == CONN_ALLOCED) {
							/* lazy */
						} else {
							if (!strncasecmp(el->conn->scheme, Z_STRVAL_P(retval), MIN(Z_STRLEN_P(retval), el->conn->scheme_len))) {
								ret = el->conn;
								DBG_INF_FMT("Userfunc chose master host : [%*s]", el->conn->scheme_len, el->conn->scheme);
								MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_MASTER_CALLBACK);
							}
						}
					}
					if (slave_list) {
						for (el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(slave_list, &pos);
							 !ret && el_pp && (el = *el_pp) && el->conn;
							 el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(slave_list, &pos))
						{
							if (CONN_GET_STATE(el->conn) == CONN_ALLOCED) {
								/* lazy */
								if (!strncasecmp(el->emulated_scheme, Z_STRVAL_P(retval), MIN(Z_STRLEN_P(retval), el->emulated_scheme_len))) {
									DBG_INF_FMT("Userfunc chose LAZY slave host : [%*s]", el->emulated_scheme_len, el->emulated_scheme);
									MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_SLAVE_CALLBACK);
									if (PASS == ms_orig_mysqlnd_conn_methods->connect(el->conn, el->host, el->user,
																				   el->passwd, el->passwd_len,
																				   el->db, el->db_len,
																				   el->port, el->socket,
																				   el->connect_flags TSRMLS_CC))
									{
										ret = el->conn;
										DBG_INF("Connected");
										MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_SUCCESS);
									} else {
										php_error_docref(NULL TSRMLS_CC, E_WARNING, "Callback chose %s but connection failed", el->emulated_scheme);
										DBG_ERR("Connect failed, forwarding error to the user");
										ret = el->conn; /* no automatic action: leave it to the user to decide! */
										MYSQLND_MS_INC_STATISTIC(MS_STAT_LAZY_CONN_SLAVE_FAILURE);
									}
								}
							} else {
								if (!strncasecmp(el->conn->scheme, Z_STRVAL_P(retval), MIN(Z_STRLEN_P(retval), el->conn->scheme_len))) {
									MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_SLAVE_CALLBACK);
									ret = el->conn;
									DBG_INF_FMT("Userfunc chose slave host : [%*s]", el->conn->scheme_len, el->conn->scheme);
								}
							}
						}
					}
				} while (0);
			}
			zval_ptr_dtor(&retval);
		}
#ifdef ALL_SERVER_DISPATCH
		convert_to_boolean(args[use_all_pos]);
		Z_DELREF_P(args[use_all_pos]);
#endif
		/* destroy the params */
		{
			unsigned int i;
			for (i = 0; i <= param; i++) {
				zval_ptr_dtor(&args[i]);
			}
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ my_long_compare */
static int
my_long_compare(const void * a, const void * b TSRMLS_DC)
{
	Bucket * f = *((Bucket **) a);
	Bucket * s = *((Bucket **) b);
	zval * first = *((zval **) f->pData);
	zval * second = *((zval **) s->pData);

	if (Z_LVAL_P(first) > Z_LVAL_P(second)) {
		return 1;
	} else if (Z_LVAL_P(first) == Z_LVAL_P(second)) {
		return 0;
	}
	return -1;
}
/* }}} */


/* {{{ mysqlnd_ms_user_pick_multiple_server */
enum_func_status
mysqlnd_ms_user_pick_multiple_server(void * f_data, const char * connect_host, const char * query, size_t query_len,
									 zend_llist * master_list, zend_llist * slave_list,
									 zend_llist * selected_masters, zend_llist * selected_slaves,
									 struct mysqlnd_ms_lb_strategies * stgy 
									 TSRMLS_DC)
{
	MYSQLND_MS_FILTER_USER_DATA * filter_data = (MYSQLND_MS_FILTER_USER_DATA *) f_data;
	zval * args[7];
	zval * retval;
	enum_func_status ret = FAIL;

	DBG_ENTER("mysqlnd_ms_user_pick_multiple_server");
	DBG_INF_FMT("query(50bytes)=%*s query_is_select=%p", MIN(50, query_len), query, MYSQLND_MS_G(user_pick_server));

	if (master_list && filter_data && filter_data->user_callback) {
		uint param = 0;

		/* connect host */
		MS_STRING((char *) connect_host, args[param]);

		/* query */
		param++;
		MS_STRINGL((char *) query, query_len, args[param]);
		{
			MYSQLND_MS_LIST_DATA * el, ** el_pp;
			zend_llist_position	pos;
			/* master list */
			param++;
			MS_ARRAY(args[param]);
			for (el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(master_list, &pos); el_pp && (el = *el_pp) && el->conn;
					el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(master_list, &pos))
			{
				if (CONN_GET_STATE(el->conn) == CONN_ALLOCED) {
					/* lazy */
					add_next_index_stringl(args[param], el->emulated_scheme, el->emulated_scheme_len, 1);
				} else {
					add_next_index_stringl(args[param], el->conn->scheme, el->conn->scheme_len, 1);
				}
			}

			/* slave list*/
			param++;
			MS_ARRAY(args[param]);
			if (slave_list) {
				for (el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(slave_list, &pos); el_pp && (el = *el_pp) && el->conn;
						el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(slave_list, &pos))
				{
					if (CONN_GET_STATE(el->conn) == CONN_ALLOCED) {
						/* lazy */
						add_next_index_stringl(args[param], el->emulated_scheme, el->emulated_scheme_len, 1);
					} else {
						add_next_index_stringl(args[param], el->conn->scheme, el->conn->scheme_len, 1);
					}
				}
			}
			/* last used connection */
			param++;
			MAKE_STD_ZVAL(args[param]);
			if (stgy->last_used_conn) {
				ZVAL_STRING(args[param], stgy->last_used_conn->scheme, 1);
			} else {
				ZVAL_NULL(args[param]);
			}

			/* in transaction */
			param++;
			MAKE_STD_ZVAL(args[param]);
			if (stgy->in_transaction) {
				ZVAL_TRUE(args[param]);
			} else {
				ZVAL_FALSE(args[param]);
			}
		}

		retval = mysqlnd_ms_call_handler(MYSQLND_MS_G(user_pick_server), param + 1, args, FALSE /* we will destroy later */ TSRMLS_CC);
		if (retval) {
			if (Z_TYPE_P(retval) != IS_ARRAY) {
				DBG_ERR("The user returned no array");
			} else {
				do {
					HashPosition hash_pos;
					zval ** users_masters, ** users_slaves;
					DBG_INF("Checking data validity");
					/* Check data validity */
					{
						zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(retval), &hash_pos);
						if (SUCCESS != zend_hash_get_current_data_ex(Z_ARRVAL_P(retval), (void **)&users_masters, &hash_pos) ||
							Z_TYPE_PP(users_masters) != IS_ARRAY ||
							SUCCESS != zend_hash_move_forward_ex(Z_ARRVAL_P(retval), &hash_pos) ||
							SUCCESS != zend_hash_get_current_data_ex(Z_ARRVAL_P(retval), (void **)&users_slaves, &hash_pos) ||
							Z_TYPE_PP(users_slaves) != IS_ARRAY ||
							(
								0 == zend_hash_num_elements(Z_ARRVAL_PP(users_masters))
								&&
								0 == zend_hash_num_elements(Z_ARRVAL_PP(users_slaves))
							))
						{
							DBG_ERR("Error in validity");
							break;
						}
					}
					/* convert to long and sort */
					DBG_INF("Converting and sorting");
					{
						zval ** selected_server;
						/* convert to longs and sort */
						zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(users_masters), &hash_pos);
						while (SUCCESS == zend_hash_get_current_data_ex(Z_ARRVAL_PP(users_masters), (void **)&selected_server, &hash_pos)) {
							convert_to_long_ex(selected_server);
							zend_hash_move_forward_ex(Z_ARRVAL_PP(users_masters), &hash_pos);
						}
						if (FAILURE == zend_hash_sort(Z_ARRVAL_PP(users_masters), zend_qsort, my_long_compare, 1 TSRMLS_CC)) {
							DBG_ERR("Error while sorting the master list");
							break;
						}
						
						/* convert to longs and sort */
						zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(users_masters), &hash_pos);
						while (SUCCESS == zend_hash_get_current_data_ex(Z_ARRVAL_PP(users_slaves), (void **)&selected_server, &hash_pos)) {
							convert_to_long_ex(selected_server);
							zend_hash_move_forward_ex(Z_ARRVAL_PP(users_slaves), &hash_pos);
						}
						if (FAILURE == zend_hash_sort(Z_ARRVAL_PP(users_slaves), zend_qsort, my_long_compare, 1 TSRMLS_CC)) {
							DBG_ERR("Error while sorting the slave list");
							break;
						}
					}
					DBG_INF("Extracting into the supplied lists");
					/* extract into llists */
					{
						unsigned int pass;
						zval ** selected_server;

						for (pass = 0; pass < 2; pass++) {
							long i = 0;
							zend_llist_position	list_pos;
							zend_llist * in_list = (pass == 0)? master_list : slave_list;
							zend_llist * out_list = (pass == 0)? selected_masters : selected_slaves;
							MYSQLND_MS_LIST_DATA ** el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(in_list, &list_pos);
							MYSQLND_MS_LIST_DATA * el = el_pp? *el_pp : NULL;
							HashTable * conn_hash = (pass == 0)? Z_ARRVAL_PP(users_masters):Z_ARRVAL_PP(users_slaves);

							DBG_INF_FMT("pass=%u", pass);
							zend_hash_internal_pointer_reset_ex(conn_hash, &hash_pos);
							while (SUCCESS == zend_hash_get_current_data_ex(conn_hash, (void **)&selected_server, &hash_pos)) {
								if (Z_LVAL_PP(selected_server) >= 0) {
									long server_id = Z_LVAL_PP(selected_server);
									DBG_INF_FMT("i=%ld server_id=%ld llist_count=%d", i, server_id, zend_llist_count(in_list));
									if (server_id >= zend_llist_count(in_list)) {
										DBG_ERR("server_id too big, skipping and breaking");
										break; /* skip impossible indices */
									}
									while (i < server_id) {
										el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(in_list, &list_pos);
										el = el_pp ? *el_pp : NULL;
										i++;
									}
									if (el && el->conn) {
										DBG_INF_FMT("gotcha. adding server_id=%ld", server_id);
										/*
										  This will copy the whole structure, not the pointer.
										  This is wanted!!
										*/
										zend_llist_add_element(out_list, &el);
									}
								}
								zend_hash_move_forward_ex(conn_hash, &hash_pos);
							}
						}
						DBG_INF_FMT("count(master_list)=%d", zend_llist_count(selected_masters));
						DBG_INF_FMT("count(slave_list)=%d", zend_llist_count(selected_slaves));

						ret = PASS;
					}
				} while (0);
			}
			zval_ptr_dtor(&retval);
		}
		/* destroy the params */
		{
			unsigned int i;
			for (i = 0; i <= param; i++) {
				zval_ptr_dtor(&args[i]);
			}
		}
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
