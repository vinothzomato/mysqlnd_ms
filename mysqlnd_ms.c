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

/* $Id$ */

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

#include "mysqlnd_ms_enum_n_def.h"
#include "mysqlnd_ms_switch.h"

struct st_mysqlnd_conn_methods * ms_orig_mysqlnd_conn_methods;

static void mysqlnd_ms_conn_free_plugin_data(MYSQLND *conn TSRMLS_DC);

MYSQLND_STATS * mysqlnd_ms_stats = NULL;


/* {{{ mysqlnd_ms_get_scheme_from_list_data */
static int
mysqlnd_ms_get_scheme_from_list_data(MYSQLND_MS_LIST_DATA * el, char ** scheme, zend_bool persistent TSRMLS_DC)
{
	char * tmp = NULL;
	int scheme_len;
	*scheme = NULL;
#ifndef PHP_WIN32
	if (el->host && !strcasecmp("localhost", el->host)) {
		scheme_len = mnd_sprintf(&tmp, 0, "unix://%s", el->socket? el->socket : "/tmp/mysql.sock");
#else
	if (el->host && !strcmp(".", host) {
		scheme_len = mnd_sprintf(&tmp, 0, "pipe://%s", el->socket? el->socket : "\\\\.\\pipe\\MySQL");
#endif
	} else {
		if (!el->port) {
			el->port = 3306;
		}
		scheme_len = mnd_sprintf(&tmp, 0, "tcp://%s:%u", el->host? el->host:"localhost", el->port);
	}
	if (tmp) {
		*scheme = mnd_pestrndup(tmp, scheme_len, persistent);
		efree(tmp); /* allocated by spprintf */
	}
	return scheme_len;
}
/* }}} */


/* {{{ mysqlnd_ms_conn_list_dtor */
void
mysqlnd_ms_conn_list_dtor(void * pDest)
{
	MYSQLND_MS_LIST_DATA * element = (MYSQLND_MS_LIST_DATA *) pDest;
	TSRMLS_FETCH();
	DBG_ENTER("mysqlnd_ms_conn_list_dtor");
	DBG_INF_FMT("conn=%p", element->conn);
	if (element->conn) {
		mysqlnd_close(element->conn, TRUE);
		element->conn = NULL;
	}
	if (element->host) {
		mnd_pefree(element->host, element->persistent);
		element->host = NULL;
	}
	if (element->user) {
		mnd_pefree(element->user, element->persistent);
		element->user = NULL;
	}
	if (element->passwd) {
		mnd_pefree(element->passwd, element->persistent);
		element->passwd = NULL;
	}
	if (element->db) {
		mnd_pefree(element->db, element->persistent);
		element->db = NULL;
	}
	if (element->socket) {
		mnd_pefree(element->socket, element->persistent);
		element->socket = NULL;
	}
	if (element->emulated_scheme) {
		mnd_pefree(element->emulated_scheme, element->persistent);
	}
	element->persistent = FALSE;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_config_json_string_is_bool_true */
static zend_bool
mysqlnd_ms_config_json_string_is_bool_false(const char * value)
{
	if (!value) {
		return TRUE;
	}
	if (!strncmp("0", value, sizeof("0") - 1)) {
		return TRUE;
	}
	if (!strncasecmp("false", value, sizeof("false") - 1)) {
		return TRUE;
	}
	if (!strncasecmp("off", value, sizeof("off") - 1)) {
		return TRUE;
	}
	if (!strncasecmp("aus", value, sizeof("aus") - 1)) {
		return TRUE;
	}
	return FALSE;
}
/* }}} */

/* {{{ mysqlnd_ms_filter_ht_dtor */
static void
mysqlnd_ms_filter_ht_dtor(void * data)
{
	HashTable * entry = * (HashTable **) data;
	TSRMLS_FETCH();
	if (entry) {
		zend_hash_destroy(entry);
		mnd_free(entry);
	}
}
/* }}} */


/* {{{ mysqlnd_ms_filter_dtor */
static void
mysqlnd_ms_filter_dtor(void * data)
{
	struct st_mysqlnd_ms_table_filter * entry = * (struct st_mysqlnd_ms_table_filter **) data;
	TSRMLS_FETCH();
	if (entry) {
		if (entry->wild) {
			mnd_free(entry->wild);
			entry->wild = NULL;
		}
		if (entry->host_id) {
			mnd_free(entry->host_id);
			entry->host_id = NULL;
		}
		if (entry->next) {
			mysqlnd_ms_filter_dtor(&entry->next);
		}
		mnd_free(entry);
		* (struct st_mysqlnd_ms_table_filter **) data = NULL;
	}
}
/* }}} */

/* {{{ mysqlnd_ms_filter_compare (based on array_data_compare) */
static int
mysqlnd_ms_filter_comparator(const struct st_mysqlnd_ms_table_filter * a, const struct st_mysqlnd_ms_table_filter * b)
{
	if (a && b) {
		return a->priority > b->priority? -1:(a->priority == b->priority? 0: 1);
	}
	return 0;
}


/* {{{ mysqlnd_ms_filter_compare (based on array_data_compare) */
static int
mysqlnd_ms_filter_compare(const void * a, const void * b TSRMLS_DC)
{
	Bucket * f = *((Bucket **) a);
	Bucket * s = *((Bucket **) b);
	struct st_mysqlnd_ms_table_filter * first = *((struct st_mysqlnd_ms_table_filter **) f->pData);
	struct st_mysqlnd_ms_table_filter * second = *((struct st_mysqlnd_ms_table_filter **) s->pData);

	return mysqlnd_ms_filter_comparator(first, second);
}
/* }}} */


/* {{{ mysqlnd_ms_load_table_filters */
static enum_func_status
mysqlnd_ms_load_table_filters(HashTable * filters_ht, struct st_mysqlnd_ms_config_json_entry * section,
							  const char * const section_name, size_t section_name_len TSRMLS_DC)
{
	enum_func_status ret = PASS;
	unsigned int filter_count = 0;
	DBG_ENTER("mysqlnd_ms_load_table_filters");

	if (section && filters_ht) {
		zend_bool section_exists;
		struct st_mysqlnd_ms_config_json_entry * filters_section =
				mysqlnd_ms_config_json_sub_section(section, TABLE_FILTERS, sizeof(TABLE_FILTERS) - 1, &section_exists TSRMLS_CC);
		zend_bool subsection_is_list = mysqlnd_ms_config_json_section_is_list(filters_section TSRMLS_CC);
		zend_bool subsection_is_obj_list =
				subsection_is_list && mysqlnd_ms_config_json_section_is_object_list(filters_section TSRMLS_CC);

		if (section_exists && filters_section && subsection_is_obj_list) {
			zend_bool value_exists = FALSE, is_list_value = FALSE;
			do {
				struct st_mysqlnd_ms_table_filter * new_filter_entry = NULL;
				char * filter_name = NULL;
				size_t filter_name_len = 0;
				char * str_value;
				struct st_mysqlnd_ms_config_json_entry * current_filter =
						mysqlnd_ms_config_json_next_sub_section(filters_section, &filter_name, &filter_name_len, NULL TSRMLS_CC);

				if (!current_filter || !filter_name || !filter_name_len) {
					DBG_INF("no next sub-section");
					break;
				}

				new_filter_entry = mnd_calloc(1, sizeof(struct st_mysqlnd_ms_table_filter));
				if (!new_filter_entry) {
					ret = FAIL;
					break;
				}

				if ((new_filter_entry->host_id_len = section_name_len) && new_filter_entry->host_id) {
					new_filter_entry->host_id = mnd_pestrndup(new_filter_entry->host_id, section_name_len, 1);
				}

				str_value = mysqlnd_ms_config_json_string_from_section(current_filter, SECT_FILTER_PRIORITY_NAME,
																	   sizeof(SECT_FILTER_PRIORITY_NAME) - 1,
																	   &value_exists, &is_list_value TSRMLS_CC);
				if (value_exists) {
					new_filter_entry->priority = (unsigned int) atoi(str_value);
				}
				if (str_value) {
					mnd_efree(str_value);
					str_value = NULL;
				}
				/* now add */
				{
					HashTable ** existing_filter;
					if (SUCCESS == zend_hash_find(filters_ht, filter_name, filter_name_len + 1, (void **) &existing_filter)) {
						DBG_INF("Filter HT already exists");
						if (!existing_filter ||
							SUCCESS != zend_hash_next_index_insert(*existing_filter, &new_filter_entry,
						  				 			 			   sizeof(struct st_mysqlnd_ms_table_filter *), NULL))
						{
							DBG_ERR_FMT("Couldn't add new filter and couldn't find the original %*s", filter_name_len, filter_name);
							php_error_docref(NULL TSRMLS_CC, E_WARNING,
									MYSQLND_MS_ERROR_PREFIX "Couldn't add new filter and couldn't find the original %*s",
									(int) filter_name_len, filter_name);
							mysqlnd_ms_filter_dtor(&new_filter_entry);
						} else {
							DBG_INF("Added to the existing HT");
							filter_count++;
							DBG_INF("re-sorting");
							/* Sort specified array. */
							zend_hash_sort(*existing_filter, zend_qsort, mysqlnd_ms_filter_compare, 0 /* renumber */ TSRMLS_CC);
						}
					} else {
						HashTable * ht_for_new_filter = mnd_malloc(sizeof(HashTable));
						DBG_INF("Filter HT doesn't exist, need to create it");
						if (ht_for_new_filter) {
							if (SUCCESS == zend_hash_init(ht_for_new_filter, 2, NULL, mysqlnd_ms_filter_dtor, 1/*pers*/)) {
								if (SUCCESS != zend_hash_add(filters_ht, filter_name, filter_name_len + 1, &ht_for_new_filter,
						  				 			 		 sizeof(HashTable *), NULL))
								{
									DBG_ERR_FMT("The hashtable %*s did not exist in the filters_ht but couldn't add",
												filter_name_len, filter_name);
									php_error_docref(NULL TSRMLS_CC, E_WARNING,
											MYSQLND_MS_ERROR_PREFIX "The hashtable %*s did not exist in the filters_ht "
											"but couldn't add", (int) filter_name_len, filter_name);

									zend_hash_destroy(ht_for_new_filter);
									mnd_free(ht_for_new_filter);
									ht_for_new_filter = NULL;
									mysqlnd_ms_filter_dtor(&new_filter_entry);
									new_filter_entry = NULL;
								} else if (SUCCESS != zend_hash_next_index_insert(ht_for_new_filter, &new_filter_entry,
						  				 			 			   				  sizeof(struct st_mysqlnd_ms_table_filter *), NULL))
								{
									DBG_ERR_FMT("Couldn't add new filter and couldn't find the original %*s", filter_name_len, filter_name);
									php_error_docref(NULL TSRMLS_CC, E_WARNING,
											MYSQLND_MS_ERROR_PREFIX "Couldn't add new filter and couldn't find the original %*s",
											(int) filter_name_len, filter_name);
										mysqlnd_ms_filter_dtor(&new_filter_entry);
								} else {
									DBG_INF("Created, added to global HT and filter added to local HT");
									filter_count++;
								}
							}
						}
					}
				}
			} while (1);
		}
	}
	DBG_INF_FMT("filter_count=%u", filter_count);

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_connect_to_host_aux */
static enum_func_status
mysqlnd_ms_connect_to_host_aux(MYSQLND * conn, char * host, zend_llist * conn_list,
							   struct st_mysqlnd_ms_conn_credentials * cred,
							   zend_bool lazy_connections, zend_bool persistent TSRMLS_DC)
{
	enum_func_status ret;

	DBG_ENTER("mysqlnd_ms_connect_to_host_aux");
	DBG_INF_FMT("conn:%p host:%s port:%d socket:%s", conn, host, cred->port, cred->socket);
	if (lazy_connections) {
		DBG_INF("Lazy connection");
		ret = PASS;
	} else {
		ret = ms_orig_mysqlnd_conn_methods->connect(conn, host, cred->user, cred->passwd, cred->passwd_len, cred->db, cred->db_len,
													cred->port, cred->socket, cred->mysql_flags TSRMLS_CC);

		DBG_INF_FMT("Connection "MYSQLND_LLU_SPEC" established", conn->thread_id);
	}

	if (ret == PASS) {
		MYSQLND_MS_LIST_DATA new_element = {0};
		new_element.conn = conn;
		new_element.host = host? mnd_pestrdup(host, persistent) : NULL;
		new_element.persistent = persistent;
		new_element.port = cred->port;

		new_element.user = cred->user? mnd_pestrdup(cred->user, conn->persistent) : NULL;

		new_element.passwd_len = cred->passwd_len;
		new_element.passwd = cred->db? mnd_pestrndup(cred->passwd, cred->passwd_len, conn->persistent) : NULL;

		new_element.db_len = cred->db_len;
		new_element.db = cred->db? mnd_pestrndup(cred->db, cred->db_len, conn->persistent) : NULL;

		new_element.connect_flags = cred->mysql_flags;

		new_element.socket = cred->socket? mnd_pestrdup(cred->socket, conn->persistent) : NULL;
		new_element.emulated_scheme_len = mysqlnd_ms_get_scheme_from_list_data(&new_element, &new_element.emulated_scheme,
																			   persistent TSRMLS_CC);
		zend_llist_add_element(conn_list, &new_element);
	}
	DBG_INF_FMT("ret=%s", ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_connect_to_host */
static enum_func_status
mysqlnd_ms_connect_to_host(MYSQLND * conn, zend_llist * conn_list,
						   struct st_mysqlnd_ms_conn_credentials * master_credentials,
						   struct st_mysqlnd_ms_config_json_entry * main_section,
						   const char * const subsection_name, size_t subsection_name_len,
						   HashTable * table_filters,
						   zend_bool lazy_connections, zend_bool persistent,
						   zend_bool process_all_list_values,
						   unsigned int success_stat, unsigned int fail_stat TSRMLS_DC)
{
	struct st_mysqlnd_ms_conn_credentials cred = *master_credentials;
	zend_bool value_exists = FALSE, is_list_value = FALSE;
	struct st_mysqlnd_ms_config_json_entry * subsection = NULL, * parent_subsection = NULL;
	zend_bool recursive = FALSE;
	unsigned int i = 0;
	unsigned int failures = 0;
	DBG_ENTER("mysqlnd_ms_connect_to_host");
	DBG_INF_FMT("conn:%p", conn);

	if (TRUE == mysqlnd_ms_config_json_sub_section_exists(main_section, subsection_name, subsection_name_len TSRMLS_CC)) {
		subsection =
			parent_subsection =
				mysqlnd_ms_config_json_sub_section(main_section, subsection_name, subsection_name_len, &value_exists TSRMLS_CC);

		recursive =	(TRUE == mysqlnd_ms_config_json_section_is_list(subsection TSRMLS_CC)
					&&
					TRUE == mysqlnd_ms_config_json_section_is_object_list(subsection TSRMLS_CC));
	} else {
		char error_buf[128];
		failures++;
		DBG_ERR_FMT("Cannot find %s section", subsection_name);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Cannot find %s section in config", subsection_name);
		if (conn) {
			snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Cannot find %s section in config", subsection_name);
			error_buf[sizeof(error_buf) - 1] = '\0';
			SET_CLIENT_ERROR(conn->error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
		}
	}
	do {
		char * socket_to_use = NULL;
		char * user_to_use = NULL;
		char * pass_to_use = NULL;
		char * db_to_use = NULL;
		char * port_str = NULL;
		char * flags_str = NULL;
		char * host = NULL;

		char * current_subsection_name = NULL;
		size_t current_subsection_name_len = 0;

		if (recursive) {
			subsection = mysqlnd_ms_config_json_next_sub_section(parent_subsection, &current_subsection_name,
																 &current_subsection_name_len, NULL TSRMLS_CC);
		}
		if (!subsection) {
			break;
		}

		port_str = mysqlnd_ms_config_json_string_from_section(subsection, SECT_PORT_NAME, sizeof(SECT_PORT_NAME) - 1,
															  &value_exists, &is_list_value TSRMLS_CC);
		if (value_exists && port_str) {
			cred.port = (unsigned int) atoi(port_str);
		}

		flags_str = mysqlnd_ms_config_json_string_from_section(subsection, SECT_CONNECT_FLAGS_NAME, sizeof(SECT_CONNECT_FLAGS_NAME)-1,
															   &value_exists, &is_list_value TSRMLS_CC);
		if (value_exists) {
			cred.mysql_flags = (unsigned int) atoi(flags_str);
		}

		socket_to_use = mysqlnd_ms_config_json_string_from_section(subsection, SECT_SOCKET_NAME, sizeof(SECT_SOCKET_NAME) - 1,
															   &value_exists, &is_list_value TSRMLS_CC);
		if (value_exists) {
			cred.socket = socket_to_use;
		}
		user_to_use = mysqlnd_ms_config_json_string_from_section(subsection, SECT_USER_NAME, sizeof(SECT_USER_NAME) - 1,
																 &value_exists, &is_list_value TSRMLS_CC);
		if (value_exists) {
			cred.user = user_to_use;
		}
		pass_to_use = mysqlnd_ms_config_json_string_from_section(subsection, SECT_PASS_NAME, sizeof(SECT_PASS_NAME) - 1,
																 &value_exists, &is_list_value TSRMLS_CC);
		if (value_exists) {
			cred.passwd = pass_to_use;
			cred.passwd_len = strlen(cred.passwd);
		}

		db_to_use = mysqlnd_ms_config_json_string_from_section(subsection, SECT_DB_NAME, sizeof(SECT_DB_NAME) - 1,
															   &value_exists, &is_list_value TSRMLS_CC);
		if (value_exists) {
			cred.db = db_to_use;
			cred.db_len = strlen(cred.db);
		}

		host = mysqlnd_ms_config_json_string_from_section(subsection, SECT_HOST_NAME, sizeof(SECT_HOST_NAME) - 1,
														  &value_exists, &is_list_value TSRMLS_CC);
		if (FALSE == value_exists) {
			DBG_ERR_FMT("Cannot find ["SECT_HOST_NAME"] in [%s] section in config", subsection_name);
			php_error_docref(NULL TSRMLS_CC, E_WARNING,
							 MYSQLND_MS_ERROR_PREFIX " Cannot find ["SECT_HOST_NAME"] in [%s] section in config", subsection_name);
			SET_CLIENT_ERROR(conn->error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE,
							 MYSQLND_MS_ERROR_PREFIX " Cannot find ["SECT_HOST_NAME"] in section in config");
			failures++;
		} else {
			MYSQLND * tmp_conn = (conn && i==0)? conn : mysqlnd_init(persistent);
			if (tmp_conn) {
				enum_func_status status =
					mysqlnd_ms_connect_to_host_aux(tmp_conn, host, conn_list, &cred, lazy_connections, persistent TSRMLS_CC);
				if (status != PASS) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Cannot connect to %s", host);
					failures++;
					if (!conn) {
						tmp_conn->m->dtor(tmp_conn TSRMLS_CC);
					}
					MYSQLND_MS_INC_STATISTIC(fail_stat);
				} else {
					if (FAIL == mysqlnd_ms_load_table_filters(table_filters, subsection, current_subsection_name,
															  current_subsection_name_len TSRMLS_CC))
					{
						failures++;
					}
					if (!lazy_connections) {
						MYSQLND_MS_INC_STATISTIC(success_stat);
					}
				}
			} else {
				failures++;
				/* Handle OOM!! */
				MYSQLND_MS_INC_STATISTIC(fail_stat);
			}
		}
		i++; /* to pass only the first conn handle */

		if (port_str) {
			mnd_efree(port_str);
		}
		if (flags_str) {
			mnd_efree(flags_str);
		}
		if (socket_to_use) {
			mnd_efree(socket_to_use);
		}
		if (user_to_use) {
			mnd_efree(user_to_use);
		}
		if (pass_to_use) {
			mnd_efree(pass_to_use);
		}
		if (db_to_use) {
			mnd_efree(db_to_use);
		}
		if (host) {
			mnd_efree(host);
			host = NULL;
		}
	} while (TRUE == process_all_list_values && TRUE == recursive && failures == 0);

	DBG_RETURN(failures==0 ? PASS:FAIL);
}
/* }}} */


/* {{{ mysqlnd_ms_lb_strategy_setup */
static void
mysqlnd_ms_lb_strategy_setup(struct mysqlnd_ms_lb_strategies * strategies,
							 struct st_mysqlnd_ms_config_json_entry * the_section TSRMLS_DC)
{
	zend_bool value_exists = FALSE, is_list_value = FALSE;

	DBG_ENTER("mysqlnd_ms_lb_strategy_setup");
	{
		char * pick_strategy =
			mysqlnd_ms_config_json_string_from_section(the_section, PICK_NAME, sizeof(PICK_NAME) - 1,
													   &value_exists, &is_list_value TSRMLS_CC);

		strategies->pick_strategy = strategies->fallback_pick_strategy = DEFAULT_PICK_STRATEGY;

		if (value_exists && pick_strategy) {
			/* random is a substing of random_once thus we check first for random_once */
			if (!strncasecmp(PICK_RROBIN, pick_strategy, sizeof(PICK_RROBIN) - 1)) {
				strategies->pick_strategy = strategies->fallback_pick_strategy = SERVER_PICK_RROBIN;
			} else if (!strncasecmp(PICK_RANDOM_ONCE, pick_strategy, sizeof(PICK_RANDOM_ONCE) - 1)) {
				strategies->pick_strategy = strategies->fallback_pick_strategy = SERVER_PICK_RANDOM_ONCE;
			} else if (!strncasecmp(PICK_RANDOM, pick_strategy, sizeof(PICK_RANDOM) - 1)) {
				strategies->pick_strategy = strategies->fallback_pick_strategy = SERVER_PICK_RANDOM;
			} else if (!strncasecmp(PICK_USER, pick_strategy, sizeof(PICK_USER) - 1)) {
				strategies->pick_strategy = SERVER_PICK_USER;
				if (is_list_value) {
					mnd_efree(pick_strategy);
					pick_strategy =
						mysqlnd_ms_config_json_string_from_section(the_section, PICK_NAME, sizeof(PICK_NAME) - 1,
																   &value_exists, &is_list_value TSRMLS_CC);
					if (pick_strategy) {
						if (!strncasecmp(PICK_RANDOM_ONCE, pick_strategy, sizeof(PICK_RANDOM_ONCE) - 1)) {
							strategies->fallback_pick_strategy = SERVER_PICK_RANDOM_ONCE;
						} else if (!strncasecmp(PICK_RANDOM, pick_strategy, sizeof(PICK_RANDOM) - 1)) {
							strategies->fallback_pick_strategy = SERVER_PICK_RANDOM;
						} else if (!strncasecmp(PICK_RROBIN, pick_strategy, sizeof(PICK_RROBIN) - 1)) {
							strategies->fallback_pick_strategy = SERVER_PICK_RROBIN;
						}
					}
				}
			}
		}
		if (pick_strategy) {
			mnd_efree(pick_strategy);
		}
	}

	{
		char * failover_strategy =
			mysqlnd_ms_config_json_string_from_section(the_section, FAILOVER_NAME, sizeof(FAILOVER_NAME) - 1,
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
			mysqlnd_ms_config_json_string_from_section(the_section, MASTER_ON_WRITE_NAME, sizeof(MASTER_ON_WRITE_NAME) - 1,
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
			mysqlnd_ms_config_json_string_from_section(the_section, TRX_STICKINESS_NAME, sizeof(TRX_STICKINESS_NAME) - 1,
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
			php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " trx_stickiness_strategy is not supported before PHP 5.3.99");
#endif
			mnd_efree(trx_strategy);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms::connect */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, connect)(MYSQLND * conn,
									const char * host,
									const char * user,
									const char * passwd,
									unsigned int passwd_len,
									const char * db,
									unsigned int db_len,
									unsigned int port,
									const char * socket,
									unsigned int mysql_flags TSRMLS_DC)
{
	enum_func_status ret = FAIL;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp;
	size_t host_len = host? strlen(host) : 0;
	zend_bool section_found;
	zend_bool hotloading = MYSLQND_MS_HOTLOADING;

	DBG_ENTER("mysqlnd_ms::connect");
	if (hotloading) {
		MYSQLND_MS_CONFIG_JSON_LOCK(mysqlnd_ms_json_config);
	}
	section_found = mysqlnd_ms_config_json_section_exists(mysqlnd_ms_json_config, host, host_len, hotloading? FALSE:TRUE TSRMLS_CC);
	if (MYSQLND_MS_G(force_config_usage) && FALSE == section_found) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Exclusive usage of configuration enforced but did not find the correct INI file section (%s)", host);
		if (hotloading) {
			MYSQLND_MS_CONFIG_JSON_UNLOCK(mysqlnd_ms_json_config);
		}
		SET_CLIENT_ERROR(conn->error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, MYSQLND_MS_ERROR_PREFIX " Exclusive usage of configuration enforced but did not find the correct INI file section");
		DBG_RETURN(FAIL);
	}
	mysqlnd_ms_conn_free_plugin_data(conn TSRMLS_CC);

	conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	*conn_data_pp = mnd_pecalloc(1, sizeof(MYSQLND_MS_CONNECTION_DATA), conn->persistent);
	zend_llist_init(&(*conn_data_pp)->master_connections, sizeof(MYSQLND_MS_LIST_DATA), (llist_dtor_func_t) mysqlnd_ms_conn_list_dtor, conn->persistent);
	zend_llist_init(&(*conn_data_pp)->slave_connections, sizeof(MYSQLND_MS_LIST_DATA), (llist_dtor_func_t) mysqlnd_ms_conn_list_dtor, conn->persistent);
	(*conn_data_pp)->cred.user = user? mnd_pestrdup(user, conn->persistent) : NULL;
	(*conn_data_pp)->cred.passwd_len = passwd_len;
	(*conn_data_pp)->cred.passwd = passwd? mnd_pestrndup(passwd, passwd_len, conn->persistent) : NULL;
	(*conn_data_pp)->cred.db_len = db_len;
	(*conn_data_pp)->cred.db = db? mnd_pestrndup(db, db_len, conn->persistent) : NULL;
	(*conn_data_pp)->cred.port = port;
	(*conn_data_pp)->cred.socket = socket? mnd_pestrdup(socket, conn->persistent) : NULL;
	(*conn_data_pp)->cred.mysql_flags = mysql_flags;

	if (FALSE == section_found) {
		DBG_INF("section not found");
		ret = mysqlnd_ms_connect_to_host_aux(conn, /*safe*/(char *)host, &(*conn_data_pp)->master_connections, &(*conn_data_pp)->cred, FALSE /*lazy*/,
										 conn->persistent TSRMLS_CC);
	} else {
		struct st_mysqlnd_ms_config_json_entry * the_section;

		if (!hotloading) {
			MYSQLND_MS_CONFIG_JSON_LOCK(mysqlnd_ms_json_config);
		}

		do {
			zend_bool value_exists = FALSE, use_lazy_connections = TRUE;
			/* create master connection */
			zend_hash_init(&(*conn_data_pp)->stgy.table_filters, 4, NULL/*hash*/, mysqlnd_ms_filter_ht_dtor/*dtor*/, 1/*pers*/);

			the_section = mysqlnd_ms_config_json_section(mysqlnd_ms_json_config, host, host_len, &value_exists TSRMLS_CC);

			{
				char * lazy_connections = mysqlnd_ms_config_json_string_from_section(the_section, LAZY_NAME, sizeof(LAZY_NAME) - 1,
													&use_lazy_connections, NULL TSRMLS_CC);
				/* ignore if lazy_connections ini entry exists or not */
				use_lazy_connections = TRUE;
				if (lazy_connections) {
					/* lazy_connections ini entry exists, disabled? */
					use_lazy_connections = !mysqlnd_ms_config_json_string_is_bool_false(lazy_connections);
					mnd_efree(lazy_connections);
					lazy_connections = NULL;
				}
			}
			DBG_INF("-------------------- MASTER CONNECTIONS ------------------");
			ret = mysqlnd_ms_connect_to_host(conn, &(*conn_data_pp)->master_connections, &(*conn_data_pp)->cred, the_section,
											 MASTER_NAME, sizeof(MASTER_NAME) - 1,
											 &(*conn_data_pp)->stgy.table_filters,
											 use_lazy_connections,
											 conn->persistent, FALSE /* multimaster*/,
											 MS_STAT_NON_LAZY_CONN_MASTER_SUCCESS,
											 MS_STAT_NON_LAZY_CONN_MASTER_FAILURE TSRMLS_CC);
			if (FAIL == ret) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Error while connecting to the master(s)");
				if (!conn->error_info.error_no) {
					SET_CLIENT_ERROR(conn->error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE,
						MYSQLND_MS_ERROR_PREFIX " Error while connecting to the master(s)");
				}
				break;
			}

			DBG_INF("-------------------- SLAVE CONNECTIONS ------------------");
			ret = mysqlnd_ms_connect_to_host(NULL, &(*conn_data_pp)->slave_connections, &(*conn_data_pp)->cred, the_section,
											 SLAVE_NAME, sizeof(SLAVE_NAME) - 1,
											 &(*conn_data_pp)->stgy.table_filters,
											 use_lazy_connections,
											 conn->persistent, TRUE /* multi*/,
											 MS_STAT_NON_LAZY_CONN_SLAVE_SUCCESS,
											 MS_STAT_NON_LAZY_CONN_SLAVE_FAILURE TSRMLS_CC);
			if (FAIL == ret) {
				if (!conn->error_info.error_no) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Error while connecting to the slaves");
					SET_CLIENT_ERROR(conn->error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE,
										MYSQLND_MS_ERROR_PREFIX " Error while connecting to the slaves");
				}
				break;
			}

			mysqlnd_ms_lb_strategy_setup(&(*conn_data_pp)->stgy, the_section TSRMLS_CC);
		} while (0);
		mysqlnd_ms_config_json_reset_section(the_section, TRUE TSRMLS_CC);

		if (!hotloading) {
			MYSQLND_MS_CONFIG_JSON_UNLOCK(mysqlnd_ms_json_config);
		}
	}

	if (hotloading) {
		MYSQLND_MS_CONFIG_JSON_UNLOCK(mysqlnd_ms_json_config);
	}
	if (ret == PASS) {
		(*conn_data_pp)->connect_host = host? mnd_pestrdup(host, conn->persistent) : NULL;
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ MYSQLND_METHOD(mysqlnd_ms, query) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, query)(MYSQLND * conn, const char * query, unsigned int query_len TSRMLS_DC)
{
	MYSQLND * connection;
	enum_func_status ret = FAIL;
	zend_bool use_all = 0;
	DBG_ENTER("mysqlnd_ms::query");

	connection = mysqlnd_ms_pick_server(conn, query, query_len, &use_all TSRMLS_CC);
	DBG_INF_FMT("Connection %p", connection);
	/*
	  Beware : error_no is set to 0 in original->query. This, this might be a problem,
	  as we dump a connection from usage till the end of the script
	*/
	if (connection->error_info.error_no) {
		DBG_RETURN(ret);
	}


#ifdef ALL_SERVER_DISPATCH
	if (use_all) {
		MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
		zend_llist * master_connections = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->master_connections : NULL;
		zend_llist * slave_connections = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->slave_connections : NULL;

		mysqlnd_ms_query_all(conn, query, query_len, master_connections, slave_connections TSRMLS_CC);
	}
#endif

	ret = ms_orig_mysqlnd_conn_methods->query(connection, query, query_len TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::use_result */
static MYSQLND_RES *
MYSQLND_METHOD(mysqlnd_ms, use_result)(MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_RES * result;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	MYSQLND * conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	DBG_ENTER("mysqlnd_ms::use_result");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, (conn)->m->get_thread_id(conn TSRMLS_CC));
	result = ms_orig_mysqlnd_conn_methods->use_result(conn TSRMLS_CC);
	DBG_RETURN(result);
}
/* }}} */


/* {{{ mysqlnd_ms::store_result */
static MYSQLND_RES *
MYSQLND_METHOD(mysqlnd_ms, store_result)(MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_RES * result;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	MYSQLND * conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	DBG_ENTER("mysqlnd_ms::store_result");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, (conn)->m->get_thread_id(conn TSRMLS_CC));
	result = ms_orig_mysqlnd_conn_methods->store_result(conn TSRMLS_CC);
	DBG_RETURN(result);
}
/* }}} */


/* {{{ mysqlnd_ms_conn_free_plugin_data */
static void
mysqlnd_ms_conn_free_plugin_data(MYSQLND *conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** data_pp;
	DBG_ENTER("mysqlnd_ms_conn_free_plugin_data");

	data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_ms_plugin_id);
	if (data_pp && *data_pp) {
		if ((*data_pp)->connect_host) {
			mnd_pefree((*data_pp)->connect_host, conn->persistent);
			(*data_pp)->connect_host = NULL;
		}

		if ((*data_pp)->cred.user) {
			mnd_pefree((*data_pp)->cred.user, conn->persistent);
			(*data_pp)->cred.user = NULL;
		}

		if ((*data_pp)->cred.passwd) {
			mnd_pefree((*data_pp)->cred.passwd, conn->persistent);
			(*data_pp)->cred.passwd = NULL;
		}
		(*data_pp)->cred.passwd_len = 0;

		if ((*data_pp)->cred.db) {
			mnd_pefree((*data_pp)->cred.db, conn->persistent);
			(*data_pp)->cred.db = NULL;
		}
		(*data_pp)->cred.db_len = 0;

		if ((*data_pp)->cred.socket) {
			mnd_pefree((*data_pp)->cred.socket, conn->persistent);
			(*data_pp)->cred.socket = NULL;
		}

		(*data_pp)->cred.port = 0;
		(*data_pp)->cred.mysql_flags = 0;

		DBG_INF_FMT("cleaning the llists");
		zend_llist_clean(&(*data_pp)->master_connections);
		zend_llist_clean(&(*data_pp)->slave_connections);

		DBG_INF_FMT("cleaning the table filters");
		zend_hash_destroy(&(*data_pp)->stgy.table_filters);

		mnd_pefree(*data_pp, conn->persistent);
		*data_pp = NULL;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms::free_contents */
static void
MYSQLND_METHOD(mysqlnd_ms, free_contents)(MYSQLND *conn TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms::free_contents");
	if (conn->refcount > 0)
		DBG_VOID_RETURN;

	mysqlnd_ms_conn_free_plugin_data(conn TSRMLS_CC);
	ms_orig_mysqlnd_conn_methods->free_contents(conn TSRMLS_CC);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms::escape_string */
static ulong
MYSQLND_METHOD(mysqlnd_ms, escape_string)(const MYSQLND * const proxy_conn, char *newstr, const char *escapestr, size_t escapestr_len TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	DBG_ENTER("mysqlnd_ms::escape_string");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, (conn)->m->get_thread_id(conn TSRMLS_CC));
	DBG_RETURN(ms_orig_mysqlnd_conn_methods->escape_string(conn, newstr, escapestr, escapestr_len TSRMLS_CC));
}
/* }}} */




/* {{{ mysqlnd_ms::change_user */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, change_user)(MYSQLND * const proxy_conn,
										  const char *user,
										  const char *passwd,
										  const char *db,
										  zend_bool silent
#if PHP_VERSION_ID >= 50399
										  ,size_t passwd_len
#endif
										  TSRMLS_DC)
{
	enum_func_status ret = PASS;
	zend_llist_position	pos;
	MYSQLND_MS_LIST_DATA * el;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	zend_llist * master_connections = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->master_connections : NULL;
	zend_llist * slave_connections = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->slave_connections : NULL;

	DBG_ENTER("mysqlnd_ms::select_db");
	if (!conn_data_pp || !*conn_data_pp) {
#if PHP_VERSION_ID >= 50399
		DBG_RETURN(ms_orig_mysqlnd_conn_methods->change_user(proxy_conn, user, passwd, db, silent, passwd_len TSRMLS_CC));
#else
		DBG_RETURN(ms_orig_mysqlnd_conn_methods->change_user(proxy_conn, user, passwd, db, silent TSRMLS_CC));
#endif
	}



	/* search the list of easy handles hanging off the multi-handle */
	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(master_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(master_connections, &pos))
	{
		if (PASS != ms_orig_mysqlnd_conn_methods->change_user(el->conn, user, passwd, db, silent
#if PHP_VERSION_ID >= 50399
															,passwd_len
#endif
															TSRMLS_CC))
		{
			ret = FAIL;
		}
	}

	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(slave_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(slave_connections, &pos))
	{
		if (PASS != ms_orig_mysqlnd_conn_methods->change_user(el->conn, user, passwd, db, silent
#if PHP_VERSION_ID >= 50399
															,passwd_len
#endif
															TSRMLS_CC))
		{
			ret = FAIL;
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::ping */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, ping)(MYSQLND * const proxy_conn TSRMLS_DC)
{
	enum_func_status ret;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	DBG_ENTER("mysqlnd_ms::ping");
	ret = ms_orig_mysqlnd_conn_methods->ping(conn TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::kill */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, kill)(MYSQLND * proxy_conn, unsigned int pid TSRMLS_DC)
{
	enum_func_status ret;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	DBG_ENTER("mysqlnd_ms::kill");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, (conn)->m->get_thread_id(conn TSRMLS_CC));
	ret = ms_orig_mysqlnd_conn_methods->kill_connection(conn, pid TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::select_db */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, select_db)(MYSQLND * const proxy_conn, const char * const db, unsigned int db_len TSRMLS_DC)
{
	enum_func_status ret = PASS;
	zend_llist_position	pos;
	MYSQLND_MS_LIST_DATA * el;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	zend_llist * master_connections = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->master_connections : NULL;
	zend_llist * slave_connections = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->slave_connections : NULL;

	DBG_ENTER("mysqlnd_ms::select_db");
	if (!conn_data_pp || !*conn_data_pp) {
		DBG_RETURN(ms_orig_mysqlnd_conn_methods->select_db(proxy_conn, db, db_len TSRMLS_CC));
	}
	/* search the list of easy handles hanging off the multi-handle */
	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(master_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(master_connections, &pos))
	{
		if (PASS != ms_orig_mysqlnd_conn_methods->select_db(el->conn, db, db_len TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(slave_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(slave_connections, &pos))
	{
		if (PASS != ms_orig_mysqlnd_conn_methods->select_db(el->conn, db, db_len TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::set_charset */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, set_charset)(MYSQLND * const proxy_conn, const char * const csname TSRMLS_DC)
{
	enum_func_status ret = PASS;
	zend_llist_position	pos;
	MYSQLND_MS_LIST_DATA * el;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	zend_llist * master_connections = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->master_connections : NULL;
	zend_llist * slave_connections = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->slave_connections : NULL;

	DBG_ENTER("mysqlnd_ms::set_charset");
	if (!conn_data_pp || !*conn_data_pp) {
		DBG_RETURN(ms_orig_mysqlnd_conn_methods->set_charset(proxy_conn, csname TSRMLS_CC));
	}
	/* search the list of easy handles hanging off the multi-handle */
	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(master_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(master_connections, &pos))
	{
		if (PASS != ms_orig_mysqlnd_conn_methods->set_charset(el->conn, csname TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(slave_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(slave_connections, &pos))
	{
		if (PASS != ms_orig_mysqlnd_conn_methods->set_charset(el->conn, csname TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::set_server_option */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, set_server_option)(MYSQLND * const proxy_conn, enum_mysqlnd_server_option option TSRMLS_DC)
{
	enum_func_status ret = PASS;
	zend_llist_position	pos;
	MYSQLND_MS_LIST_DATA * el;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	zend_llist * master_connections = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->master_connections : NULL;
	zend_llist * slave_connections = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->slave_connections : NULL;

	DBG_ENTER("mysqlnd_ms::set_server_option");
	if (!conn_data_pp || !*conn_data_pp) {
		DBG_RETURN(ms_orig_mysqlnd_conn_methods->set_server_option(proxy_conn, option TSRMLS_CC));
	}
	/* search the list of easy handles hanging off the multi-handle */
	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(master_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(master_connections, &pos))
	{
		if (PASS != ms_orig_mysqlnd_conn_methods->set_server_option(el->conn, option TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(slave_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(slave_connections, &pos))
	{
		if (PASS != ms_orig_mysqlnd_conn_methods->set_server_option(el->conn, option TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::set_client_option */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, set_client_option)(MYSQLND * const proxy_conn, enum_mysqlnd_option option, const char * const value TSRMLS_DC)
{
	enum_func_status ret = PASS;
	zend_llist_position	pos;
	MYSQLND_MS_LIST_DATA * el;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	zend_llist * master_connections = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->master_connections : NULL;
	zend_llist * slave_connections = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->slave_connections : NULL;

	DBG_ENTER("mysqlnd_ms::set_server_option");
	if (!conn_data_pp || !*conn_data_pp) {
		DBG_RETURN(ms_orig_mysqlnd_conn_methods->set_client_option(proxy_conn, option, value TSRMLS_CC));
	}
	/* search the list of easy handles hanging off the multi-handle */
	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(master_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(master_connections, &pos))
	{
		if (PASS != ms_orig_mysqlnd_conn_methods->set_client_option(el->conn, option, value TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(slave_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(slave_connections, &pos))
	{
		if (PASS != ms_orig_mysqlnd_conn_methods->set_client_option(el->conn, option, value TSRMLS_CC)) {
			ret = FAIL;
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::next_result */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, next_result)(MYSQLND * const proxy_conn TSRMLS_DC)
{
	enum_func_status ret;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	DBG_ENTER("mysqlnd_ms::next_result");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, (conn)->m->get_thread_id(conn TSRMLS_CC));
	ret = ms_orig_mysqlnd_conn_methods->next_result(conn TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::more_results */
static zend_bool
MYSQLND_METHOD(mysqlnd_ms, more_results)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	zend_bool ret;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	DBG_ENTER("mysqlnd_ms::more_results");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, (conn)->m->get_thread_id(conn TSRMLS_CC));
	ret = ms_orig_mysqlnd_conn_methods->more_results(conn TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::errno */
static unsigned int
MYSQLND_METHOD(mysqlnd_ms, errno)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->error_info.error_no;
}
/* }}} */


/* {{{ mysqlnd_ms::error */
static const char *
MYSQLND_METHOD(mysqlnd_ms, error)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->error_info.error;
}
/* }}} */


/* {{{ mysqlnd_conn::sqlstate */
static const char *
MYSQLND_METHOD(mysqlnd_ms, sqlstate)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->error_info.sqlstate[0] ? conn->error_info.sqlstate:MYSQLND_SQLSTATE_NULL;
}
/* }}} */


/* {{{ mysqlnd_ms::field_count */
static unsigned int
MYSQLND_METHOD(mysqlnd_ms, field_count)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->field_count;
}
/* }}} */


/* {{{ mysqlnd_conn::thread_id */
static uint64_t
MYSQLND_METHOD(mysqlnd_ms, thread_id)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->thread_id;
}
/* }}} */


/* {{{ mysqlnd_ms::insert_id */
static uint64_t
MYSQLND_METHOD(mysqlnd_ms, insert_id)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->upsert_status.last_insert_id;
}
/* }}} */


/* {{{ mysqlnd_ms::affected_rows */
static uint64_t
MYSQLND_METHOD(mysqlnd_ms, affected_rows)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->upsert_status.affected_rows;
}
/* }}} */


/* {{{ mysqlnd_ms::warning_count */
static unsigned int
MYSQLND_METHOD(mysqlnd_ms, warning_count)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->upsert_status.warning_count;
}
/* }}} */


/* {{{ mysqlnd_ms::info */
static const char *
MYSQLND_METHOD(mysqlnd_ms, info)(const MYSQLND * const proxy_conn TSRMLS_DC)
{
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	const MYSQLND * const conn = ((*conn_data_pp) && (*conn_data_pp)->last_used_connection)? (*conn_data_pp)->last_used_connection:proxy_conn;
	return conn->last_message;
}
/* }}} */


#if MYSQLND_VERSION_ID >= 50009
/* {{{ MYSQLND_METHOD(mysqlnd_ms, set_autocommit) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, set_autocommit)(MYSQLND * proxy_conn, unsigned int mode TSRMLS_DC)
{
	enum_func_status ret = PASS;
	zend_llist_position	pos;
	MYSQLND_MS_LIST_DATA * el;
	MYSQLND_MS_CONNECTION_DATA ** conn_data_pp = (MYSQLND_MS_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(proxy_conn, mysqlnd_ms_plugin_id);
	zend_llist * master_connections = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->master_connections : NULL;
	zend_llist * slave_connections = (conn_data_pp && *conn_data_pp)? &(*conn_data_pp)->slave_connections : NULL;

	DBG_ENTER("mysqlnd_ms::set_autocommit");
	if (!conn_data_pp || !*conn_data_pp) {
		DBG_RETURN(ms_orig_mysqlnd_conn_methods->set_autocommit(proxy_conn, mode TSRMLS_CC));
	}

	/* search the list of easy handles hanging off the multi-handle */
	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(master_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(master_connections, &pos))
	{
		/* lazy connection ? */
		if ((CONN_GET_STATE(el->conn) != CONN_ALLOCED) &&
			(PASS != ms_orig_mysqlnd_conn_methods->set_autocommit(el->conn, mode TSRMLS_CC))) {
			ret = FAIL;
		}

	}

	for (el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_first_ex(slave_connections, &pos); el && el->conn;
			el = (MYSQLND_MS_LIST_DATA *) zend_llist_get_next_ex(slave_connections, &pos))
	{
		/* lazy connection ? */
		if ((CONN_GET_STATE(el->conn) != CONN_ALLOCED) &&
			(PASS != ms_orig_mysqlnd_conn_methods->set_autocommit(el->conn, mode TSRMLS_CC))) {
			ret = FAIL;
		}
	}

	if (mode) {
		(*conn_data_pp)->stgy.in_transaction = FALSE;
		MYSQLND_MS_INC_STATISTIC(MS_STAT_TRX_AUTOCOMMIT_ON);
	} else {
		(*conn_data_pp)->stgy.in_transaction = TRUE;
		MYSQLND_MS_INC_STATISTIC(MS_STAT_TRX_AUTOCOMMIT_OFF);
	}
	DBG_INF_FMT("in_transaction = %d", (*conn_data_pp)->stgy.in_transaction);

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ MYSQLND_METHOD(mysqlnd_ms, tx_commit) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, tx_commit)(MYSQLND * conn TSRMLS_DC)
{
	enum_func_status ret;
	DBG_ENTER("mysqlnd_ms::tx_commit");

	ret = ms_orig_mysqlnd_conn_methods->tx_commit(conn TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ MYSQLND_METHOD(mysqlnd_ms, tx_rollback) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, tx_rollback)(MYSQLND * conn TSRMLS_DC)
{
	enum_func_status ret;
	DBG_ENTER("mysqlnd_ms::tx_rollback");

	ret = ms_orig_mysqlnd_conn_methods->tx_rollback(conn TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */
#endif

static struct st_mysqlnd_conn_methods my_mysqlnd_conn_methods;

/* {{{ mysqlnd_ms_register_hooks*/
void
mysqlnd_ms_register_hooks()
{
	ms_orig_mysqlnd_conn_methods = mysqlnd_conn_get_methods();

	memcpy(&my_mysqlnd_conn_methods, ms_orig_mysqlnd_conn_methods, sizeof(struct st_mysqlnd_conn_methods));

	my_mysqlnd_conn_methods.connect				= MYSQLND_METHOD(mysqlnd_ms, connect);
	my_mysqlnd_conn_methods.query				= MYSQLND_METHOD(mysqlnd_ms, query);
	my_mysqlnd_conn_methods.use_result			= MYSQLND_METHOD(mysqlnd_ms, use_result);
	my_mysqlnd_conn_methods.store_result		= MYSQLND_METHOD(mysqlnd_ms, store_result);
	my_mysqlnd_conn_methods.free_contents		= MYSQLND_METHOD(mysqlnd_ms, free_contents);
	my_mysqlnd_conn_methods.escape_string		= MYSQLND_METHOD(mysqlnd_ms, escape_string);
	my_mysqlnd_conn_methods.change_user			= MYSQLND_METHOD(mysqlnd_ms, change_user);
	my_mysqlnd_conn_methods.ping				= MYSQLND_METHOD(mysqlnd_ms, ping);
	my_mysqlnd_conn_methods.kill_connection		= MYSQLND_METHOD(mysqlnd_ms, kill);
	my_mysqlnd_conn_methods.select_db			= MYSQLND_METHOD(mysqlnd_ms, select_db);
	my_mysqlnd_conn_methods.set_charset			= MYSQLND_METHOD(mysqlnd_ms, set_charset);
	my_mysqlnd_conn_methods.set_server_option	= MYSQLND_METHOD(mysqlnd_ms, set_server_option);
	my_mysqlnd_conn_methods.set_client_option	= MYSQLND_METHOD(mysqlnd_ms, set_client_option);
	my_mysqlnd_conn_methods.next_result			= MYSQLND_METHOD(mysqlnd_ms, next_result);
	my_mysqlnd_conn_methods.more_results		= MYSQLND_METHOD(mysqlnd_ms, more_results);
	my_mysqlnd_conn_methods.get_error_no		= MYSQLND_METHOD(mysqlnd_ms, errno);
	my_mysqlnd_conn_methods.get_error_str		= MYSQLND_METHOD(mysqlnd_ms, error);
	my_mysqlnd_conn_methods.get_sqlstate		= MYSQLND_METHOD(mysqlnd_ms, sqlstate);


	my_mysqlnd_conn_methods.get_thread_id		= MYSQLND_METHOD(mysqlnd_ms, thread_id);
	my_mysqlnd_conn_methods.get_field_count		= MYSQLND_METHOD(mysqlnd_ms, field_count);
	my_mysqlnd_conn_methods.get_last_insert_id	= MYSQLND_METHOD(mysqlnd_ms, insert_id);
	my_mysqlnd_conn_methods.get_affected_rows	= MYSQLND_METHOD(mysqlnd_ms, affected_rows);
	my_mysqlnd_conn_methods.get_warning_count	= MYSQLND_METHOD(mysqlnd_ms, warning_count);
	my_mysqlnd_conn_methods.get_last_message	= MYSQLND_METHOD(mysqlnd_ms, info);
#if MYSQLND_VERSION_ID >= 50009
	my_mysqlnd_conn_methods.set_autocommit		= MYSQLND_METHOD(mysqlnd_ms, set_autocommit);
	my_mysqlnd_conn_methods.tx_commit			= MYSQLND_METHOD(mysqlnd_ms, tx_commit);
	my_mysqlnd_conn_methods.tx_rollback			= MYSQLND_METHOD(mysqlnd_ms, tx_rollback);
#endif
	mysqlnd_conn_set_methods(&my_mysqlnd_conn_methods);
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
