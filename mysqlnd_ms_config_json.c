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
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_priv.h"

#include "mysqlnd_ms.h"
#include "mysqlnd_ms_config_json.h"

#include "ext/json/JSON_parser.h"
#include "ext/json/php_json.h"

struct st_mysqlnd_ms_json_config {
	HashTable * config;
#ifdef ZTS
	MUTEX_T LOCK_access;
#endif	
};


struct st_mysqlnd_ms_config_json_entry
{
	union {
		struct {
			char * c;
			size_t len;
		} str;
		HashTable * ht;
		double dval;
		int64_t lval;
	} value;
	zend_uchar type;
};


/* {{{ mysqlnd_ms_config_json_init */
PHPAPI struct st_mysqlnd_ms_json_config *
mysqlnd_ms_config_json_init(TSRMLS_D)
{
	struct st_mysqlnd_ms_json_config * ret;
	DBG_ENTER("mysqlnd_ms_config_json_init");
	ret = mnd_calloc(1, sizeof(struct st_mysqlnd_ms_json_config));
#ifdef ZTS
	ret->LOCK_access = tsrm_mutex_alloc();
#endif
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_config_json_free */
PHPAPI void
mysqlnd_ms_config_json_free(struct st_mysqlnd_ms_json_config * cfg TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms_config_json_free");
	if (cfg) {
		if (cfg->config) {
			zend_hash_destroy(cfg->config);
			mnd_free(cfg->config);
		}
#ifdef ZTS
		tsrm_mutex_free(cfg->LOCK_access);
#endif
		mnd_free(cfg);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_config_json_section_dtor */
static void
mysqlnd_ms_config_json_section_dtor(void * data)
{
	struct st_mysqlnd_ms_config_json_entry * entry = * (struct st_mysqlnd_ms_config_json_entry **) data;
	TSRMLS_FETCH();
	switch (entry->type) {
		case IS_DOUBLE:
		case IS_LONG:
			break;
		case IS_STRING:
			mnd_free(entry->value.str.c);
			break;
		case IS_ARRAY:
			zend_hash_destroy(entry->value.ht);
			mnd_free(entry->value.ht);
			break;
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX
					" Unknown entry type %d  in mysqlnd_ms_config_json_section_dtor", entry->type);
	}
	mnd_free(entry);
}
/* }}} */

static HashTable * mysqlnd_ms_zval_data_to_hashtable(zval * json_data TSRMLS_DC);

/* {{{ mysqlnd_ms_add_zval_to_hash */
static void
mysqlnd_ms_add_zval_to_hash(zval * zv, HashTable * ht, const char * skey, size_t skey_len, ulong nkey, int key_type TSRMLS_DC)
{
	struct st_mysqlnd_ms_config_json_entry * new_entry;
	DBG_ENTER("mysqlnd_ms_add_zval_to_hash");
	new_entry = mnd_calloc(1, sizeof(struct st_mysqlnd_ms_config_json_entry));
		
	switch (Z_TYPE_P(zv)) {
		case IS_ARRAY:
			new_entry->type = IS_ARRAY;
			new_entry->value.ht = mysqlnd_ms_zval_data_to_hashtable(zv TSRMLS_CC);
			break;
		case IS_STRING:
			new_entry->type = IS_STRING;
			new_entry->value.str.c = mnd_pestrndup(Z_STRVAL_P(zv), Z_STRLEN_P(zv), 1);
			new_entry->value.str.len = Z_STRLEN_P(zv);
			break;
		case IS_DOUBLE:
			new_entry->type = IS_DOUBLE;
			new_entry->value.dval = Z_DVAL_P(zv);
			break;
		case IS_LONG:
			new_entry->type = IS_LONG;
			new_entry->value.lval = Z_LVAL_P(zv);
			break;
		case IS_NULL:
			new_entry->type = IS_NULL;
			break;
		default:
			mnd_free(new_entry);
			new_entry = NULL;
			break;
	}
	if (new_entry) {
		switch (key_type) {
			case HASH_KEY_IS_STRING:
				zend_hash_add(ht, skey, skey_len, &new_entry, sizeof(struct st_mysqlnd_ms_config_json_entry *), NULL);
				DBG_INF_FMT("New HASH_KEY_IS_STRING entry [%s]", skey);
				break;
			default:
				zend_hash_index_update(ht, nkey, &new_entry, sizeof(struct st_mysqlnd_ms_config_json_entry *), NULL);
				DBG_INF_FMT("New HASH_KEY_IS_LONG entry [%u]", nkey);
				break;
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_zval_data_to_hashtable */
static HashTable *
mysqlnd_ms_zval_data_to_hashtable(zval * json_data TSRMLS_DC)
{
	HashTable * ret = NULL;

	DBG_ENTER("mysqlnd_ms_zval_data_to_hashtable");
	if (json_data && (ret = mnd_calloc(1, sizeof(HashTable)))) {
		HashPosition pos;
		zval ** entry_zval;

		zend_hash_init(ret, Z_TYPE_P(json_data) == IS_ARRAY? zend_hash_num_elements(Z_ARRVAL_P(json_data)) : 1,
						NULL /* hash_func */, mysqlnd_ms_config_json_section_dtor /*dtor*/, 1 /* persistent */);

		if (Z_TYPE_P(json_data) == IS_ARRAY) {
			zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(json_data), &pos);
			while (zend_hash_get_current_data_ex(Z_ARRVAL_P(json_data), (void **)&entry_zval, &pos) == SUCCESS) {
				char * skey;
				uint skey_len;
				ulong nkey;
				int key_type = zend_hash_get_current_key_ex(Z_ARRVAL_P(json_data), &skey, &skey_len, &nkey, 0/*dup*/, &pos);

				mysqlnd_ms_add_zval_to_hash(*entry_zval, ret, skey, skey_len, nkey, key_type TSRMLS_CC);

				zend_hash_move_forward_ex(Z_ARRVAL_P(json_data), &pos);
			} /* while */
		} else {
		
		}
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_config_json_load_configuration */
PHPAPI enum_func_status
mysqlnd_ms_config_json_load_configuration(struct st_mysqlnd_ms_json_config * cfg TSRMLS_DC)
{
	enum_func_status ret = FAIL;
	char * json_file_name = INI_STR("mysqlnd_ms.ini_file");
	DBG_ENTER("mysqlnd_ms_config_json_load_configuration");
	DBG_INF_FMT("json_file=%s", json_file_name? json_file_name:"n/a");
	if (!json_file_name) {
		ret = PASS;
	} else if (json_file_name && cfg) {
		do {
			php_stream * stream;
			int str_data_len;
			char * str_data;
			zval json_data;
			stream = php_stream_open_wrapper_ex(json_file_name, "rb", REPORT_ERRORS, NULL, NULL);

			if (!stream) {
				break;
			}
			str_data_len = php_stream_copy_to_mem(stream, &str_data, PHP_STREAM_COPY_ALL, 0);
			php_stream_close(stream);
			if (str_data_len <= 0) {
				break;
			}
#if PHP_VERSION_ID >= 50399
			php_json_decode_ex(&json_data, str_data, str_data_len, PHP_JSON_OBJECT_AS_ARRAY, JSON_PARSER_DEFAULT_DEPTH TSRMLS_CC);
#else
			php_json_decode(&json_data, str_data, str_data_len, 1 /* assoc */, JSON_PARSER_DEFAULT_DEPTH TSRMLS_CC);
#endif
			cfg->config = mysqlnd_ms_zval_data_to_hashtable(&json_data TSRMLS_CC);
			zval_dtor(&json_data);
			efree(str_data);
			if (!cfg->config) {
				break;
			}
			ret = PASS;
		} while (0);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_config_json_section_exists */
PHPAPI zend_bool
mysqlnd_ms_config_json_section_exists(struct st_mysqlnd_ms_json_config * cfg, const char * section, size_t section_len,
									  zend_bool use_lock TSRMLS_DC)
{
	zend_bool ret = FALSE;
	DBG_ENTER("mysqlnd_ms_config_json_section_exists");
	DBG_INF_FMT("section=[%s] len=[%d]", section? section:"n/a", section_len);

	if (cfg && section && section_len) {
		char ** ini_entry;
		if (use_lock) {
			MYSQLND_MS_CONFIG_JSON_LOCK(cfg);
		}
		if (cfg->config) {
			ret = (SUCCESS == zend_hash_find(cfg->config, section, section_len + 1, (void **) &ini_entry))? TRUE:FALSE;
		}
		if (use_lock) {
			MYSQLND_MS_CONFIG_JSON_UNLOCK(cfg);
		}
	}

	DBG_INF_FMT("ret=%d", ret);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_config_json_string */
PHPAPI char *
mysqlnd_ms_config_json_string(struct st_mysqlnd_ms_json_config * cfg, const char * section, size_t section_len,
							  const char * name, size_t name_len,
							  zend_bool * exists, zend_bool * is_list_value, zend_bool use_lock TSRMLS_DC)
{
	zend_bool tmp_bool;
	char * ret = NULL;

	DBG_ENTER("mysqlnd_ms_config_json_string");
	DBG_INF_FMT("name=%s", name);
	
	if (!cfg) {
		DBG_RETURN(ret);
	}
	
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
		MYSQLND_MS_CONFIG_JSON_LOCK(cfg);
	}
	if (cfg->config) {
		struct st_mysqlnd_ms_config_json_entry ** ini_section;
		if (zend_hash_find(cfg->config, section, section_len + 1, (void **) &ini_section) == SUCCESS) {
			struct st_mysqlnd_ms_config_json_entry * ini_section_entry = NULL;

			switch ((*ini_section)->type) {
				case IS_LONG:
				case IS_DOUBLE:
				case IS_STRING:
					ini_section_entry = *ini_section;
					break;
					break;
				case IS_ARRAY: {
					struct st_mysqlnd_ms_config_json_entry ** ini_section_entry_pp;
					if (zend_hash_find((*ini_section)->value.ht, name, name_len + 1, (void **) &ini_section_entry_pp) == SUCCESS) {
						ini_section_entry = *ini_section_entry_pp;
					}
					break;
				}
			}	
			if (ini_section_entry) {
				const char * spec_type;
				switch (ini_section_entry->type) {
					case IS_LONG:
						spec_type = MYSQLND_LL_SPEC;
						goto long_or_double_to_str;
					case IS_DOUBLE:
						spec_type = "%f";
long_or_double_to_str:
						{
							char * tmp_buf;
							int tmp_buf_len = spprintf(&tmp_buf, 0, spec_type,
										ini_section_entry->type==IS_LONG? ini_section_entry->value.lval:
																		  ini_section_entry->value.dval);
							ret = mnd_pestrndup(tmp_buf, tmp_buf_len, 0);
							efree(tmp_buf);
						}
						*exists = 1;
						break;
					case IS_STRING:
						ret = mnd_pestrndup(ini_section_entry->value.str.c, ini_section_entry->value.str.len, 0);
						*exists = 1;
						break;
					case IS_ARRAY:
					{
						struct st_mysqlnd_ms_config_json_entry ** value;
						*is_list_value = 1;
						DBG_INF_FMT("the list has %d entries", zend_hash_num_elements(ini_section_entry->value.ht));
						if (SUCCESS == zend_hash_get_current_data(ini_section_entry->value.ht, (void **)&value)) {	
							switch ((*value)->type) {
								case IS_STRING:
									ret = mnd_pestrndup((*value)->value.str.c, (*value)->value.str.len, 0);
									*exists = 1;
									break;
								case IS_LONG:
									spec_type = MYSQLND_LL_SPEC;
									goto long_or_double_to_str2;
								case IS_DOUBLE:
									spec_type = "%f";
long_or_double_to_str2:
									{
										char * tmp_buf;
										int tmp_buf_len = spprintf(&tmp_buf, 0, spec_type,
													(*value)->type==IS_LONG? (*value)->value.lval:(*value)->value.dval);
										ret = mnd_pestrndup(tmp_buf, tmp_buf_len, 0);
										efree(tmp_buf);
									}
									*exists = 1;
									break;
								case IS_ARRAY:
									DBG_ERR("still unsupported type");
									/* to do */
									break;
							}
							zend_hash_move_forward(ini_section_entry->value.ht);
						}
						break;
					}
					default:
						php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX
									" Unknown entry type %d in mysqlnd_ms_config_json_string", ini_section_entry->type);
						break;
				}
			}
		} /* if (zend_hash... */
	} /* if (cfg->config) */
	if (use_lock) {
		MYSQLND_MS_CONFIG_JSON_UNLOCK(cfg);
	}

	DBG_INF_FMT("ret=%s", ret? ret:"n/a");

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_str_to_long_long */
static long long
mysqlnd_ms_str_to_long_long(const char * const s, zend_bool * valid)
{
	long long ret;
	char * end_ptr;
	errno = 0;
	ret = strtoll(s, &end_ptr, 10);
	if (
			(
				(errno == ERANGE && (ret == LLONG_MAX || ret == LLONG_MIN))
				||
				(errno != 0 && ret == 0)
			)
			||
			end_ptr == s
		)
	{
		*valid = 0;
		return 0;
	}
	*valid = 1;
	return ret;
}
/* }}} */


/* {{{ mysqlnd_ms_config_json_int */
PHPAPI int64_t
mysqlnd_ms_config_json_int(struct st_mysqlnd_ms_json_config * cfg, const char * section, size_t section_len,
						   const char * name, size_t name_len,
						   zend_bool * exists, zend_bool * is_list_value, zend_bool use_lock TSRMLS_DC)
{
	zend_bool tmp_bool;
	int64_t ret = 0;

	DBG_ENTER("mysqlnd_ms_config_json_int");
	DBG_INF_FMT("name=%s", name);
	
	if (!cfg) {
		DBG_RETURN(ret);
	}
	
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
		MYSQLND_MS_CONFIG_JSON_LOCK(cfg);
	}
	if (cfg->config) {
		struct st_mysqlnd_ms_config_json_entry ** ini_section;
		if (zend_hash_find(cfg->config, section, section_len + 1, (void **) &ini_section) == SUCCESS) {
			struct st_mysqlnd_ms_config_json_entry * ini_section_entry = NULL;

			switch ((*ini_section)->type) {
				case IS_LONG:
				case IS_DOUBLE:
				case IS_STRING:
					ini_section_entry = *ini_section;
					break;
				case IS_ARRAY: {
					struct st_mysqlnd_ms_config_json_entry ** ini_section_entry_pp;
					if (zend_hash_find((*ini_section)->value.ht, name, name_len + 1, (void **) &ini_section_entry_pp) == SUCCESS) {
						ini_section_entry = *ini_section_entry_pp;
					}
					break;
				}
			}	
			if (ini_section_entry) {
				switch (ini_section_entry->type) {
					case IS_LONG:
						ret = ini_section_entry->value.lval;
						*exists = 1;
						break;
					case IS_DOUBLE:
						ret = (int64_t) ini_section_entry->value.dval;
						*exists = 1;
						break;
					case IS_STRING:
						ret = mysqlnd_ms_str_to_long_long(ini_section_entry->value.str.c, exists);
						break;
					case IS_ARRAY:
					{
						struct st_mysqlnd_ms_config_json_entry ** value;
						*is_list_value = 1;
						DBG_INF_FMT("the list has %d entries", zend_hash_num_elements(ini_section_entry->value.ht));
						if (SUCCESS == zend_hash_get_current_data(ini_section_entry->value.ht, (void **)&value)) {	
							switch ((*value)->type) {
								case IS_STRING:
									ret = mysqlnd_ms_str_to_long_long((*value)->value.str.c, exists);
									break;
								case IS_LONG:
									ret = (*value)->value.lval;
									break;
								case IS_DOUBLE:
									ret = (int64_t) (*value)->value.dval;
									*exists = 1;
									break;
								case IS_ARRAY:
									DBG_ERR("still unsupported type");
									break;
									/* to do */
							}
							zend_hash_move_forward(ini_section_entry->value.ht);
						}
						break;
					}
					default:
						php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX
									" Unknown entry type %d in mysqlnd_ms_config_json_int", ini_section_entry->type);
						break;
				}
			}
		} /* if (zend_hash... */
	} /* if (cfg->config) */
	if (use_lock) {
		MYSQLND_MS_CONFIG_JSON_UNLOCK(cfg);
	}

	DBG_INF_FMT("ret="MYSQLND_LL_SPEC, (long long) ret);

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_str_to_double */
static double
mysqlnd_ms_str_to_double(const char * const s, zend_bool * valid)
{
	double ret;
	char * end_ptr;
	errno = 0;
	ret = strtod(s, &end_ptr);
	if (
			(
				(errno == ERANGE && (ret == HUGE_VALF || ret == HUGE_VALL))
				||
				(errno != 0 && ret == 0)
			)
			||
			end_ptr == s
		)
	{
		*valid = 0;
		return 0;
	}
	*valid = 1;
	return ret;
}
/* }}} */


/* {{{ mysqlnd_ms_config_json_double */
PHPAPI double
mysqlnd_ms_config_json_double(struct st_mysqlnd_ms_json_config * cfg, const char * section, size_t section_len,
							  const char * name, size_t name_len,
							  zend_bool * exists, zend_bool * is_list_value, zend_bool use_lock TSRMLS_DC)
{
	zend_bool tmp_bool;
	double ret = 0;

	DBG_ENTER("mysqlnd_ms_config_json_double");
	DBG_INF_FMT("name=%s", name);
	
	if (!cfg) {
		DBG_RETURN(ret);
	}
	
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
		MYSQLND_MS_CONFIG_JSON_LOCK(cfg);
	}
	if (cfg->config) {
		struct st_mysqlnd_ms_config_json_entry ** ini_section;
		if (zend_hash_find(cfg->config, section, section_len + 1, (void **) &ini_section) == SUCCESS) {
			struct st_mysqlnd_ms_config_json_entry * ini_section_entry = NULL;

			switch ((*ini_section)->type) {
				case IS_LONG:
				case IS_DOUBLE:
				case IS_STRING:
					ini_section_entry = *ini_section;
					break;
				case IS_ARRAY: {
					struct st_mysqlnd_ms_config_json_entry ** ini_section_entry_pp;
					if (zend_hash_find((*ini_section)->value.ht, name, name_len + 1, (void **) &ini_section_entry_pp) == SUCCESS) {
						ini_section_entry = *ini_section_entry_pp;
					}
					break;
				}
			}	
			if (ini_section_entry) {
				switch (ini_section_entry->type) {
					case IS_LONG:
						ret = (double) ini_section_entry->value.lval;
						*exists = 1;
						break;
					case IS_DOUBLE:
						ret = ini_section_entry->value.dval;
						*exists = 1;
						break;
					case IS_STRING:
						ret = mysqlnd_ms_str_to_double(ini_section_entry->value.str.c, exists);
						break;
					case IS_ARRAY:
					{
						struct st_mysqlnd_ms_config_json_entry ** value;
						*is_list_value = 1;
						DBG_INF_FMT("the list has %d entries", zend_hash_num_elements(ini_section_entry->value.ht));
						if (SUCCESS == zend_hash_get_current_data(ini_section_entry->value.ht, (void **)&value)) {	
							switch ((*value)->type) {
								case IS_STRING:
									ret = mysqlnd_ms_str_to_double((*value)->value.str.c, exists);
									break;
								case IS_LONG:
									ret = (double) (*value)->value.lval;
									*exists = 1;
									break;
								case IS_DOUBLE:
									ret = (*value)->value.dval;
									*exists = 1;
									break;
								case IS_ARRAY:
									DBG_ERR("still unsupported type");
									break;
									/* to do */
							}
							zend_hash_move_forward(ini_section_entry->value.ht);
						}
						break;
					}
					default:
						php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX
									" Unknown entry type %d in mysqlnd_ms_config_json_double", ini_section_entry->type);
						break;
				}
			}
		} /* if (zend_hash... */
	} /* if (cfg->config) */
	if (use_lock) {
		MYSQLND_MS_CONFIG_JSON_UNLOCK(cfg);
	}

	DBG_INF_FMT("ret=%d", ret);

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_config_json_reset_section */
PHPAPI void
mysqlnd_ms_config_json_reset_section(struct st_mysqlnd_ms_json_config * cfg, const char * section, size_t section_len,
									 zend_bool use_lock TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms_config_json_reset_section");
	DBG_INF_FMT("section=%s", section);

	if (cfg) {
		if (use_lock) {
			MYSQLND_MS_CONFIG_JSON_LOCK(cfg);
		}
		if (cfg->config) {
			struct st_mysqlnd_ms_config_json_entry ** ini_section;
			if (zend_hash_find(cfg->config, section, section_len + 1, (void **) &ini_section) == SUCCESS) {
				if ((*ini_section)->type == IS_ARRAY) {
					struct st_mysqlnd_ms_config_json_entry ** ini_section_entry_pp;
					HashPosition pos;

					zend_hash_internal_pointer_reset_ex((*ini_section)->value.ht, &pos);
					while (zend_hash_get_current_data_ex((*ini_section)->value.ht, (void **) &ini_section_entry_pp, &pos) == SUCCESS) {
						if (IS_ARRAY == (*ini_section_entry_pp)->type) {
							zend_hash_internal_pointer_reset((*ini_section_entry_pp)->value.ht);
						}
						zend_hash_move_forward_ex((*ini_section)->value.ht, &pos);
					}
				}
			}
		}
		if (use_lock) {
			MYSQLND_MS_CONFIG_JSON_UNLOCK(cfg);
		}
	}

	DBG_VOID_RETURN;

}
/* }}} */


#ifdef ZTS
/* {{{ mysqlnd_ms_config_json_lock */
PHPAPI void
mysqlnd_ms_config_json_lock(struct st_mysqlnd_ms_json_config * cfg, const char * const file, unsigned int line TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms_config_json_lock");
	DBG_INF_FMT("mutex=%p file=%s line=%u", cfg->LOCK_access, file, line);
	tsrm_mutex_lock(cfg->LOCK_access);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_config_json_unlock */
PHPAPI void
mysqlnd_ms_config_json_unlock(struct st_mysqlnd_ms_json_config * cfg, const char * const file, unsigned int line TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms_config_json_unlock");
	DBG_INF_FMT("mutex=%p file=%s line=%u", cfg->LOCK_access, file, line);
	tsrm_mutex_unlock(cfg->LOCK_access);
	DBG_VOID_RETURN;
}
/* }}} */
#endif
