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

/* $Id: header 252479 2008-02-07 19:39:50Z iliaa $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_priv.h"
#include "Zend/zend_ini.h"
#include "Zend/zend_ini_scanner.h"
#include "mysqlnd_ms.h"
#include "mysqlnd_ms_ini.h"


#ifndef zend_llist_get_current
#define zend_llist_get_current(l) zend_llist_get_current_ex(l, NULL)
#endif

/* {{{ zend_llist_get_current_ex */
static void *
zend_llist_get_current_ex(zend_llist *l, zend_llist_position *pos)
{
	zend_llist_position *current = pos ? pos : &l->traverse_ptr;

	if (*current) {
		return (*current)->data;
	} else {
		return NULL;
	}
}
/* }}} */

#ifdef ZTS
MUTEX_T LOCK_global_config_access;
#endif

struct st_mysqlnd_ms_ini_entry
{
	union {
		struct {
			char * c;
			size_t len;
		} str;
		zend_llist * list;
	} value;
	zend_uchar type;
};

struct st_mysqlnd_ms_ini_parser_cb_data
{
	HashTable * global_ini;
	HashTable * current_ini_section;
	char * current_ini_section_name;
};


/* {{{ mysqlnd_ms_ini_list_entry_dtor */
static void
mysqlnd_ms_ini_list_entry_dtor(void * pDest)
{
	char * str = *(char **) pDest;
	TSRMLS_FETCH();
	mnd_free(str);
}
/* }}} */


/* {{{ mysqlnd_ms_global_configuration_section_dtor */
static void
mysqlnd_ms_global_configuration_section_dtor(void * data)
{
	struct st_mysqlnd_ms_ini_entry * entry = * (struct st_mysqlnd_ms_ini_entry **) data;
	TSRMLS_FETCH();
	switch (entry->type) {
		case IS_STRING:
			mnd_free(entry->value.str.c);
			break;
		case IS_ARRAY:
			zend_llist_clean(entry->value.list);
			mnd_free(entry->value.list);
			break;
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Unknown entry type in mysqlnd_ms_global_configuration_section_dtor");
	}
	mnd_free(entry);
}
/* }}} */


/* {{{ mysqlnd_ms_ini_parser_cb */
static void
mysqlnd_ms_ini_parser_cb(zval * key, zval * value, zval * arg3, int callback_type, void * cb_data TSRMLS_DC)
{
	struct st_mysqlnd_ms_ini_parser_cb_data * parser_data = (struct st_mysqlnd_ms_ini_parser_cb_data *) cb_data;

	DBG_ENTER("mysqlnd_ms_ini_parser_cb");
	DBG_INF_FMT("key=%p value=%p", key, value);
	DBG_INF_FMT("current_section=[%s] [%p]", parser_data->current_ini_section_name, parser_data->current_ini_section);

	/* `value` will be NULL for ZEND_INI_PARSER_SECTION */
	if (!key) {
		DBG_VOID_RETURN;
	}

	switch (callback_type) {
		case ZEND_INI_PARSER_ENTRY:
		{
			struct st_mysqlnd_ms_ini_entry * new_entry = NULL;
			DBG_INF("ZEND_INI_PARSER_ENTRY");

			if (!value) {
				DBG_VOID_RETURN;
			}

			convert_to_string_ex(&key);
			convert_to_string_ex(&value);

			new_entry = mnd_calloc(1, sizeof(struct st_mysqlnd_ms_ini_entry));
			new_entry->type = IS_STRING;
			new_entry->value.str.len = Z_STRLEN_P(value);
			new_entry->value.str.c = mnd_pestrndup(Z_STRVAL_P(value), Z_STRLEN_P(value), 1);

			if (FAILURE == zend_hash_add(parser_data->current_ini_section, Z_STRVAL_P(key), Z_STRLEN_P(key) + 1,
										 &new_entry, sizeof(struct st_mysqlnd_ms_ini_entry *), NULL))
			{
				if (SUCCESS == zend_hash_update(parser_data->current_ini_section, Z_STRVAL_P(key), Z_STRLEN_P(key) + 1,
												&new_entry, sizeof(struct st_mysqlnd_ms_ini_entry *), NULL))
				{
					DBG_ERR_FMT("Option [%s] already defined in section [%s]. Overwriting",
									 Z_STRVAL_P(key), parser_data->current_ini_section_name? parser_data->current_ini_section_name:"n/a");
					php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Option [%s] already defined in section [%s]. Overwriting",
									 Z_STRVAL_P(key), parser_data->current_ini_section_name? parser_data->current_ini_section_name:"n/a");
				}
			}
			DBG_INF_FMT("New entry [%s]=[%s]", Z_STRVAL_P(key), Z_STRVAL_P(value));
			break;
		}
		case ZEND_INI_PARSER_SECTION:
		{
			HashTable * new_section = mnd_calloc(1, sizeof(HashTable));
			DBG_INF("ZEND_INI_PARSER_SECTION");
			convert_to_string_ex(&key);
			zend_hash_init(new_section, 2, NULL /* hash_func */, mysqlnd_ms_global_configuration_section_dtor /*dtor*/, 1 /* persistent */);
			zend_hash_update(parser_data->global_ini, Z_STRVAL_P(key), Z_STRLEN_P(key) + 1, &new_section, sizeof(HashTable *), NULL);
			parser_data->current_ini_section = new_section;
			if (parser_data->current_ini_section_name) {
				mnd_efree(parser_data->current_ini_section_name);
			}
			parser_data->current_ini_section_name = mnd_pestrndup(Z_STRVAL_P(key), Z_STRLEN_P(key), 0);

			DBG_INF_FMT("Found section [%s][len=%d] [%p]", Z_STRVAL_P(key), Z_STRLEN_P(key) + 1, new_section);
			break;
		}
		case ZEND_INI_PARSER_POP_ENTRY:
		{
			struct st_mysqlnd_ms_ini_entry ** entry_pp = NULL;
			struct st_mysqlnd_ms_ini_entry * entry = NULL;
			char * value_copy;
			zend_bool newly_created = FALSE;
			DBG_INF("ZEND_INI_PARSER_POP_ENTRY");

			convert_to_string_ex(&key);
			convert_to_string_ex(&value);

			DBG_INF_FMT("[%s]=[%s]", Z_STRVAL_P(key), Z_STRVAL_P(value));

			if (SUCCESS == zend_hash_find(parser_data->current_ini_section, Z_STRVAL_P(key), Z_STRLEN_P(key) + 1, (void **) &entry_pp) && entry_pp) {
				DBG_INF_FMT("Found existing entry...");
				entry = *entry_pp;
				if (entry && entry->type != IS_ARRAY) {
					DBG_INF_FMT("and it is not an array!");
					php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Found [] for an option originally defined without []. Dropping original value");
					entry = NULL;
				}
			}

			if (!entry) {
				entry = mnd_calloc(1, sizeof(struct st_mysqlnd_ms_ini_entry));
				entry->type = IS_ARRAY;
				entry->value.list = mnd_calloc(1, sizeof(zend_llist));
				zend_llist_init(entry->value.list, sizeof(char *), (llist_dtor_func_t) mysqlnd_ms_ini_list_entry_dtor, 1 /* persistent */);

				zend_hash_update(parser_data->current_ini_section, Z_STRVAL_P(key), Z_STRLEN_P(key) + 1, &entry,
								 sizeof(struct st_mysqlnd_ms_ini_entry *), NULL);
				newly_created = TRUE;
			}

			value_copy = mnd_pestrndup(Z_STRVAL_P(value), Z_STRLEN_P(value), 1);
			zend_llist_add_element(entry->value.list, &value_copy);

			/* rewind */
			if (TRUE == newly_created) {
				zend_llist_get_first(entry->value.list);
			}

			break;
		}
		default:
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, MYSQLND_MS_ERROR_PREFIX " Unexpected callback_type while parsing server list ini file");
			break;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_ini_string */
char *
mysqlnd_ms_ini_string(HashTable * config, const char * section, size_t section_len, const char * name, size_t name_len,
					  zend_bool * exists, zend_bool * is_list_value, zend_bool use_lock TSRMLS_DC)
{
	zend_bool tmp_bool;
	char * ret = NULL;
	HashTable ** ini_section;
	DBG_ENTER("mysqlnd_ms_ini_string");
	DBG_INF_FMT("name=%s", name);
	if (exists) {
		*exists = 0;
	} else {
		exists = &tmp_bool;
	}
	if (is_list_value) {
		*is_list_value = 0;
	} else {
		is_list_value = &tmp_bool;
	}

	if (use_lock) {
		MYSQLND_MS_CONFIG_LOCK;
	}
	if (zend_hash_find(config, section, section_len + 1, (void **) &ini_section) == SUCCESS) {
		struct st_mysqlnd_ms_ini_entry ** ini_section_entry_pp;
		struct st_mysqlnd_ms_ini_entry * ini_section_entry;

		if (zend_hash_find(*(HashTable **)ini_section, name, name_len + 1, (void **) &ini_section_entry_pp) == SUCCESS) {
			ini_section_entry = *ini_section_entry_pp;
			switch (ini_section_entry->type) {
				case IS_STRING:
					ret = mnd_pestrndup(ini_section_entry->value.str.c, ini_section_entry->value.str.len, 0);
					*exists = 1;
					break;
				case IS_ARRAY:
				{
					char ** original;
					*is_list_value = 1;
					DBG_INF_FMT("the list has %d entries", zend_llist_count(ini_section_entry->value.list));
					original = zend_llist_get_current(ini_section_entry->value.list);
					if (original && *original) {
						ret = mnd_pestrdup(*original, 0);
						*exists = 1;
						/* move to next position */
						zend_llist_get_next(ini_section_entry->value.list);
					}
					break;
				}
				default:
					php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Unknown entry type in mysqlnd_ms_ini_string");
			}
		}
	}
	if (use_lock) {
		MYSQLND_MS_CONFIG_UNLOCK;
	}

	DBG_INF_FMT("ret=%s", ret? ret:"n/a");

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_ini_string_array_reset_pos */
char *
mysqlnd_ms_ini_reset_section(HashTable * config, const char * section, size_t section_len, zend_bool use_lock TSRMLS_DC)
{
	char * ret = NULL;
	HashTable ** ini_section;
	DBG_ENTER("mysqlnd_ms_ini_string_array_reset_pos");
	DBG_INF_FMT("section=%s", section);

	if (use_lock) {
		MYSQLND_MS_CONFIG_LOCK;
	}
	if (zend_hash_find(config, section, section_len + 1, (void **) &ini_section) == SUCCESS) {
		HashPosition pos;
		struct st_mysqlnd_ms_ini_entry ** ini_section_entry_pp;

		zend_hash_internal_pointer_reset_ex(*(HashTable **)ini_section, &pos);
		while (zend_hash_get_current_data_ex(*(HashTable **)ini_section, (void **) &ini_section_entry_pp, &pos) == SUCCESS) {
			if (IS_ARRAY == (*ini_section_entry_pp)->type) {
				zend_llist_get_first((*ini_section_entry_pp)->value.list);
			}
			zend_hash_move_forward_ex(*(HashTable **)ini_section, &pos);
		}
	}
	if (use_lock) {
		MYSQLND_MS_CONFIG_UNLOCK;
	}

	DBG_INF_FMT("ret=%s", ret? ret:"n/a");

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_init_server_list */
enum_func_status
mysqlnd_ms_init_server_list(HashTable * configuration TSRMLS_DC)
{
	enum_func_status ret = PASS;
	char * ini_file = INI_STR("mysqlnd_ms.ini_file");
	DBG_ENTER("mysqlnd_ms_init_server_list");
	DBG_INF_FMT("ini_file=%s", ini_file? ini_file:"n/a");

	if (ini_file) {
		HashTable * global_section = mnd_calloc(1, sizeof(HashTable));
		struct st_mysqlnd_ms_ini_parser_cb_data ini_parse_data = {0};
		zend_file_handle fh = {0};

		zend_hash_init(global_section, 2, NULL /* hash_func */, mysqlnd_ms_global_configuration_section_dtor /*dtor*/, 1 /* persistent */);
		zend_hash_update(configuration, GLOBAL_SECTION_NAME, sizeof(GLOBAL_SECTION_NAME), &global_section, sizeof(HashTable *), NULL);

		ini_parse_data.global_ini = configuration;
		ini_parse_data.current_ini_section = global_section;
		ini_parse_data.current_ini_section_name = mnd_pestrndup(GLOBAL_SECTION_NAME, sizeof(GLOBAL_SECTION_NAME) - 1, 0);

		fh.filename = ini_file;
		fh.type = ZEND_HANDLE_FILENAME;

		if (FAILURE == zend_parse_ini_file(&fh, 0, ZEND_INI_SCANNER_NORMAL, mysqlnd_ms_ini_parser_cb, &ini_parse_data TSRMLS_CC)) {
			char *cwd_ret = NULL;
			char cwd[MAXPATHLEN];
#if HAVE_GETCWD
			cwd_ret = VCWD_GETCWD(cwd, MAXPATHLEN);
#elif HAVE_GETWD
			cwd_ret = VCWD_GETWD(cwd);
#endif
			php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Failed to parse server list ini file %s (current working dir: %s)", ini_file, cwd_ret ? cwd : "(unknown)");
			ret = FAIL;
		}
		if (ini_parse_data.current_ini_section_name) {
			mnd_efree(ini_parse_data.current_ini_section_name);
			ini_parse_data.current_ini_section_name = NULL;
		}
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_ini_section_exists */
zend_bool
mysqlnd_ms_ini_section_exists(HashTable * config, const char * section, size_t section_len, zend_bool use_lock TSRMLS_DC)
{
	zend_bool ret = FALSE;
	char ** ini_entry;
	DBG_ENTER("mysqlnd_ms_ini_section_exists");
	DBG_INF_FMT("section=[%s] len=[%d]", section? section:"n/a", section_len);

	if (config && section && section_len) {
		if (use_lock) {
			MYSQLND_MS_CONFIG_LOCK;
		}
		ret = (SUCCESS == zend_hash_find(config, section, section_len + 1, (void **) &ini_entry))? TRUE:FALSE;
		if (use_lock) {
			MYSQLND_MS_CONFIG_UNLOCK;
		}
	}

	DBG_INF_FMT("ret=%d", ret);
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
