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

/* $Id: $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mysqlnd_ms_conn_pool.h"
#include "mysqlnd_ms.h"

#if PHP_VERSION_ID >= 50400
#include "ext/mysqlnd/mysqlnd_ext_plugin.h"
#endif
#ifndef mnd_emalloc
#include "ext/mysqlnd/mysqlnd_alloc.h"
#endif

/* {{{ mysqlnd_ms_pool_all_list_dtor */
void
mysqlnd_ms_pool_all_list_dtor(void * pDest)
{
	MYSQLND_MS_POOL_ENTRY * element = pDest? *(MYSQLND_MS_POOL_ENTRY **) pDest : NULL;
	TSRMLS_FETCH();

	DBG_ENTER("mysqlnd_ms_pool_all_list_dtor");

	if (!element) {
		DBG_VOID_RETURN;
	}
	if (element->ms_list_data_dtor) {
		element->ms_list_data_dtor((void *)&element->data);
	}
	mnd_pefree(element, element->persistent);

	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ mysqlnd_ms_pool_cmd_dtor */
void
mysqlnd_ms_pool_cmd_dtor(void *pDest) {
	MYSQLND_MS_POOL_CMD * element = pDest ? *(MYSQLND_MS_POOL_CMD**)pDest : NULL;
	TSRMLS_FETCH();

	DBG_ENTER("mysqlnd_ms_pool_cmd_dtor");

	if (element) {
		switch (element->cmd) {
			case SET_CHARSET:
				DBG_INF_FMT("pool_cmd=%p SET_CHARSET", element->data);
				{
					MYSQLND_MS_POOL_CMD_SET_CHARSET * arg = (MYSQLND_MS_POOL_CMD_SET_CHARSET *)element->data;
					if (arg->csname) {
						mnd_pefree(arg->csname, element->persistent);
					}
				}
				break;

			case SELECT_DB:
				DBG_INF_FMT("pool_cmd=%p SELECT_DB", element->data);
				{
					MYSQLND_MS_POOL_CMD_SELECT_DB * arg = (MYSQLND_MS_POOL_CMD_SELECT_DB *)element->data;
					if (arg->db) {
						mnd_pefree(arg->db, element->persistent);
					}
					mnd_pefree(arg, element->persistent);
				}
				break;

			case SET_SERVER_OPTION:
				DBG_INF_FMT("pool_cmd=%p SET_SERVER_OPTION", element->data);
				mnd_pefree(element->data, element->persistent);
				break;

			case SET_CLIENT_OPTION:
				DBG_INF_FMT("pool_cmd=%p SET_CLIENT_OPTION", element->data);
				{
					MYSQLND_MS_POOL_CMD_SET_CLIENT_OPTION * arg = (MYSQLND_MS_POOL_CMD_SET_CLIENT_OPTION *)element->data;
					if (arg->value) {
						mnd_pefree(arg->value, element->persistent);
					}
					mnd_pefree(arg, element->persistent);
				}
				break;
		}
		mnd_pefree(element, element->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ pool_flush */
static enum_func_status
pool_flush_active(MYSQLND_MS_POOL * pool TSRMLS_DC)
{
	MYSQLND_MS_LIST_DATA * el;
	MYSQLND_MS_POOL_ENTRY ** pool_element = NULL;
	enum_func_status ret = PASS;

	DBG_ENTER("mysqlnd_ms::pool_flush_active");

	BEGIN_ITERATE_OVER_SERVER_LIST(el, &(pool->data.active_master_list))
	{
		if (SUCCESS == zend_hash_find(&(pool->data.master_list), el->pool_hash_key.c, el->pool_hash_key.len, (void**)&pool_element)) {
			(*pool_element)->active = FALSE;
		} else
		{
			ret = FAIL;
			DBG_INF_FMT("Pool master list is inconsistent, key=%s", el->pool_hash_key.c);
		}
	}
	END_ITERATE_OVER_SERVER_LIST;
	zend_llist_clean(&(pool->data.active_master_list));
	MYSQLND_MS_INC_STATISTIC_W_VALUE(MS_STAT_POOL_MASTERS_ACTIVE, 0);

	BEGIN_ITERATE_OVER_SERVER_LIST(el, &(pool->data.active_slave_list))
	{
		if (SUCCESS == zend_hash_find(&(pool->data.slave_list), el->pool_hash_key.c, el->pool_hash_key.len, (void**)&pool_element)) {
			(*pool_element)->active = FALSE;
		} else
		{
			ret = FAIL;
			DBG_INF_FMT("Pool slave list is inconsistent, key=%s", el->pool_hash_key.c);
		}
	}
	END_ITERATE_OVER_SERVER_LIST;
	zend_llist_clean(&(pool->data.active_slave_list));
	MYSQLND_MS_INC_STATISTIC_W_VALUE(MS_STAT_POOL_SLAVES_ACTIVE, 0);

	DBG_RETURN(ret);
}
/* }}} */

static int
pool_clear_all_ht_apply_func(void *pDest, void * argument TSRMLS_DC)
{
	int ret = ZEND_HASH_APPLY_REMOVE;
	MYSQLND_MS_POOL_ENTRY * pool_element = (MYSQLND_MS_POOL_ENTRY *)pDest;
	unsigned int * referenced = (unsigned int *)argument;

	if (pool_element->ref_counter > 1) {
		*referenced++;
		ret = ZEND_HASH_APPLY_KEEP;
	} else {
		pool_element->active = FALSE;
	}
	return ret;
}

/* {{{ pool_clear_all */
static enum_func_status
pool_clear_all(MYSQLND_MS_POOL *pool, unsigned int * referenced TSRMLS_DC)
{
	enum_func_status ret = PASS;
	HashPosition pos;
	HashTable *ht[2];
	int i;
	MYSQLND_MS_POOL_ENTRY ** pool_element = NULL;

	DBG_ENTER("pool_clear_all");
	*referenced = 0;

	/* Active list will always be flushed */
	zend_llist_clean(&(pool->data.active_master_list));
	zend_llist_clean(&(pool->data.active_slave_list));

	zend_hash_apply_with_argument(&(pool->data.master_list), pool_clear_all_ht_apply_func, (void *)referenced TSRMLS_CC);
	zend_hash_apply_with_argument(&(pool->data.slave_list), pool_clear_all_ht_apply_func, (void *)referenced TSRMLS_CC);

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ pool_get_conn_hash_key */
static void
pool_get_conn_hash_key(smart_str * hash_key,
						const char * unique_name_from_config,
						const char * host, const char * user,
						const char * passwd, size_t passwd_len,	unsigned int port,
						char * socket, char * db, size_t db_len, unsigned long connect_flags,
						zend_bool persistent)
{
	if (unique_name_from_config) {
		smart_str_appendl(hash_key, unique_name_from_config, strlen(unique_name_from_config));
	}
	if (host) {
		smart_str_appendl(hash_key, host, strlen(host));
	}
	if (user) {
		smart_str_appendl(hash_key, user, strlen(user));
	}
	if (passwd_len) {
		smart_str_appendl(hash_key, passwd, passwd_len);
	}
	if (socket) {
		smart_str_appendl(hash_key, socket, strlen(socket));
	}
	smart_str_append_unsigned(hash_key, port);
	if (db_len) {
		smart_str_appendl(hash_key, db, db_len);
	}
	smart_str_append_unsigned(hash_key, connect_flags);
	smart_str_appendc(hash_key, '\0');
}
/* }}} */



/* {{{ pool_init_pool_hash_key */
static void
pool_init_pool_hash_key(MYSQLND_MS_LIST_DATA * data)
{
	/* Note, accessing "private" MS_LIST_DATA contents */
	if (data->pool_hash_key.len) {
		smart_str_free(&(data->pool_hash_key));
	}
	pool_get_conn_hash_key(&(data->pool_hash_key),
							data->name_from_config,
							data->host, data->user,	data->passwd, data->passwd_len,
							data->port, data->socket, data->db, data->db_len,
							data->connect_flags, data->persistent);
}
/* }}} */

/* {{{ pool_add_slave */
static enum_func_status
pool_add_slave(MYSQLND_MS_POOL * pool, smart_str * hash_key,
			   MYSQLND_MS_LIST_DATA * data, zend_bool persistent TSRMLS_DC)
{
	enum_func_status ret = PASS;
	MYSQLND_MS_POOL_ENTRY * pool_element;

	DBG_ENTER("mysqlnd_ms::pool_add_slave");

	pool_element = mnd_pecalloc(1, sizeof(MYSQLND_MS_POOL_ENTRY), persistent);
	if (!pool_element) {
		MYSQLND_MS_WARN_OOM();
		ret = FAIL;
	} else {
		DBG_INF_FMT("pool_element=%d data=%p", pool_element, data);
		pool_element->data = data;
		if (pool->data.ms_list_data_dtor) {
			pool_element->ms_list_data_dtor = pool->data.ms_list_data_dtor;
		}
		pool_element->persistent = persistent;
		pool_element->active = TRUE;
		pool_element->activation_counter = 1;
		pool_element->ref_counter = 1;

		if (SUCCESS != zend_hash_add(&(pool->data.slave_list), hash_key->c, hash_key->len, &pool_element, sizeof(MYSQLND_MS_POOL_ENTRY *), NULL)) {
			DBG_INF_FMT("Failed adding, duplicate hash key, added twice?");
			ret = FAIL;
		}
		zend_llist_add_element(&(pool->data.active_slave_list), &pool_element->data);

		MYSQLND_MS_INC_STATISTIC_W_VALUE(MS_STAT_POOL_SLAVES_TOTAL, zend_hash_num_elements(&(pool->data.slave_list)));
		MYSQLND_MS_INC_STATISTIC_W_VALUE(MS_STAT_POOL_SLAVES_ACTIVE, zend_llist_count(&(pool->data.active_slave_list)));
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ pool_add_master */
static enum_func_status
pool_add_master(MYSQLND_MS_POOL * pool, smart_str * hash_key,
				MYSQLND_MS_LIST_DATA * data, zend_bool persistent TSRMLS_DC)
{
	enum_func_status ret = PASS;
	MYSQLND_MS_POOL_ENTRY * pool_element;

	DBG_ENTER("mysqlnd_ms::pool_add_master");

	pool_element = mnd_pecalloc(1, sizeof(MYSQLND_MS_POOL_ENTRY), persistent);
	if (!pool_element) {
		MYSQLND_MS_WARN_OOM();
		ret = FAIL;
	} else {
		DBG_INF_FMT("pool_element=%d data=%p", pool_element, data);

		pool_element->data = data;
		if (pool->data.ms_list_data_dtor) {
			pool_element->ms_list_data_dtor = pool->data.ms_list_data_dtor;
		}
		pool_element->persistent = persistent;
		pool_element->active = TRUE;
		pool_element->activation_counter = 1;
		pool_element->ref_counter = 1;

		if (SUCCESS != zend_hash_add(&(pool->data.master_list), hash_key->c, hash_key->len, &pool_element, sizeof(MYSQLND_MS_POOL_ENTRY *), NULL)) {
			DBG_INF_FMT("Failed adding, duplicate hash key, added twice?");
			ret = FAIL;
		}
		zend_llist_add_element(&(pool->data.active_master_list), &pool_element->data);

		MYSQLND_MS_INC_STATISTIC_W_VALUE(MS_STAT_POOL_MASTERS_TOTAL, zend_hash_num_elements(&(pool->data.master_list)));
		MYSQLND_MS_INC_STATISTIC_W_VALUE(MS_STAT_POOL_MASTERS_ACTIVE, zend_llist_count(&(pool->data.active_master_list)));
	}

	DBG_RETURN(ret);
}
/* }}} */

/* {{{ pool_connection_exists */
static zend_bool
pool_connection_exists(MYSQLND_MS_POOL * pool, smart_str * hash_key,
					   MYSQLND_MS_LIST_DATA ** data,
					   zend_bool * is_master, zend_bool * is_active TSRMLS_DC) {
	zend_bool ret = FALSE;
	MYSQLND_MS_POOL_ENTRY ** pool_element;

	DBG_ENTER("mysqlnd_ms::pool_connection_exists");
	DBG_INF_FMT("pool=%p", pool);

	if (SUCCESS == zend_hash_find(&(pool->data.master_list), hash_key->c, hash_key->len, (void**)&pool_element)) {
		*is_master = TRUE;
		*is_active = (*pool_element)->active;
		*data = (*pool_element)->data;
		ret = TRUE;
		DBG_INF_FMT("element=%p list=%p is_master=%d", *pool_element, &(pool->data.master_list), *is_master);
	} else if (SUCCESS == zend_hash_find(&(pool->data.slave_list), hash_key->c, hash_key->len, (void**)&pool_element)) {
		*is_master = FALSE;
		*is_active = (*pool_element)->active;
		*data = (*pool_element)->data;
		DBG_INF_FMT("element=%p list=%p is_master=%d", *pool_element, &(pool->data.slave_list), *is_master);
		ret = TRUE;
	}

	DBG_RETURN(ret);
}
/* }}} */

/* {{{ pool_connection_reactivate */
static enum_func_status
pool_connection_reactivate(MYSQLND_MS_POOL * pool, smart_str * hash_key, zend_bool is_master TSRMLS_DC) {
	enum_func_status ret = FAIL;
	MYSQLND_MS_POOL_ENTRY ** pool_element;
	enum_mysqlnd_ms_collected_stats stat;
	HashTable * ht;
	zend_llist * list;

	DBG_ENTER("mysqlnd_ms::pool_connection_reactivate");
	DBG_INF_FMT("pool=%p", pool);

	ht = (is_master) ? &(pool->data.master_list) : &(pool->data.slave_list);
	list = (is_master) ? &(pool->data.active_master_list) : &(pool->data.active_slave_list);
	stat = (is_master) ? MS_STAT_POOL_MASTERS_ACTIVE : MS_STAT_POOL_SLAVES_ACTIVE;

	if (SUCCESS == (ret = zend_hash_find(ht, hash_key->c, hash_key->len, (void**)&pool_element))) {

		(*pool_element)->active = TRUE;
		(*pool_element)->activation_counter++;

		zend_llist_add_element(list, &(*pool_element)->data);
		DBG_INF_FMT("reactivating element=%p, list=%p, list_count=%d, is_master=%d", *pool_element, list, zend_llist_count(list), is_master);
		MYSQLND_MS_INC_STATISTIC_W_VALUE(stat, zend_llist_count(list));

	} else {
		DBG_INF_FMT("not found");
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ pool_register_replace_listener */
static enum_func_status
pool_register_replace_listener(MYSQLND_MS_POOL * pool, func_pool_replace_listener listener, void * data TSRMLS_DC) {
	enum_func_status ret = PASS;
	MYSQLND_MS_POOL_LISTENER entry;

	entry.listener = listener;
	entry.data = data;

	DBG_ENTER("mysqlnd_ms::pool_register_replace_listener");
	zend_llist_add_element(&(pool->data.replace_listener), &entry);

	DBG_RETURN(ret);
}
/* }}} */

/* {{{ pool_notify_replace_listener */
static enum_func_status
pool_notify_replace_listener(MYSQLND_MS_POOL * pool TSRMLS_DC) {
	enum_func_status ret = PASS;
	MYSQLND_MS_POOL_LISTENER * entry_p;
	zend_llist_position	pos;

	DBG_ENTER("mysqlnd_ms::pool_notify_replace_listener");

	for (entry_p = (MYSQLND_MS_POOL_LISTENER *) zend_llist_get_first_ex(&(pool->data.replace_listener), &pos);
			entry_p;
			entry_p = (MYSQLND_MS_POOL_LISTENER *) zend_llist_get_next_ex(&(pool->data.replace_listener), &pos)) {
		entry_p->listener(pool, entry_p->data TSRMLS_CC);
	}

	DBG_RETURN(ret);
}
/* }}} */

static int
pool_add_reference_ht_apply_func(void *pDest, void * argument TSRMLS_DC)
{
	int ret = ZEND_HASH_APPLY_REMOVE;
	MYSQLND_MS_POOL_ENTRY * pool_element = (MYSQLND_MS_POOL_ENTRY *)pDest;
	unsigned int * referenced = (unsigned int *)argument;

	if (pool_element->ref_counter > 1) {
		*referenced++;
		ret = ZEND_HASH_APPLY_KEEP;
	} else {
		pool_element->active = FALSE;
	}
	return ret;
}

/* {{{ pool_add_reference */
static enum_func_status
pool_add_reference(MYSQLND_MS_POOL * pool, MYSQLND_CONN_DATA * const conn TSRMLS_DC)
{
	enum_func_status ret = FAIL;
	MYSQLND_MS_POOL_ENTRY ** pool_element;
	HashTable * ht[2];
	unsigned int i;
	DBG_ENTER("mysqlnd_ms::pool_add_reference");

	ht[0] = &(pool->data.master_list);
	ht[1] = &(pool->data.slave_list);

	for (i = 0; i < 2; i++) {
		for (zend_hash_internal_pointer_reset(ht[i]);
			 (zend_hash_has_more_elements(ht[i]) == SUCCESS) && (zend_hash_get_current_data(ht[i], (void**)&pool_element) == SUCCESS);
			zend_hash_move_forward(ht[i])) {
			if ((*pool_element)->data->conn == conn) {
				(*pool_element)->ref_counter++;
				DBG_INF_FMT("%s ref_counter=%d", (0 == i) ? "master" : "slave", (*pool_element)->ref_counter);
				ret = SUCCESS;
				DBG_RETURN(ret);
			}
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ pool_free_reference */
static enum_func_status
pool_free_reference(MYSQLND_MS_POOL * pool, MYSQLND_CONN_DATA * const conn TSRMLS_DC)
{
	enum_func_status ret = FAIL;
	MYSQLND_MS_POOL_ENTRY ** pool_element;
	HashTable * ht[2];
	unsigned int i;
	DBG_ENTER("mysqlnd_ms::pool_free_reference");

	if (!conn || !pool) {
		DBG_INF("No connection or pool given!");
		DBG_RETURN(ret);
	}

	ht[0] = &(pool->data.master_list);
	ht[1] = &(pool->data.slave_list);

	for (i = 0; i < 2; i++) {
		for (zend_hash_internal_pointer_reset(ht[i]);
			 (zend_hash_has_more_elements(ht[i]) == SUCCESS) && (zend_hash_get_current_data(ht[i], (void**)&pool_element) == SUCCESS);
			(zend_hash_move_forward(ht[i]) == SUCCESS)) {
			ret = SUCCESS;
			if ((*pool_element)->ref_counter > 1) {
				(*pool_element)->ref_counter--;
				DBG_INF_FMT("%s ref_counter=%d", (0 == i) ? "master" : "slave", (*pool_element)->ref_counter);
			} else {
				DBG_INF_FMT("%s double free", (0 == i) ? "master" : "slave");
			}
			DBG_RETURN(ret);
		}
	}
	DBG_RETURN(ret);
}
/* }}} */


/* private */
static enum_func_status
pool_dispatch_cmd(MYSQLND_MS_POOL * pool, enum_mysqlnd_pool_cmd cmd, void ** cmd_data, size_t cmd_data_size TSRMLS_DC) {
	enum_func_status ret = PASS;
	MYSQLND_MS_POOL_CMD * pool_cmd, ** pool_cmd_pp;
	DBG_ENTER("mysqlnd_ms::pool_dispatch_cmd");

	*cmd_data = NULL;
	if (zend_hash_index_find(&(pool->data.buffered_cmds), cmd, (void **)&pool_cmd_pp) == SUCCESS) {
		cmd_data = (*pool_cmd_pp)->data;
	} else {
		pool_cmd = mnd_pecalloc(1, sizeof(MYSQLND_MS_POOL_CMD), pool->data.persistent);
		if (!pool_cmd) {
			MYSQLND_MS_WARN_OOM();
			ret = FAIL;
			DBG_RETURN(ret);
		}

		*cmd_data = mnd_pecalloc(1, cmd_data_size, pool->data.persistent);
		if (!*cmd_data) {
			MYSQLND_MS_WARN_OOM();
			ret = FAIL;
			mnd_pefree(pool_cmd, pool->data.persistent);
			DBG_RETURN(ret);
		}

		pool_cmd->cmd = cmd;
		pool_cmd->data = *cmd_data;
		pool_cmd->persistent = pool->data.persistent;
		DBG_INF_FMT("pool_cmd=%p pool_cmd.data=%p", pool_cmd, pool_cmd->data);

		if (zend_hash_index_update(&(pool->data.buffered_cmds), cmd, (void *)&pool_cmd, sizeof(MYSQLND_MS_POOL_CMD *), NULL) != SUCCESS) {
			DBG_INF("update failed\n");
			ret = FAIL;
			mnd_pefree(pool_cmd, pool->data.persistent);
			mnd_pefree(*cmd_data, pool->data.persistent);
			DBG_RETURN(ret);
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ pool_dispatch_select_db */
static enum_func_status
pool_dispatch_select_db(MYSQLND_MS_POOL * pool, func_mysqlnd_conn_data__select_db cb,
							const char * db, unsigned int db_len TSRMLS_DC) {
	enum_func_status ret = PASS;
	enum_mysqlnd_pool_cmd cmd = SELECT_DB;
	MYSQLND_MS_POOL_CMD_SELECT_DB  * cmd_data;

	DBG_ENTER("mysqlnd_ms::pool_dispatch_select_db");
	if (pool->data.in_buffered_replay) {
		DBG_RETURN(ret)
	}

	ret = pool_dispatch_cmd(pool, SELECT_DB, (void *)&cmd_data, sizeof(MYSQLND_MS_POOL_CMD_SELECT_DB) TSRMLS_CC);
	if ((SUCCESS == ret) && cmd_data) {
		if (cmd_data->db) {
			DBG_INF_FMT("free db=%s\n", cmd_data->db);
			mnf_pefree(cmd_data->db, pool->data.persistent);
		}
		cmd_data->cb = cb;
		cmd_data->db = (char *)mnd_pestrndup(db, db_len, pool->data.persistent);
		cmd_data->db_len = db_len;
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ pool_dispatch_set_charset */
static enum_func_status
pool_dispatch_set_charset(MYSQLND_MS_POOL * pool, func_mysqlnd_conn_data__set_charset cb,
							const char * const csname TSRMLS_DC) {
	enum_func_status ret = PASS;
	enum_mysqlnd_pool_cmd cmd = SET_CHARSET;
	MYSQLND_MS_POOL_CMD_SET_CHARSET  * cmd_data;

	DBG_ENTER("mysqlnd_ms::pool_dispatch_set_charset");
	if (pool->data.in_buffered_replay) {
		DBG_RETURN(ret)
	}

	ret = pool_dispatch_cmd(pool, SELECT_DB, (void *)&cmd_data, sizeof(MYSQLND_MS_POOL_CMD_SET_CHARSET) TSRMLS_CC);
	if ((SUCCESS == ret) && cmd_data) {
		if (cmd_data->csname) {
			DBG_INF_FMT("free csname=%s\n", cmd_data->csname);
			mnf_pefree(cmd_data->csname, pool->data.persistent);
		}
		cmd_data->cb = cb;
		cmd_data->csname = (char *)mnd_pestrndup(csname, strlen(csname), pool->data.persistent);
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ pool_dispatch_set_server_option */
static enum_func_status
pool_dispatch_set_server_option(MYSQLND_MS_POOL * pool, func_mysqlnd_conn_data__set_server_option cb,
								enum_mysqlnd_server_option option TSRMLS_DC) {
	enum_func_status ret = PASS;
	enum_mysqlnd_pool_cmd cmd = SET_SERVER_OPTION;
	MYSQLND_MS_POOL_CMD_SET_SERVER_OPTION  * cmd_data;

	DBG_ENTER("mysqlnd_ms::pool_dispatch_set_server_option");
	if (pool->data.in_buffered_replay) {
		DBG_RETURN(ret)
	}

	ret = pool_dispatch_cmd(pool, SET_SERVER_OPTION, (void *)&cmd_data, sizeof(MYSQLND_MS_POOL_CMD_SET_SERVER_OPTION) TSRMLS_CC);
	if ((PASS == ret) && cmd_data) {
		cmd_data->cb = cb;
		cmd_data->option = option;
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ pool_dispatch_set_client_option */
static enum_func_status
pool_dispatch_set_client_option(MYSQLND_MS_POOL * pool, func_mysqlnd_conn_data__set_client_option cb,
								enum_mysqlnd_option option,	const char * const value TSRMLS_DC) {
	enum_func_status ret = PASS;
	enum_mysqlnd_pool_cmd cmd = SET_CLIENT_OPTION;
	MYSQLND_MS_POOL_CMD_SET_CLIENT_OPTION  * cmd_data;

	DBG_ENTER("mysqlnd_ms::pool_dispatch_set_client_option");
	if (pool->data.in_buffered_replay) {
		DBG_RETURN(ret)
	}

	ret = pool_dispatch_cmd(pool, SET_CLIENT_OPTION, (void *)&cmd_data, sizeof(MYSQLND_MS_POOL_CMD_SET_CLIENT_OPTION) TSRMLS_CC);
	if ((PASS == ret) && cmd_data) {
		if (cmd_data->value) {
			DBG_INF_FMT("free csname=%s\n", cmd_data->value);
			mnf_pefree(cmd_data->value, pool->data.persistent);
		}
		cmd_data->cb = cb;
		cmd_data->option = option;
		cmd_data->value = (char *)mnd_pestrndup(value, strlen(value), pool->data.persistent);
	}

	DBG_RETURN(ret);
}
/* }}} */

/* {{{ pool_replay_cmds */
static enum_func_status
pool_replay_cmds(MYSQLND_MS_POOL * pool, MYSQLND_CONN_DATA * const proxy_conn, MYSQLND_MS_CONN_DATA ** proxy_conn_data TSRMLS_DC)
{
	enum_func_status ret = PASS;
	MYSQLND_MS_POOL_CMD ** pool_cmd;

	DBG_ENTER("mysqlnd_ms::pool_replay_cmds");
	DBG_INF_FMT("pool=%p", pool);
	pool->data.in_buffered_replay = TRUE;

	for (zend_hash_internal_pointer_reset(&(pool->data.buffered_cmds));
		 (zend_hash_has_more_elements(&(pool->data.buffered_cmds)) == SUCCESS) && (zend_hash_get_current_data(&(pool->data.buffered_cmds), (void **)&pool_cmd) == SUCCESS);
		 zend_hash_move_forward(&(pool->data.buffered_cmds))) {
		DBG_INF_FMT("pool_cmd=%p pool_cmd.data=%p", *pool_cmd, (*pool_cmd)->data);

		switch ((*pool_cmd)->cmd) {
			case SELECT_DB:
				{
					MYSQLND_MS_POOL_CMD_SELECT_DB * args = (MYSQLND_MS_POOL_CMD_SELECT_DB *)(*pool_cmd)->data;
					ret = args->cb(proxy_conn, args->db, args->db_len TSRMLS_CC);
				}
				break;

			case SET_CHARSET:
				{
					MYSQLND_MS_POOL_CMD_SET_CHARSET * args = (MYSQLND_MS_POOL_CMD_SET_CHARSET *)(*pool_cmd)->data;
					ret = args->cb(proxy_conn, args->csname TSRMLS_CC);
				}
				break;

			case SET_SERVER_OPTION:
				{
					MYSQLND_MS_POOL_CMD_SET_SERVER_OPTION * args = (MYSQLND_MS_POOL_CMD_SET_SERVER_OPTION *)(*pool_cmd)->data;
					ret = args->cb(proxy_conn, args->option TSRMLS_CC);
				}
				break;

			case SET_CLIENT_OPTION:
				{
					MYSQLND_MS_POOL_CMD_SET_CLIENT_OPTION * args = (MYSQLND_MS_POOL_CMD_SET_CLIENT_OPTION *)(*pool_cmd)->data;
					ret = args->cb(proxy_conn, args->option, args->value TSRMLS_CC);
				}
				break;

			default:
				DBG_INF("Unknown command");
				break;
		}

	}
	pool->data.in_buffered_replay = FALSE;

	DBG_RETURN(ret)
}
/* }}} */


/* {{{ pool_get_active_masters */
static zend_llist *
pool_get_active_masters(MYSQLND_MS_POOL * pool TSRMLS_DC)
{
	zend_llist * ret = &((pool->data).active_master_list);
	return ret;
}
/* }}} */


/* {{{ pool_get_active_slaves */
static zend_llist *
pool_get_active_slaves(MYSQLND_MS_POOL * pool TSRMLS_DC)
{
	zend_llist * ret = &((pool->data).active_slave_list);
	return ret;
}
/* }}} */

static void
pool_dtor(MYSQLND_MS_POOL * pool TSRMLS_DC) {
	DBG_ENTER("mysqlnd_ms::pool_dtor");

	if (pool) {
		zend_llist_clean(&(pool->data.replace_listener));

		zend_llist_clean(&(pool->data.active_master_list));
		zend_llist_clean(&(pool->data.active_slave_list));

		zend_hash_destroy(&(pool->data.master_list));
		zend_hash_destroy(&(pool->data.slave_list));

		zend_hash_destroy(&(pool->data.buffered_cmds));

		mnd_pefree(pool, pool->data.persistent);
	}
	DBG_VOID_RETURN;
}


MYSQLND_MS_POOL * mysqlnd_ms_pool_ctor(llist_dtor_func_t ms_list_data_dtor, zend_bool persistent TSRMLS_DC) {
	DBG_ENTER("mysqlnd_ms::pool_ctor");

	MYSQLND_MS_POOL * pool = mnd_pecalloc(1, sizeof(MYSQLND_MS_POOL), persistent);
	if (pool) {

		/* methods */
		pool->flush_active = pool_flush_active;
		pool->clear_all = pool_clear_all;

		pool->init_pool_hash_key = pool_init_pool_hash_key;
		pool->get_conn_hash_key = pool_get_conn_hash_key;
		pool->add_slave = pool_add_slave;
		pool->add_master = pool_add_master;
		pool->connection_exists = pool_connection_exists;
		pool->connection_reactivate = pool_connection_reactivate;

		pool->register_replace_listener = pool_register_replace_listener;
		pool->notify_replace_listener = pool_notify_replace_listener;

		pool->get_active_masters = pool_get_active_masters;
		pool->get_active_slaves = pool_get_active_slaves;

		pool->add_reference = pool_add_reference;
		pool->free_reference = pool_free_reference;

		pool->dispatch_select_db = pool_dispatch_select_db;
		pool->dispatch_set_charset = pool_dispatch_set_charset;
		pool->dispatch_set_server_option = pool_dispatch_set_server_option;
		pool->dispatch_set_client_option = pool_dispatch_set_client_option;
		pool->replay_cmds = pool_replay_cmds;

		pool->dtor = pool_dtor;

		/* data */
		pool->data.persistent = persistent;

		zend_llist_init(&(pool->data.replace_listener), sizeof(MYSQLND_MS_POOL_LISTENER), NULL, persistent);

		/* Our own dtor's will call the MS provided dtor on demand */
		pool->data.ms_list_data_dtor = ms_list_data_dtor;

		zend_hash_init(&(pool->data.master_list), 4, NULL, mysqlnd_ms_pool_all_list_dtor, persistent);
		zend_hash_init(&(pool->data.slave_list), 4, NULL, mysqlnd_ms_pool_all_list_dtor, persistent);

		zend_llist_init(&(pool->data.active_master_list), sizeof(MYSQLND_MS_LIST_DATA *), NULL, persistent);
		zend_llist_init(&(pool->data.active_slave_list), sizeof(MYSQLND_MS_LIST_DATA *), NULL, persistent);

		zend_hash_init(&(pool->data.buffered_cmds), 4, NULL, mysqlnd_ms_pool_cmd_dtor, persistent);
	}

	DBG_RETURN(pool);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
