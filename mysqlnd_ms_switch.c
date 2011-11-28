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
#include "ext/standard/info.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_priv.h"
#if PHP_VERSION_ID >= 50400
#include "ext/mysqlnd/mysqlnd_ext_plugin.h"
#endif
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
#include "mysqlnd_ms_filter_user.h"
#include "mysqlnd_ms_filter_random.h"
#include "mysqlnd_ms_filter_round_robin.h"
#include "mysqlnd_ms_filter_table_partition.h"
#include "mysqlnd_ms_filter_qos.h"

#include "mysqlnd_ms_switch.h"

typedef MYSQLND_MS_FILTER_DATA * (*func_specific_ctor)(struct st_mysqlnd_ms_config_json_entry * section,
													   MYSQLND_ERROR_INFO * error_info, zend_bool persistent TSRMLS_DC);


/* {{{ roundrobin_specific_dtor */
static void
rr_specific_dtor(struct st_mysqlnd_ms_filter_data * pDest TSRMLS_DC)
{
	MYSQLND_MS_FILTER_RR_DATA * filter = (MYSQLND_MS_FILTER_RR_DATA *) pDest;
	DBG_ENTER("rr_specific_dtor");

	zend_hash_destroy(&filter->master_context);
	zend_hash_destroy(&filter->slave_context);
	mnd_pefree(filter, filter->parent.persistent);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ roundrobin_specific_ctor */
static MYSQLND_MS_FILTER_DATA *
rr_specific_ctor(struct st_mysqlnd_ms_config_json_entry * section, MYSQLND_ERROR_INFO * error_info, zend_bool persistent TSRMLS_DC)
{
	MYSQLND_MS_FILTER_RR_DATA * ret;
	DBG_ENTER("rr_specific_ctor");
	DBG_INF_FMT("section=%p", section);
	/* section could be NULL! */
	ret = mnd_pecalloc(1, sizeof(MYSQLND_MS_FILTER_RR_DATA), persistent);
	if (ret) {
		ret->parent.specific_dtor = rr_specific_dtor;
		zend_hash_init(&ret->master_context, 4, NULL/*hash*/, NULL/*dtor*/, persistent);
		zend_hash_init(&ret->slave_context, 4, NULL/*hash*/, NULL/*dtor*/, persistent);
	}
	DBG_RETURN((MYSQLND_MS_FILTER_DATA *) ret);
}
/* }}} */

/* {{{ random_specific_dtor */
static void
random_specific_dtor(struct st_mysqlnd_ms_filter_data * pDest TSRMLS_DC)
{
	MYSQLND_MS_FILTER_RANDOM_DATA * filter = (MYSQLND_MS_FILTER_RANDOM_DATA *) pDest;
	DBG_ENTER("random_specific_dtor");

	zend_hash_destroy(&filter->sticky.master_context);
	zend_hash_destroy(&filter->sticky.slave_context);
	mnd_pefree(filter, filter->parent.persistent);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ random_specific_ctor */
static MYSQLND_MS_FILTER_DATA *
random_specific_ctor(struct st_mysqlnd_ms_config_json_entry * section, MYSQLND_ERROR_INFO * error_info, zend_bool persistent TSRMLS_DC)
{
	MYSQLND_MS_FILTER_RANDOM_DATA * ret;
	DBG_ENTER("random_specific_ctor");
	DBG_INF_FMT("section=%p", section);
	ret = mnd_pecalloc(1, sizeof(MYSQLND_MS_FILTER_RANDOM_DATA), persistent);
	if (ret) {
		ret->parent.specific_dtor = random_specific_dtor;

		/* section could be NULL! */
		if (section) {
			zend_bool value_exists = FALSE, is_list_value = FALSE;
			char * once_value = mysqlnd_ms_config_json_string_from_section(section, PICK_ONCE, sizeof(PICK_ONCE) - 1, 0,
																		   &value_exists, &is_list_value TSRMLS_CC);
			if (value_exists && once_value) {
				ret->sticky.once = !mysqlnd_ms_config_json_string_is_bool_false(once_value);
				mnd_efree(once_value);
			}
		} else {
			 /*
			   Stickiness by default when no filters section in the config
			   Implies NULL passed to this ctor.
			 */
			ret->sticky.once = TRUE;
		}
		DBG_INF_FMT("sticky=%d", ret->sticky.once);
		/* XXX: this could be initialized only in case of ONCE */
		zend_hash_init(&ret->sticky.master_context, 4, NULL/*hash*/, NULL/*dtor*/, persistent);
		zend_hash_init(&ret->sticky.slave_context, 4, NULL/*hash*/, NULL/*dtor*/, persistent);
	}
	DBG_RETURN((MYSQLND_MS_FILTER_DATA *) ret);
}
/* }}} */


/* {{{ user_specific_dtor */
static void
user_specific_dtor(struct st_mysqlnd_ms_filter_data * pDest TSRMLS_DC)
{
	MYSQLND_MS_FILTER_USER_DATA * filter = (MYSQLND_MS_FILTER_USER_DATA *) pDest;
	DBG_ENTER("user_specific_dtor");

	if (filter->user_callback) {
		zval_ptr_dtor(&filter->user_callback);
	}
	mnd_pefree(filter, filter->parent.persistent);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ user_specific_ctor */
static MYSQLND_MS_FILTER_DATA *
user_specific_ctor(struct st_mysqlnd_ms_config_json_entry * section, MYSQLND_ERROR_INFO * error_info, zend_bool persistent TSRMLS_DC)
{
	MYSQLND_MS_FILTER_USER_DATA * ret;
	DBG_ENTER("user_specific_ctor");
	DBG_INF_FMT("section=%p", section);
	if (section) {
		ret = mnd_pecalloc(1, sizeof(MYSQLND_MS_FILTER_USER_DATA), persistent);

		if (ret) {
			zend_bool value_exists = FALSE, is_list_value = FALSE;
			char * callback;

			ret->parent.specific_dtor = user_specific_dtor;

			callback = mysqlnd_ms_config_json_string_from_section(section, SECT_USER_CALLBACK, sizeof(SECT_USER_CALLBACK) - 1, 0,
																  &value_exists, &is_list_value TSRMLS_CC);

			if (value_exists) {
				zval * zv;
				char * c_name;

				MAKE_STD_ZVAL(zv);
				ZVAL_STRING(zv, callback, 1);
				mnd_efree(callback);
				ret->user_callback = zv;
				ret->callback_valid = zend_is_callable(zv, 0, &c_name TSRMLS_CC);
				DBG_INF_FMT("name=%s valid=%d", c_name, ret->callback_valid);
				efree(c_name);
			} else {
				mnd_pefree(ret, persistent);
				php_error_docref(NULL TSRMLS_CC, E_ERROR,
									 MYSQLND_MS_ERROR_PREFIX " Error by creating filter 'user', can't find section '%s' . Stopping.", SECT_USER_CALLBACK);
			}
		}
	}
	DBG_RETURN((MYSQLND_MS_FILTER_DATA *) ret);
}
/* }}} */


/* {{{ qos_specific_dtor */
static void
qos_specific_dtor(struct st_mysqlnd_ms_filter_data * pDest TSRMLS_DC)
{
	MYSQLND_MS_FILTER_QOS_DATA * filter = (MYSQLND_MS_FILTER_QOS_DATA *) pDest;
	DBG_ENTER("qos_specific_dtor");
	mnd_pefree(filter, filter->parent.persistent);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ qos_specific_ctor */
static MYSQLND_MS_FILTER_DATA *
qos_specific_ctor(struct st_mysqlnd_ms_config_json_entry * section, MYSQLND_ERROR_INFO * error_info, zend_bool persistent TSRMLS_DC)
{
	MYSQLND_MS_FILTER_QOS_DATA * ret;
	DBG_ENTER("qos_specific_ctor");
	DBG_INF_FMT("section=%p", section);
	if (section) {
		ret = mnd_pecalloc(1, sizeof(MYSQLND_MS_FILTER_QOS_DATA), persistent);


		if (ret) {
			zend_bool value_exists = FALSE, is_list_value = FALSE;
			char * service;

			ret->parent.specific_dtor = qos_specific_dtor;
			ret->consistency = CONSISTENCY_LAST_ENUM_ENTRY;

			service = mysqlnd_ms_config_json_string_from_section(section, SECT_QOS_STRONG, sizeof(SECT_QOS_STRONG) - 1, 0,
																  &value_exists, &is_list_value TSRMLS_CC);
			if (value_exists) {
			  DBG_INF("strong consistency");
			  mnd_efree(service);
			  ret->consistency = CONSISTENCY_STRONG;
			}

			service = mysqlnd_ms_config_json_string_from_section(section, SECT_QOS_SESSION, sizeof(SECT_QOS_SESSION) - 1, 0,
																  &value_exists, &is_list_value TSRMLS_CC);
			if (value_exists) {
			  DBG_INF("session consistency");
			  mnd_efree(service);
			  if (ret->consistency != CONSISTENCY_LAST_ENUM_ENTRY) {
				mnd_pefree(ret, persistent);
				php_error_docref(NULL TSRMLS_CC, E_ERROR,
									 MYSQLND_MS_ERROR_PREFIX " Error by creating filter '%s', '%s' clashes with previous setting. Stopping.", PICK_QOS, SECT_QOS_SESSION);
			  } else {
				ret->consistency = CONSISTENCY_SESSION;
			  }
			}

			service = mysqlnd_ms_config_json_string_from_section(section, SECT_QOS_EVENTUAL, sizeof(SECT_QOS_EVENTUAL) - 1, 0,
																  &value_exists, &is_list_value TSRMLS_CC);
			if (value_exists) {
			  DBG_INF("eventual consistency");
			  mnd_efree(service);
			  if (ret->consistency != CONSISTENCY_LAST_ENUM_ENTRY) {
				mnd_pefree(ret, persistent);
				php_error_docref(NULL TSRMLS_CC, E_ERROR,
									 MYSQLND_MS_ERROR_PREFIX " Error by creating filter '%s', '%s' clashes with previous setting. Stopping.", PICK_QOS, SECT_QOS_EVENTUAL);
			  } else {
				ret->consistency = CONSISTENCY_EVENTUAL;
			  }
			}

			if ((ret->consistency != CONSISTENCY_STRONG) &&
				(ret->consistency != CONSISTENCY_SESSION) &&
				(ret->consistency != CONSISTENCY_EVENTUAL))
				{
				mnd_pefree(ret, persistent);
				php_error_docref(NULL TSRMLS_CC, E_ERROR,
									 MYSQLND_MS_ERROR_PREFIX " Error by creating filter '%s', can't find section '%s', '%s' or '%s' . Stopping.", PICK_QOS, SECT_QOS_STRONG, SECT_QOS_SESSION, SECT_QOS_EVENTUAL);
			}

		}
	}

	DBG_RETURN((MYSQLND_MS_FILTER_DATA *) ret);
}
/* }}} */


#ifdef MYSQLND_MS_HAVE_FILTER_TABLE_PARTITION
/* {{{ table_specific_dtor */
static void
table_specific_dtor(struct st_mysqlnd_ms_filter_data * pDest TSRMLS_DC)
{
	MYSQLND_MS_FILTER_TABLE_DATA * filter = (MYSQLND_MS_FILTER_TABLE_DATA *) pDest;
	DBG_ENTER("table_specific_dtor");

	zend_hash_destroy(&filter->master_rules);
	zend_hash_destroy(&filter->slave_rules);
	mnd_pefree(filter, filter->parent.persistent);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ table_specific_ctor */
static MYSQLND_MS_FILTER_DATA *
table_specific_ctor(struct st_mysqlnd_ms_config_json_entry * section, MYSQLND_ERROR_INFO * error_info, zend_bool persistent TSRMLS_DC)
{
	MYSQLND_MS_FILTER_TABLE_DATA * ret;
	DBG_ENTER("table_specific_ctor");
	DBG_INF_FMT("section=%p", section);
	ret = mnd_pecalloc(1, sizeof(MYSQLND_MS_FILTER_TABLE_DATA), persistent);
	if (ret) {
		do {
			ret->parent.specific_dtor = table_specific_dtor;
			zend_hash_init(&ret->master_rules, 4, NULL/*hash*/, mysqlnd_ms_filter_ht_dtor/*dtor*/, persistent);
			zend_hash_init(&ret->slave_rules, 4, NULL/*hash*/, mysqlnd_ms_filter_ht_dtor/*dtor*/, persistent);
			if (FAIL == mysqlnd_ms_load_table_filters(&ret->master_rules, &ret->slave_rules, section, error_info, persistent TSRMLS_CC)) {
				table_specific_dtor((MYSQLND_MS_FILTER_DATA *)ret TSRMLS_CC);
				ret = NULL;
				break;
			}
		} while (0);
	}
	DBG_RETURN((MYSQLND_MS_FILTER_DATA *) ret);
}
/* }}} */
#endif /* MYSQLND_MS_HAVE_FILTER_TABLE_PARTITION */

struct st_specific_ctor_with_name
{
	const char * name;
	size_t name_len;
	func_specific_ctor ctor;
	enum mysqlnd_ms_server_pick_strategy pick_type;
	zend_bool multi_filter;
};


/* TODO: write copy ctors */
static const struct st_specific_ctor_with_name specific_ctors[] =
{
	{PICK_RROBIN,		sizeof(PICK_RROBIN) - 1,		rr_specific_ctor,		SERVER_PICK_RROBIN,		FALSE},
	{PICK_RANDOM,		sizeof(PICK_RANDOM) - 1,		random_specific_ctor,	SERVER_PICK_RANDOM,		FALSE},
	{PICK_USER,			sizeof(PICK_USER) - 1,			user_specific_ctor,		SERVER_PICK_USER,		FALSE},
	{PICK_USER_MULTI,	sizeof(PICK_USER_MULTI) - 1,	user_specific_ctor,		SERVER_PICK_USER_MULTI,	TRUE},
#ifdef MYSQLND_MS_HAVE_FILTER_TABLE_PARTITION
	{PICK_TABLE,		sizeof(PICK_TABLE) - 1,			table_specific_ctor,	SERVER_PICK_TABLE,		TRUE},
#endif
	{PICK_QOS,			sizeof(PICK_QOS) -1,			qos_specific_ctor,		SERVER_PICK_QOS,		TRUE},
	{NULL,				0,								NULL, 					SERVER_PICK_LAST_ENUM_ENTRY}
};


/* {{{ get_element_ptr */
static void mysqlnd_ms_get_element_ptr(void * d, void * arg TSRMLS_DC)
{
	MYSQLND_MS_LIST_DATA * data = d? *(MYSQLND_MS_LIST_DATA **) d : NULL ;
	char ptr_buf[SIZEOF_SIZE_T + 1];
	smart_str * context = (smart_str *) arg;
	if (data) {
#if SIZEOF_SIZE_T == 8
		int8store(ptr_buf, (size_t) data->conn);
#elif SIZEOF_SIZE_T == 4
		int4store(ptr_buf, (size_t) data->conn);
#else
#error Unknown platform
#endif
		ptr_buf[SIZEOF_SIZE_T] = '\0';
		smart_str_appendl(context, ptr_buf, SIZEOF_SIZE_T);
	}
}
/* }}} */


/* {{{ mysqlnd_ms_get_fingerprint */
void
mysqlnd_ms_get_fingerprint(smart_str * context, zend_llist * list TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms_get_fingerprint");
	zend_llist_apply_with_argument(list, mysqlnd_ms_get_element_ptr, context TSRMLS_CC);
	smart_str_appendc(context, '\0');
	DBG_INF_FMT("len=%d", context->len);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_filter_list_dtor */
static void
mysqlnd_ms_filter_list_dtor(void * pDest)
{
	MYSQLND_MS_FILTER_DATA * filter = *(MYSQLND_MS_FILTER_DATA **) pDest;
	TSRMLS_FETCH();
	DBG_ENTER("mysqlnd_ms_filter_list_dtor");
	DBG_INF_FMT("%p", filter);
	if (filter) {
		zend_bool pers = filter->persistent;
		DBG_INF_FMT("name=%s dtor=%p", filter->name? filter->name:"n/a", filter->specific_dtor);
		if (filter->name) {
			mnd_pefree(filter->name, pers);
		}
		if (filter->specific_dtor) {
			filter->specific_dtor(filter TSRMLS_CC);
		} else {
			mnd_pefree(filter, pers);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_lb_strategy_setup */
void
mysqlnd_ms_lb_strategy_setup(struct mysqlnd_ms_lb_strategies * strategies,
							 struct st_mysqlnd_ms_config_json_entry * the_section,
							 MYSQLND_ERROR_INFO * error_info TSRMLS_DC)
{
	zend_bool value_exists = FALSE, is_list_value = FALSE;

	DBG_ENTER("mysqlnd_ms_lb_strategy_setup");
	{
		char * failover_strategy =
			mysqlnd_ms_config_json_string_from_section(the_section, FAILOVER_NAME, sizeof(FAILOVER_NAME) - 1, 0,
													   &value_exists, &is_list_value TSRMLS_CC);

		strategies->failover_strategy = DEFAULT_FAILOVER_STRATEGY;

		if (value_exists && failover_strategy) {
			if (!strncasecmp(FAILOVER_DISABLED, failover_strategy, sizeof(FAILOVER_DISABLED) - 1)) {
				strategies->failover_strategy = SERVER_FAILOVER_DISABLED;
			} else if (!strncasecmp(FAILOVER_MASTER, failover_strategy, sizeof(FAILOVER_MASTER) - 1)) {
				strategies->failover_strategy = SERVER_FAILOVER_MASTER;
			}
			mnd_efree(failover_strategy);
		}
	}

	{
		char * master_on_write =
			mysqlnd_ms_config_json_string_from_section(the_section, MASTER_ON_WRITE_NAME, sizeof(MASTER_ON_WRITE_NAME) - 1, 0,
													   &value_exists, &is_list_value TSRMLS_CC);

		strategies->mysqlnd_ms_flag_master_on_write = FALSE;
		strategies->master_used = FALSE;

		if (value_exists && master_on_write) {
			DBG_INF("Master on write active");
			strategies->mysqlnd_ms_flag_master_on_write = !mysqlnd_ms_config_json_string_is_bool_false(master_on_write);
			mnd_efree(master_on_write);
		}
	}

	{
		char * trx_strategy =
			mysqlnd_ms_config_json_string_from_section(the_section, TRX_STICKINESS_NAME, sizeof(TRX_STICKINESS_NAME) - 1, 0,
													   &value_exists, &is_list_value TSRMLS_CC);

		strategies->trx_stickiness_strategy = DEFAULT_TRX_STICKINESS_STRATEGY;
		strategies->in_transaction = FALSE;

		if (value_exists && trx_strategy) {
#if PHP_VERSION_ID >= 50399
			if (!strncasecmp(TRX_STICKINESS_MASTER, trx_strategy, sizeof(TRX_STICKINESS_MASTER) - 1)) {
				DBG_INF("Transaction strickiness strategy = master");
				strategies->trx_stickiness_strategy = TRX_STICKINESS_STRATEGY_MASTER;
			}
#else
			SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE,
							 MYSQLND_MS_ERROR_PREFIX " trx_stickiness_strategy is not supported before PHP 5.3.99");
			php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " trx_stickiness_strategy is not supported before PHP 5.3.99");
#endif
			mnd_efree(trx_strategy);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */

#if PHP_VERSION_ID > 50399
/* {{{ mysqlnd_ms_remove_qos_filter */
static int
mysqlnd_ms_remove_qos_filter(void * element, void * data) {
	MYSQLND_MS_FILTER_DATA * filter = *(MYSQLND_MS_FILTER_DATA **)element;
	return (filter->pick_type == SERVER_PICK_QOS) ? 1 : 0;
}
/* }}} */


/* {{{ mysqlnd_ms_section_filters_prepend_qos */
enum_func_status
mysqlnd_ms_section_filters_prepend_qos(MYSQLND * proxy_conn,
										enum mysqlnd_ms_filter_qos_consistency consistency,
										enum mysqlnd_ms_filter_qos_option option,
										long option_value TSRMLS_DC) {
  	MYSQLND_MS_CONN_DATA ** conn_data;
	enum_func_status ret = FAIL;
	/* not sure... */
	zend_bool persistent = proxy_conn->persistent;

	DBG_ENTER("mysqlnd_ms_section_filters_prepend_qos");

	conn_data = (MYSQLND_MS_CONN_DATA **) mysqlnd_plugin_get_plugin_connection_data_data(proxy_conn->data, mysqlnd_ms_plugin_id);
	DBG_INF_FMT("conn_data=%p *conn_data=%p", conn_data, conn_data? *conn_data : NULL);

	if (conn_data && *conn_data) {
		struct mysqlnd_ms_lb_strategies * stgy = &(*conn_data)->stgy;
		zend_llist * filters = stgy->filters;
		MYSQLND_MS_FILTER_DATA * new_filter_entry = NULL;
		MYSQLND_MS_FILTER_QOS_DATA * new_qos_filter = NULL;

		/* remove all existing QOS filters */
		zend_llist_del_element(filters, NULL, mysqlnd_ms_remove_qos_filter);

		/* new QOS filter comes first */
		new_qos_filter = mnd_pecalloc(1, sizeof(MYSQLND_MS_FILTER_QOS_DATA), persistent);
		new_qos_filter->parent.specific_dtor = qos_specific_dtor;
		new_qos_filter->consistency = consistency;
		new_qos_filter->option = option;
		new_qos_filter->option_value = option_value;
		new_filter_entry = (MYSQLND_MS_FILTER_DATA *)new_qos_filter;
		new_filter_entry->persistent = persistent;
		new_filter_entry->name = mnd_pestrndup(PICK_QOS, sizeof(PICK_QOS) -1, persistent);
		new_filter_entry->name_len = sizeof(PICK_QOS) -1;
		new_filter_entry->pick_type = (enum mysqlnd_ms_server_pick_strategy)SERVER_PICK_QOS;
		new_filter_entry->multi_filter = TRUE;
		zend_llist_prepend_element(filters, &new_filter_entry);
	}

	ret = PASS;
	DBG_RETURN(ret);
}
/* }}} */
#endif

/* {{{ mysqlnd_ms_section_filters_add_filter */
static MYSQLND_MS_FILTER_DATA *
mysqlnd_ms_section_filters_add_filter(zend_llist * filters,
									  struct st_mysqlnd_ms_config_json_entry * filter_config,
									  const char * const filter_name, const size_t filter_name_len,
									  const zend_bool persistent,
									  MYSQLND_ERROR_INFO * error_info TSRMLS_DC)
{
	MYSQLND_MS_FILTER_DATA * new_filter_entry = NULL;

	DBG_ENTER("mysqlnd_ms_section_filters_add_filter");
	if (filter_name && filter_name_len) {
		unsigned int i = 0;
		/* find specific ctor, if available */
		while (specific_ctors[i].name) {
			DBG_INF_FMT("current_ctor->name=%s", specific_ctors[i].name);
			if (!strcasecmp(specific_ctors[i].name, filter_name)) {
				if (zend_llist_count(filters)) {
					zend_llist_position llist_pos;
					MYSQLND_MS_FILTER_DATA * prev = *(MYSQLND_MS_FILTER_DATA **)zend_llist_get_last_ex(filters, &llist_pos);
					if (FALSE == prev->multi_filter) {
						char error_buf[128];
						snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Error while creating filter '%s' . "
								 "Non-multi filter '%s' already created. Stopping", filter_name, prev->name);
						error_buf[sizeof(error_buf) - 1] = '\0';
						SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
						DBG_RETURN(NULL);
					}
				}
				if (specific_ctors[i].ctor) {
					new_filter_entry = specific_ctors[i].ctor(filter_config, error_info, persistent TSRMLS_CC);
					if (!new_filter_entry) {
						char error_buf[128];
						snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Error while creating filter '%s' . Stopping", filter_name);
						error_buf[sizeof(error_buf) - 1] = '\0';
						SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
						DBG_RETURN(NULL);
					}
				} else {
					new_filter_entry = mnd_pecalloc(1, sizeof(MYSQLND_MS_FILTER_DATA), persistent);
				}
				new_filter_entry->persistent = persistent;
				new_filter_entry->name = mnd_pestrndup(filter_name, filter_name_len, persistent);
				new_filter_entry->name_len = filter_name_len;
				new_filter_entry->pick_type = specific_ctors[i].pick_type;
				new_filter_entry->multi_filter = specific_ctors[i].multi_filter;
				zend_llist_add_element(filters, &new_filter_entry);
				break;
			}
			++i;
		}
	}
	if (!new_filter_entry) {
		char error_buf[128];
		snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Unknown filter '%s' . Stopping", filter_name);
		error_buf[sizeof(error_buf) - 1] = '\0';
		SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
	}
	DBG_RETURN(new_filter_entry);
}
/* }}} */


/* {{{ mysqlnd_ms_load_section_filters */
zend_llist *
mysqlnd_ms_load_section_filters(struct st_mysqlnd_ms_config_json_entry * section, MYSQLND_ERROR_INFO * error_info,
								zend_bool persistent TSRMLS_DC)
{
	zend_llist * ret = NULL;
	DBG_ENTER("mysqlnd_ms_load_section_filters");

	if (section) {
		ret = mnd_pemalloc(sizeof(zend_llist), persistent);
	}
	if (ret) {
		zend_bool section_exists;
		struct st_mysqlnd_ms_config_json_entry * filters_section =
				mysqlnd_ms_config_json_sub_section(section, SECT_FILTER_NAME, sizeof(SECT_FILTER_NAME) - 1, &section_exists TSRMLS_CC);

		zend_llist_init(ret, sizeof(MYSQLND_MS_FILTER_DATA *), (llist_dtor_func_t) mysqlnd_ms_filter_list_dtor /*dtor*/, persistent);
		DBG_INF_FMT("normal filters section =%d", section_exists && filters_section);
		switch (section_exists && filters_section? 1:0) {
			case 1:
				do {
					char * filter_name = NULL;
					size_t filter_name_len = 0;
					ulong filter_int_name;
					struct st_mysqlnd_ms_config_json_entry * current_filter =
							mysqlnd_ms_config_json_next_sub_section(filters_section, &filter_name, &filter_name_len, &filter_int_name TSRMLS_CC);

					if (!current_filter || !filter_name || !filter_name_len) {
						if (current_filter) {
							char error_buf[128];
							if (filter_name && !filter_name_len) {
								snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Error loading filters. Filter with empty name found");
							} else if (FALSE == mysqlnd_ms_config_json_section_is_list(current_filter TSRMLS_CC)) { /* filter_int_name */
								filter_name =
									mysqlnd_ms_config_json_string_from_section(filters_section, NULL, 0,
																			   filter_int_name, NULL, NULL TSRMLS_CC);
								filter_name_len = strlen(filter_name);
								DBG_INF_FMT("%d was found as key, the value is %s", filter_int_name, filter_name);
								if (NULL == mysqlnd_ms_section_filters_add_filter(ret, current_filter, filter_name, filter_name_len,
																				  persistent, error_info TSRMLS_CC)) {
									mnd_pefree(filter_name, 0);
									goto err;
								}
								mnd_pefree(filter_name, 0);
								continue;
							} else {
								snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Unknown filter '%d' . Stopping", filter_int_name);
							}
							error_buf[sizeof(error_buf) - 1] = '\0';
							SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
							goto err;
						}
						DBG_INF("no next sub-section");
						break;
					}
					if (NULL == mysqlnd_ms_section_filters_add_filter(ret, current_filter, filter_name, filter_name_len,
																	  persistent, error_info TSRMLS_CC)) {
						goto err;
					}
				} while (1);
				if (zend_llist_count(ret)) {
					zend_llist_position llist_pos;
					MYSQLND_MS_FILTER_DATA * prev = *(MYSQLND_MS_FILTER_DATA **)zend_llist_get_last_ex(ret, &llist_pos);
					if (FALSE != prev->multi_filter) {
						char error_buf[128];
						snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Error in configuration. "
							"Last filter is multi filter. Needs to be non-multi one. Stopping");
						error_buf[sizeof(error_buf) - 1] = '\0';
						SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
						goto err;
					}
					break;
				}
				/* fall-through */
			case 0:
			{
				/* setup the default */
				unsigned int i = 0;
				DBG_INF("No section, using defaults");
				while (specific_ctors[i].name) {
					if (DEFAULT_PICK_STRATEGY == specific_ctors[i].pick_type) {
						DBG_INF_FMT("Found default pick strategy : %s", specific_ctors[i].name);
						if (NULL == mysqlnd_ms_section_filters_add_filter(ret, NULL, specific_ctors[i].name, specific_ctors[i].name_len,
																		  persistent, error_info TSRMLS_CC)) {
							char error_buf[128];
							snprintf(error_buf, sizeof(error_buf),
									MYSQLND_MS_ERROR_PREFIX " Can't load default filter '%d' . Stopping", specific_ctors[i].name);
							error_buf[sizeof(error_buf) - 1] = '\0';
							SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
							goto err;
						}
						break;
					}
					++i;
				}
			}
		}
	}
	DBG_RETURN(ret);
err:
	zend_llist_clean(ret);
	mnd_pefree(ret, persistent);
	DBG_RETURN(NULL);
}
/* }}} */


/* {{{ mysqlnd_ms_query_is_select */
PHPAPI enum enum_which_server
mysqlnd_ms_query_is_select(const char * query, size_t query_len, zend_bool * forced TSRMLS_DC)
{
	enum enum_which_server ret = USE_MASTER;
	struct st_qc_token_and_value token = {0};
	struct st_mysqlnd_query_scanner * scanner;
	DBG_ENTER("mysqlnd_ms_query_is_select");
	*forced = FALSE;
	if (!query) {
		DBG_RETURN(USE_MASTER);
	}

	scanner = mysqlnd_qp_create_scanner(TSRMLS_C);
	mysqlnd_qp_set_string(scanner, query, query_len TSRMLS_CC);
	token = mysqlnd_qp_get_token(scanner TSRMLS_CC);
	DBG_INF_FMT("token=COMMENT? = %d", token.token == QC_TOKEN_COMMENT);
	while (token.token == QC_TOKEN_COMMENT) {
		if (!strncasecmp(Z_STRVAL(token.value), MASTER_SWITCH, sizeof(MASTER_SWITCH) - 1)) {
			DBG_INF("forced master");
			ret = USE_MASTER;
			*forced = TRUE;
			MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_MASTER_FORCED);
		} else if (!strncasecmp(Z_STRVAL(token.value), SLAVE_SWITCH, sizeof(SLAVE_SWITCH) - 1)) {
			DBG_INF("forced slave");
			if (MYSQLND_MS_G(disable_rw_split)) {
				ret = USE_MASTER;
			} else {
				ret = USE_SLAVE;
				MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_SLAVE_FORCED);
			}
			*forced = TRUE;
		} else if (!strncasecmp(Z_STRVAL(token.value), LAST_USED_SWITCH, sizeof(LAST_USED_SWITCH) - 1)) {
			DBG_INF("forced last used");
			ret = USE_LAST_USED;
			*forced = TRUE;
			MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_LAST_USED_FORCED);
#ifdef ALL_SERVER_DISPATCH
		} else if (!strncasecmp(Z_STRVAL(token.value), ALL_SERVER_SWITCH, sizeof(ALL_SERVER_SWITCH) - 1)) {
			DBG_INF("forced all server");
			ret = USE_ALL;
			*forced = TRUE;
#endif
		}
		zval_dtor(&token.value);
		token = mysqlnd_qp_get_token(scanner TSRMLS_CC);
	}
	if (*forced == FALSE) {
	  	if (MYSQLND_MS_G(disable_rw_split)) {
			ret = USE_MASTER;
		} else if (token.token == QC_TOKEN_SELECT) {
			MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_SLAVE_GUESS);
			ret = USE_SLAVE;
#ifdef ALL_SERVER_DISPATCH
		} else if (token.token == QC_TOKEN_SET) {
			ret = USE_ALL;
#endif
		} else {
			MYSQLND_MS_INC_STATISTIC(MS_STAT_USE_MASTER_GUESS);
			ret = USE_MASTER;
		}
	}
	zval_dtor(&token.value);
	mysqlnd_qp_free_scanner(scanner TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_select_servers_all */
enum_func_status
mysqlnd_ms_select_servers_all(zend_llist * master_list, zend_llist * slave_list,
							  zend_llist * selected_masters, zend_llist * selected_slaves TSRMLS_DC)
{
	enum_func_status ret = PASS;
	DBG_ENTER("mysqlnd_ms_select_servers_all");

	{
		zend_llist_position	pos;
		MYSQLND_MS_LIST_DATA * el, ** el_pp;
		if (zend_llist_count(master_list)) {
			for (el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(master_list, &pos); el_pp && (el = *el_pp) && el->conn;
					el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(master_list, &pos))
			{
				/*
				  This will copy the whole structure, not the pointer.
				  This is wanted!!
				*/
				zend_llist_add_element(selected_masters, &el);
			}
		}
		if (zend_llist_count(slave_list)) {
			for (el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(slave_list, &pos); el_pp && (el = *el_pp) && el->conn;
					el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(slave_list, &pos))
			{
				/*
				  This will copy the whole structure, not the pointer.
				  This is wanted!!
				*/
				zend_llist_add_element(selected_slaves, &el);
			}
		}
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_pick_server_ex */
MYSQLND_CONN_DATA *
mysqlnd_ms_pick_server_ex(MYSQLND_CONN_DATA * conn, const char * const query, const size_t query_len TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, conn);
	MYSQLND_CONN_DATA * connection = conn;
	DBG_ENTER("mysqlnd_ms_pick_server_ex");
	DBG_INF_FMT("conn_data=%p *conn_data=%p", conn_data, conn_data? *conn_data : NULL);

	if (conn_data && *conn_data) {
		struct mysqlnd_ms_lb_strategies * stgy = &(*conn_data)->stgy;
		zend_llist * filters = stgy->filters;
		zend_llist * master_list = &(*conn_data)->master_connections;
		zend_llist * slave_list = &(*conn_data)->slave_connections;
		zend_llist * selected_masters = NULL, * selected_slaves = NULL;
		zend_llist * output_masters = NULL, * output_slaves = NULL;
		MYSQLND_MS_FILTER_DATA * filter, ** filter_pp;
		zend_llist_position	pos;

		/* order of allocation and initialisation is important !*/
		/* 1. */
		selected_masters = mnd_pemalloc(sizeof(zend_llist), conn->persistent);
		if (!selected_masters) {
			goto end;
		}
		zend_llist_init(selected_masters, sizeof(MYSQLND_MS_LIST_DATA *), NULL /*dtor*/, conn->persistent);

		/* 2. */
		selected_slaves = mnd_pemalloc(sizeof(zend_llist), conn->persistent);
		if (!selected_slaves) {
			goto end;
		}
		zend_llist_init(selected_slaves, sizeof(MYSQLND_MS_LIST_DATA *), NULL /*dtor*/, conn->persistent);

		/* 3. */
		output_masters = mnd_pemalloc(sizeof(zend_llist), conn->persistent);
		if (!output_masters) {
			goto end;
		}
		zend_llist_init(output_masters, sizeof(MYSQLND_MS_LIST_DATA *), NULL /*dtor*/, conn->persistent);

		/* 4. */
		output_slaves = mnd_pemalloc(sizeof(zend_llist), conn->persistent);
		if (!output_slaves) {
			goto end;
		}
		zend_llist_init(output_slaves, sizeof(MYSQLND_MS_LIST_DATA *), NULL /*dtor*/, conn->persistent);

		mysqlnd_ms_select_servers_all(master_list, slave_list, selected_masters, selected_slaves TSRMLS_CC);
		connection = NULL;

		for (filter_pp = (MYSQLND_MS_FILTER_DATA **) zend_llist_get_first_ex(filters, &pos);
			 filter_pp && (filter = *filter_pp);
			 filter_pp = (MYSQLND_MS_FILTER_DATA **) zend_llist_get_next_ex(filters, &pos))
		{
			zend_bool multi_filter = FALSE;
			if (zend_llist_count(output_masters) || zend_llist_count(output_slaves)) {
				/* swap and clean */
				zend_llist * tmp_sel_masters = selected_masters;
				zend_llist * tmp_sel_slaves = selected_slaves;
				zend_llist_clean(selected_masters);
				zend_llist_clean(selected_slaves);
				selected_masters = output_masters;
				selected_slaves = output_slaves;
				output_masters = tmp_sel_masters;
				output_slaves = tmp_sel_slaves;
			}

			switch (filter->pick_type) {
				case SERVER_PICK_USER:
					connection = mysqlnd_ms_user_pick_server(filter, (*conn_data)->connect_host, query, query_len,
															 selected_masters, selected_slaves, stgy,
															 &MYSQLND_MS_ERROR_INFO(conn) TSRMLS_CC);
					break;
				case SERVER_PICK_USER_MULTI:
					multi_filter = TRUE;
					mysqlnd_ms_user_pick_multiple_server(filter, (*conn_data)->connect_host, query, query_len,
														 selected_masters, selected_slaves,
														 output_masters, output_slaves, stgy,
														 &MYSQLND_MS_ERROR_INFO(conn) TSRMLS_CC);
					break;
#ifdef MYSQLND_MS_HAVE_FILTER_TABLE_PARTITION
				case SERVER_PICK_TABLE:
					multi_filter = TRUE;
					mysqlnd_ms_choose_connection_table_filter(filter, query, query_len,
															  CONN_GET_STATE(conn) > CONN_ALLOCED?
															  	conn->connect_or_select_db:
															  	(*conn_data)->cred.db,
															  selected_masters, selected_slaves, output_masters, output_slaves,
															  stgy, &MYSQLND_MS_ERROR_INFO(conn) TSRMLS_CC);
					break;
#endif
				case SERVER_PICK_RANDOM:
					connection = mysqlnd_ms_choose_connection_random(filter, query, query_len, stgy, &MYSQLND_MS_ERROR_INFO(conn),
																	 selected_masters, selected_slaves, NULL TSRMLS_CC);
					break;
				case SERVER_PICK_RROBIN:
					connection = mysqlnd_ms_choose_connection_rr(filter, query, query_len, stgy, &MYSQLND_MS_ERROR_INFO(conn),
																 selected_masters, selected_slaves, NULL TSRMLS_CC);
					break;
				case SERVER_PICK_QOS:
					/* TODO: MS must not bail if slave or master list is empty */
					multi_filter = TRUE;
					mysqlnd_ms_qos_pick_server(filter, (*conn_data)->connect_host, query, query_len,
														 selected_masters, selected_slaves,
														 output_masters, output_slaves, stgy,
														 &MYSQLND_MS_ERROR_INFO(conn) TSRMLS_CC);
					break;
				default:
				{
					char error_buf[128];
					snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Unknown pick type");
					error_buf[sizeof(error_buf) - 1] = '\0';
					SET_CLIENT_ERROR(MYSQLND_MS_ERROR_INFO(conn), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
					php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "%s", error_buf);
				}
			}
			DBG_INF_FMT("out_masters_count=%d  out_slaves_count=%d", zend_llist_count(output_masters), zend_llist_count(output_slaves));
			/* if a multi-connection filter reduces the list to a single connection, then use this connection */
			if (!connection &&
				multi_filter == TRUE &&
				(1 == zend_llist_count(output_masters) + zend_llist_count(output_slaves)))
			{
				MYSQLND_MS_LIST_DATA ** el_pp;
				if (zend_llist_count(output_masters)) {
					el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first(output_masters);
				} else {
					el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first(output_slaves);
				}
				if (el_pp && (*el_pp)->conn) {
					MYSQLND_MS_LIST_DATA * element = *el_pp;
					if (CONN_GET_STATE(element->conn) == CONN_ALLOCED) {
						DBG_INF("Lazy connection, trying to connect...");
						/* lazy connection, connect now */

						if (PASS != mysqlnd_ms_lazy_connect(element, zend_llist_count(output_masters)? TRUE:FALSE TSRMLS_CC)) {
							DBG_INF_FMT("Using master connection "MYSQLND_LLU_SPEC"", element->conn->thread_id);
							connection = element->conn;
						}
					} else {
						connection = element->conn;
					}
				}
			}
			if (!connection && multi_filter == FALSE) {
				char error_buf[128];
				snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " No connection selected by the last filter");
				error_buf[sizeof(error_buf) - 1] = '\0';
				SET_CLIENT_ERROR(MYSQLND_MS_ERROR_INFO(conn), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_buf);
				goto end;
			}
			if (!connection && (0 == zend_llist_count(output_masters) && 0 == zend_llist_count(output_slaves))) {
				/* filtered everything out */
				if (SERVER_FAILOVER_MASTER == stgy->failover_strategy) {
					DBG_INF("FAILOVER");
					connection = conn;
				} else {
					char error_buf[128];
					snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Couldn't find the appropriate master connection. Something is wrong");
					error_buf[sizeof(error_buf) - 1] = '\0';
					DBG_ERR(error_buf);
					SET_CLIENT_ERROR(MYSQLND_MS_ERROR_INFO(conn), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_buf);
					goto end;
				}
			}
			if (connection) {
				/* filter till we get one, for now this is just a hack, not the ultimate solution */
				break;
			}
		}
		stgy->last_used_conn = connection;
end:
		if (selected_masters) {
			zend_llist_clean(selected_masters);
			mnd_pefree(selected_masters, conn->persistent);
		}
		if (selected_slaves) {
			zend_llist_clean(selected_slaves);
			mnd_pefree(selected_slaves, conn->persistent);
		}
		if (output_masters) {
			zend_llist_clean(output_masters);
			mnd_pefree(output_masters, conn->persistent);
		}
		if (output_slaves) {
			zend_llist_clean(output_slaves);
			mnd_pefree(output_slaves, conn->persistent);
		}
#if 0
		if (!connection) {
			connection = conn;
		}
#endif
	}

	DBG_RETURN(connection);
}
/* }}} */


#ifdef ALL_SERVER_DISPATCH
/* {{{ mysqlnd_ms_query_all */
static enum_func_status
mysqlnd_ms_query_all(MYSQLND * const proxy_conn, const char * query, unsigned int query_len,
					 zend_llist * master_connections, zend_llist * slave_connections TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	enum_func_status ret = PASS;

	DBG_ENTER("mysqlnd_ms_query_all");
	if (!conn_data || !*conn_data) {
		DBG_RETURN(MS_CALL_ORIGINAL_CONN_DATA_METHOD(query)(proxy_conn, query, query_len TSRMLS_CC));
	} else {
		zend_llist * lists[] = {NULL, &(*conn_data)->master_connections, &(*conn_data)->slave_connections, NULL};
		zend_llist ** list = lists;
		while (*++list) {
			zend_llist_position	pos;
			MYSQLND_MS_LIST_DATA * el, ** el_pp;
			/* search the list of easy handles hanging off the multi-handle */
			for (el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(*list, &pos); el_pp && (el = *el_pp) && el->conn;
					el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(*list, &pos))
			{
				if (PASS != MS_CALL_ORIGINAL_CONN_DATA_METHOD(query)(el->conn, query, query_len TSRMLS_CC)) {
					ret = FAIL;
				}
			}
		}
	}

	DBG_RETURN(ret);
}
/* }}} */
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
