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
#include "mysqlnd_ms_config_json.h"
#include "mysqlnd_qp.h"

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
/* }}} */


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
enum_func_status
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
					new_filter_entry->host_id = mnd_pestrndup(section_name, section_name_len, 1);
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
