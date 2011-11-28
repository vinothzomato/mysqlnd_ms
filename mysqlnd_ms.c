/*s
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

#include "mysqlnd_ms_enum_n_def.h"
#include "mysqlnd_ms_switch.h"

#if MYSQLND_VERSION_ID >= 50010
#define MS_LOAD_AND_COPY_CONN_HANDLE_METHODS(orig_methods, ms_methods) \
	(orig_methods) = mysqlnd_conn_get_methods(); \
	memcpy(&(ms_methods), (orig_methods), sizeof(struct st_mysqlnd_conn_methods));

#define MS_SET_CONN_HANDLE_METHODS(ms_methods) mysqlnd_conn_set_methods((ms_methods));

#define MS_LOAD_AND_COPY_CONN_DATA_METHODS(orig_methods, ms_methods) \
	(orig_methods) = mysqlnd_conn_data_get_methods(); \
	memcpy(&(ms_methods), (orig_methods), sizeof(struct st_mysqlnd_conn_data_methods));

#define MS_SET_CONN_DATA_METHODS(ms_methods) mysqlnd_conn_data_set_methods((ms_methods));

#define MS_GET_CONN_DATA_FROM_CONN(conn) (conn)->data

struct st_mysqlnd_conn_data_methods * ms_orig_mysqlnd_conn_methods;
static struct st_mysqlnd_conn_methods my_mysqlnd_conn_handle_methods;

struct st_mysqlnd_conn_methods * ms_orig_mysqlnd_conn_handle_methods;
static struct st_mysqlnd_conn_data_methods my_mysqlnd_conn_methods;

#else

#define MS_LOAD_AND_COPY_CONN_HANDLE_METHODS(orig_methods, ms_methods)
#define MS_SET_CONN_HANDLE_METHODS(ms_methods)

#define MS_LOAD_AND_COPY_CONN_DATA_METHODS(orig_methods, ms_methods) \
	(orig_methods) = mysqlnd_conn_get_methods(); \
	memcpy(&(ms_methods), (orig_methods), sizeof(struct st_mysqlnd_conn_methods));

#define MS_SET_CONN_DATA_METHODS(ms_methods) mysqlnd_conn_set_methods((ms_methods));

#define MS_GET_CONN_DATA_FROM_CONN(conn) (conn)

struct st_mysqlnd_conn_methods * ms_orig_mysqlnd_conn_methods;
static struct st_mysqlnd_conn_methods my_mysqlnd_conn_methods;
#endif


struct st_mysqlnd_stmt_methods * ms_orig_mysqlnd_stmt_methods;
static struct st_mysqlnd_stmt_methods my_mysqlnd_stmt_methods;

static void mysqlnd_ms_conn_free_plugin_data(MYSQLND_CONN_DATA * conn TSRMLS_DC);

MYSQLND_STATS * mysqlnd_ms_stats = NULL;


#define CONN_DATA_NOT_SET(conn_data) (!(conn_data) || !*(conn_data) || !(*(conn_data))->initialized || (*(conn_data))->skip_ms_calls)
#define CONN_DATA_TRX_SET(conn_data) ((conn_data) && (*(conn_data)) && (!(*(conn_data))->skip_ms_calls))
#define CONN_DATA_TRY_TRX_INJECTION(conn_data) (((*(conn_data))->connection_opened) && ((FALSE == (*(conn_data))->skip_ms_calls)) && ((*(conn_data))->global_trx.on_commit) && (TRUE == (*(conn_data))->global_trx.is_master))

#define MS_TRX_INJECT(ret, connection, conn_data) \
	if (PASS == (ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(send_query)((connection), ((*(conn_data))->global_trx.on_commit), ((*(conn_data))->global_trx.on_commit_len) TSRMLS_CC))) \
		(ret) = MS_CALL_ORIGINAL_CONN_DATA_METHOD(reap_query)((connection) TSRMLS_CC);

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
	if (el->host && !strcmp(".", el->host)) {
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
	MYSQLND_MS_LIST_DATA * element = pDest? *(MYSQLND_MS_LIST_DATA **) pDest : NULL;
	TSRMLS_FETCH();
	DBG_ENTER("mysqlnd_ms_conn_list_dtor");
	DBG_INF_FMT("conn=%p", element->conn);
	if (!element) {
		DBG_VOID_RETURN;
	}
	if (element->name_from_config) {
		mnd_pefree(element->name_from_config, element->persistent);
		element->name_from_config = NULL;
	}
	if (element->conn) {
		element->conn->m->free_reference(element->conn TSRMLS_CC);
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
		element->emulated_scheme = NULL;
	}

	mnd_pefree(element, element->persistent);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_commands_list_dtor */
void
mysqlnd_ms_commands_list_dtor(void * pDest)
{
	MYSQLND_MS_COMMAND * element = (MYSQLND_MS_COMMAND *) pDest;
	TSRMLS_FETCH();
	DBG_ENTER("mysqlnd_ms_commands_list_dtor");
	if (element) {
		if (element->payload) {
			mnd_pefree(element->payload, element->persistent);
			element->payload = NULL;
		}
		element->payload_len = 0;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_lazy_connect */
enum_func_status
mysqlnd_ms_lazy_connect(MYSQLND_MS_LIST_DATA * element, zend_bool master TSRMLS_DC)
{
	enum_func_status ret;
	MYSQLND_CONN_DATA * connection = element->conn;
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, connection);

	DBG_ENTER("mysqlnd_ms_advanced_connect");
	ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(connect)(connection, element->host, element->user,
												   element->passwd, element->passwd_len,
												   element->db, element->db_len,
												   element->port, element->socket, element->connect_flags TSRMLS_CC);
	if (PASS == ret) {
		DBG_INF("Connected");
		MYSQLND_MS_INC_STATISTIC(master? MS_STAT_LAZY_CONN_MASTER_SUCCESS:MS_STAT_LAZY_CONN_SLAVE_SUCCESS);
#ifndef MYSQLND_HAS_INJECTION_FEATURE
		/* TODO: without this the global trx id injection logic will fail on recently opened lazy connections */
		if (conn_data && *conn_data) {
			(*conn_data)->initialized = TRUE;
			(*conn_data)->connection_opened = TRUE;
		}
#endif
#ifdef BUFFERED_COMMANDS
		/* let's run the buffered commands */
		{
			zend_llist_position	pos;
			MYSQLND_MS_COMMAND * cmd;
			/* search the list of easy handles hanging off the multi-handle */
			for (cmd = (MYSQLND_MS_COMMAND *) zend_llist_get_first_ex(&(*conn_data)->delayed_commands, &pos);
				 cmd;
				 cmd = (MYSQLND_MS_COMMAND *) zend_llist_get_next_ex(&(*conn_data)->delayed_commands, &pos))
			{
				(void) MS_CALL_ORIGINAL_CONN_DATA_METHOD(simple_command)(connection, cmd->command, cmd->payload,
																	cmd->payload_len, cmd->ok_packet, cmd->silent,
																	cmd->ignore_upsert_status TSRMLS_CC);
			}
		}
#endif
	} else {
		MYSQLND_MS_INC_STATISTIC(master? MS_STAT_LAZY_CONN_MASTER_FAILURE:MS_STAT_LAZY_CONN_SLAVE_FAILURE);
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_connect_init_global_trx */
static void
mysqlnd_ms_init_connection_global_trx(struct st_mysqlnd_ms_global_trx_injection * new_global_trx,
									  struct st_mysqlnd_ms_global_trx_injection * orig_global_trx,
									  zend_bool is_master, zend_bool persistent TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms_init_connection_global_trx");

	if (TRUE == is_master) {
		new_global_trx->on_commit_len = orig_global_trx->on_commit_len;
		new_global_trx->on_commit = (orig_global_trx->on_commit) ?
			mnd_pestrndup(orig_global_trx->on_commit, orig_global_trx->on_commit_len, persistent) : NULL;
	} else {
		new_global_trx->on_commit_len = 0;
		new_global_trx->on_commit = NULL;
	}

	new_global_trx->fetch_last_gtid_len = orig_global_trx->fetch_last_gtid_len;
	new_global_trx->fetch_last_gtid = (orig_global_trx->fetch_last_gtid) ?
		mnd_pestrndup(orig_global_trx->fetch_last_gtid, orig_global_trx->fetch_last_gtid_len, persistent) : NULL;

	new_global_trx->check_for_gtid_len = orig_global_trx->check_for_gtid_len;
	new_global_trx->check_for_gtid = (orig_global_trx->check_for_gtid) ?
		mnd_pestrndup(orig_global_trx->check_for_gtid, orig_global_trx->check_for_gtid_len, persistent) : NULL;

	new_global_trx->is_master = is_master;
	new_global_trx->report_error = orig_global_trx->report_error;
	new_global_trx->use_multi_statement = orig_global_trx->use_multi_statement;
	new_global_trx->multi_statement_user_enabled = orig_global_trx->multi_statement_user_enabled;
	new_global_trx->multi_statement_gtx_enabled = orig_global_trx->multi_statement_gtx_enabled;

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_connect_to_host_aux */
static enum_func_status
mysqlnd_ms_connect_to_host_aux(MYSQLND_CONN_DATA * proxy_conn, MYSQLND_CONN_DATA * conn, const char * name_from_config,
							   zend_bool is_master,
							   const char * host, zend_llist * conn_list,
							   struct st_mysqlnd_ms_conn_credentials * cred,
							   struct st_mysqlnd_ms_global_trx_injection * global_trx,
							   zend_bool lazy_connections, zend_bool persistent TSRMLS_DC)
{
	enum_func_status ret;
	DBG_ENTER("mysqlnd_ms_connect_to_host_aux");
	DBG_INF_FMT("conn:%p host:%s port:%d socket:%s", conn, host, cred->port, cred->socket);
	if (lazy_connections) {
		DBG_INF("Lazy connection");
		ret = PASS;
	} else {
		ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(connect)(conn, host, cred->user, cred->passwd, cred->passwd_len, cred->db, cred->db_len,
													cred->port, cred->socket, cred->mysql_flags TSRMLS_CC);

		if (PASS == ret) {
			DBG_INF_FMT("Connection "MYSQLND_LLU_SPEC" established", conn->thread_id);
		}
	}

	if (ret == PASS) {
		MYSQLND_MS_LIST_DATA * new_element = mnd_pecalloc(1, sizeof(MYSQLND_MS_LIST_DATA), persistent);
		new_element->name_from_config = mnd_pestrdup(name_from_config? name_from_config:"", conn->persistent);
#if MYSQLND_VERSION_ID >= 50010
		new_element->conn = conn->m->get_reference(conn TSRMLS_CC);
#else
		new_element->conn = conn;
#endif
		new_element->host = host? mnd_pestrdup(host, persistent) : NULL;
		new_element->persistent = persistent;
		new_element->port = cred->port;

		new_element->user = cred->user? mnd_pestrdup(cred->user, conn->persistent) : NULL;

		new_element->passwd_len = cred->passwd_len;
		new_element->passwd = cred->passwd? mnd_pestrndup(cred->passwd, cred->passwd_len, conn->persistent) : NULL;

		new_element->db_len = cred->db_len;
		new_element->db = cred->db? mnd_pestrndup(cred->db, cred->db_len, conn->persistent) : NULL;

		new_element->connect_flags = cred->mysql_flags;

		new_element->socket = cred->socket? mnd_pestrdup(cred->socket, conn->persistent) : NULL;
		new_element->emulated_scheme_len = mysqlnd_ms_get_scheme_from_list_data(new_element, &new_element->emulated_scheme,
																			   persistent TSRMLS_CC);

		zend_llist_add_element(conn_list, &new_element);

		{
			/* initialize for every connection, even for slaves and secondary masters */
			MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, conn);
			if (proxy_conn != conn) {
				/* otherwise we will overwrite ourselves */
				*conn_data = mnd_pecalloc(1, sizeof(MYSQLND_MS_CONN_DATA), conn->persistent);
			}
			(*conn_data)->skip_ms_calls = FALSE;
			(*conn_data)->proxy_conn = proxy_conn;

			zend_llist_init(&(*conn_data)->delayed_commands, sizeof(MYSQLND_MS_COMMAND),
							(llist_dtor_func_t) mysqlnd_ms_commands_list_dtor, conn->persistent);
#ifndef MYSQLND_HAS_INJECTION_FEATURE
			mysqlnd_ms_init_connection_global_trx(&(*conn_data)->global_trx, global_trx, is_master, conn->persistent TSRMLS_CC);
			(*conn_data)->connection_opened = (lazy_connections) ? FALSE : TRUE;
#endif
		}
	}
	DBG_INF_FMT("ret=%s", ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_connect_to_host */
static enum_func_status
mysqlnd_ms_connect_to_host(MYSQLND_CONN_DATA * proxy_conn, MYSQLND_CONN_DATA * conn,
						   zend_llist * conn_list, zend_bool is_master,
						   struct st_mysqlnd_ms_conn_credentials * master_credentials,
						   struct st_mysqlnd_ms_global_trx_injection * master_global_trx,
						   struct st_mysqlnd_ms_config_json_entry * main_section,
						   const char * const subsection_name, size_t subsection_name_len,
						   zend_bool lazy_connections, zend_bool persistent,
						   zend_bool process_all_list_values,
						   unsigned int success_stat, unsigned int fail_stat,
						   MYSQLND_ERROR_INFO * error_info TSRMLS_DC)
{
	zend_bool value_exists = FALSE, is_list_value = FALSE;
	struct st_mysqlnd_ms_config_json_entry * subsection = NULL, * parent_subsection = NULL;
	zend_bool recursive = FALSE;
	unsigned int i = 0;
	unsigned int failures = 0;
	DBG_ENTER("mysqlnd_ms_connect_to_host");
	DBG_INF_FMT("conn:%p", conn);

	if (TRUE == mysqlnd_ms_config_json_sub_section_exists(main_section, subsection_name, subsection_name_len, 0 TSRMLS_CC)) {
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
		php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, MYSQLND_MS_ERROR_PREFIX " Cannot find %s section in config", subsection_name);
		if (conn) {
			snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Cannot find %s section in config", subsection_name);
			error_buf[sizeof(error_buf) - 1] = '\0';
			SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
		}
	}
	do {
		struct st_mysqlnd_ms_conn_credentials cred = *master_credentials;
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

		port_str = mysqlnd_ms_config_json_string_from_section(subsection, SECT_PORT_NAME, sizeof(SECT_PORT_NAME) - 1, 0,
															  &value_exists, &is_list_value TSRMLS_CC);
		if (value_exists && port_str) {
			int tmp_port = atoi(port_str);
			if (tmp_port < 0 || tmp_port > 65535 || (!tmp_port && flags_str[0] != '0')) {
				char error_buf[128];
				snprintf(error_buf, sizeof(error_buf),
							MYSQLND_MS_ERROR_PREFIX " Invalid value for "SECT_PORT_NAME" '%s' . Stopping", port_str);
				error_buf[sizeof(error_buf) - 1] = '\0';

				php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "%s", error_buf);
				SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
				failures++;
			} else {
				cred.port = (unsigned int) atoi(port_str);
			}
		}

		flags_str = mysqlnd_ms_config_json_string_from_section(subsection, SECT_CONNECT_FLAGS_NAME, sizeof(SECT_CONNECT_FLAGS_NAME)-1, 0,
															   &value_exists, &is_list_value TSRMLS_CC);
		if (value_exists) {
			int tmp_flags = atoi(flags_str);
			if (tmp_flags < 0 || (!tmp_flags && flags_str[0] != '0')) {
				char error_buf[128];
				snprintf(error_buf, sizeof(error_buf),
							MYSQLND_MS_ERROR_PREFIX " Invalid value for "SECT_CONNECT_FLAGS_NAME" '%s' . Stopping", flags_str);
				error_buf[sizeof(error_buf) - 1] = '\0';

				php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "%s", error_buf);
				SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
				failures++;
			} else {
				cred.mysql_flags = (unsigned int) tmp_flags;
			}
		}

		socket_to_use = mysqlnd_ms_config_json_string_from_section(subsection, SECT_SOCKET_NAME, sizeof(SECT_SOCKET_NAME) - 1, 0,
															   &value_exists, &is_list_value TSRMLS_CC);
		if (value_exists) {
			cred.socket = socket_to_use;
		}
		user_to_use = mysqlnd_ms_config_json_string_from_section(subsection, SECT_USER_NAME, sizeof(SECT_USER_NAME) - 1, 0,
																 &value_exists, &is_list_value TSRMLS_CC);
		if (value_exists) {
			cred.user = user_to_use;
		}
		pass_to_use = mysqlnd_ms_config_json_string_from_section(subsection, SECT_PASS_NAME, sizeof(SECT_PASS_NAME) - 1, 0,
																 &value_exists, &is_list_value TSRMLS_CC);
		if (value_exists) {
			cred.passwd = pass_to_use;
			cred.passwd_len = strlen(cred.passwd);
		}

		db_to_use = mysqlnd_ms_config_json_string_from_section(subsection, SECT_DB_NAME, sizeof(SECT_DB_NAME) - 1, 0,
															   &value_exists, &is_list_value TSRMLS_CC);
		if (value_exists) {
			cred.db = db_to_use;
			cred.db_len = strlen(cred.db);
		}

		host = mysqlnd_ms_config_json_string_from_section(subsection, SECT_HOST_NAME, sizeof(SECT_HOST_NAME) - 1, 0,
														  &value_exists, &is_list_value TSRMLS_CC);
		if (FALSE == value_exists) {
			DBG_ERR_FMT("Cannot find ["SECT_HOST_NAME"] in [%s] section in config", subsection_name);
			php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR,
							 MYSQLND_MS_ERROR_PREFIX " Cannot find ["SECT_HOST_NAME"] in [%s] section in config", subsection_name);
			SET_CLIENT_ERROR((*error_info), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE,
							 MYSQLND_MS_ERROR_PREFIX " Cannot find ["SECT_HOST_NAME"] in section in config");
			failures++;
		} else {
#if MYSQLND_VERSION_ID >= 50010
			MYSQLND * tmp_conn_handle = NULL;
			MYSQLND_CONN_DATA * tmp_conn = NULL;
			if (conn && i==0) {
				tmp_conn = conn;
			} else {
				tmp_conn_handle = mysqlnd_init(persistent);
				if (tmp_conn_handle) {
					tmp_conn = MS_GET_CONN_DATA_FROM_CONN(tmp_conn_handle);
				}
			}
#else
			MYSQLND * tmp_conn = (conn && i==0)? conn->m->get_reference(conn TSRMLS_CC) : mysqlnd_init(persistent);
#endif
			if (tmp_conn) {
				enum_func_status status =
					mysqlnd_ms_connect_to_host_aux(proxy_conn, tmp_conn, current_subsection_name, is_master, host, conn_list, &cred,
												   master_global_trx, lazy_connections, persistent TSRMLS_CC);
#if MYSQLND_VERSION_ID >= 50010
				if (tmp_conn_handle) {
					tmp_conn_handle->m->dtor(tmp_conn_handle TSRMLS_CC);
				}
#endif
				if (status != PASS) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Cannot connect to %s", host);
					(*error_info) = MYSQLND_MS_ERROR_INFO(tmp_conn);
					failures++;
					/* let's free the handle, if there is one. The underlying object will stay alive */
					MYSQLND_MS_INC_STATISTIC(fail_stat);
#if MYSQLND_VERSION_ID < 50010
					if (tmp_conn != conn) {
						tmp_conn->m->dtor(tmp_conn TSRMLS_CC);
					} else {
						conn->m->free_reference(conn TSRMLS_CC);
					}
#endif
				} else {
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
	} while (TRUE == process_all_list_values && TRUE == recursive /* && failures == 0 */ );

	DBG_RETURN(failures==0 ? PASS:FAIL);
}
/* }}} */


/* {{{ mysqlnd_ms_init_trx_to_null */
static void
mysqlnd_ms_init_trx_to_null(struct st_mysqlnd_ms_global_trx_injection * trx TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms_init_trx_to_null");

	trx->on_commit = NULL;
	trx->on_commit_len = (size_t)0;
	trx->fetch_last_gtid = NULL;
	trx->fetch_last_gtid_len = (size_t)0;
	trx->check_for_gtid = NULL;
	trx->check_for_gtid_len = (size_t)0;
	trx->is_master = FALSE;
	trx->report_error = TRUE;
	trx->use_multi_statement = FALSE;
	trx->multi_statement_user_enabled = FALSE;
	trx->multi_statement_gtx_enabled = FALSE;

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms_load_trx_config */
static void
mysqlnd_ms_load_trx_config(struct st_mysqlnd_ms_config_json_entry * main_section,
						   struct st_mysqlnd_ms_global_trx_injection * trx,
						   zend_bool persistent TSRMLS_DC)
{
	zend_bool entry_exists;
	zend_bool entry_is_list;
	struct st_mysqlnd_ms_config_json_entry * g_trx_section;
	DBG_ENTER("mysqlnd_ms_load_trx_config");

	g_trx_section =	mysqlnd_ms_config_json_sub_section(main_section, SECT_G_TRX_NAME, sizeof(SECT_G_TRX_NAME) - 1, &entry_exists TSRMLS_CC);

	if (entry_exists && g_trx_section) {
		char * json_value = NULL;
		size_t json_value_len;

		json_value = mysqlnd_ms_config_json_string_from_section(g_trx_section, SECT_G_TRX_ON_COMMIT, sizeof(SECT_G_TRX_ON_COMMIT) - 1, 0, &entry_exists, &entry_is_list TSRMLS_CC);
		if (entry_exists && json_value) {
			if (entry_is_list) {
				/* TODO: bail out */
			} else {
				json_value_len = strlen(json_value);
				trx->on_commit = mnd_pestrndup(json_value, json_value_len, persistent);
				trx->on_commit_len = strlen(json_value);
			}
			mnd_efree(json_value);
		}

		json_value = mysqlnd_ms_config_json_string_from_section(g_trx_section, SECT_G_TRX_FETCH_LAST_GTID, sizeof(SECT_G_TRX_FETCH_LAST_GTID) - 1, 0, &entry_exists, &entry_is_list TSRMLS_CC);
		if (entry_exists && json_value) {
			if (entry_is_list) {
				/* TODO: bail out */
			} else {
				json_value_len = strlen(json_value);
				trx->fetch_last_gtid = mnd_pestrndup(json_value, json_value_len, persistent);
				trx->fetch_last_gtid_len = strlen(json_value);
			}
			mnd_efree(json_value);
		}

		json_value = mysqlnd_ms_config_json_string_from_section(g_trx_section, SECT_G_TRX_CHECK_FOR_GTID, sizeof(SECT_G_TRX_CHECK_FOR_GTID) - 1, 0, &entry_exists, &entry_is_list TSRMLS_CC);
		if (entry_exists && json_value) {
			if (entry_is_list) {
				/* TODO: bail out */
			} else {
				json_value_len = strlen(json_value);
				trx->check_for_gtid = mnd_pestrndup(json_value, json_value_len, persistent);
				trx->check_for_gtid_len = strlen(json_value);
			}
			mnd_efree(json_value);
		}

		json_value = mysqlnd_ms_config_json_string_from_section(g_trx_section, SECT_G_TRX_REPORT_ERROR, sizeof(SECT_G_TRX_REPORT_ERROR) - 1, 0, &entry_exists, &entry_is_list TSRMLS_CC);
		if (entry_exists && json_value) {
			trx->report_error = !mysqlnd_ms_config_json_string_is_bool_false(json_value);
			mnd_efree(json_value);
		}

		json_value = mysqlnd_ms_config_json_string_from_section(g_trx_section, SECT_G_TRX_MULTI_STMT, sizeof(SECT_G_TRX_MULTI_STMT) - 1, 0, &entry_exists, &entry_is_list TSRMLS_CC);
		if (entry_exists && json_value) {
			trx->use_multi_statement = !mysqlnd_ms_config_json_string_is_bool_false(json_value);
			mnd_efree(json_value);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms::connect */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, connect)(MYSQLND_CONN_DATA * conn,
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
	MYSQLND_MS_CONN_DATA ** conn_data;
	size_t host_len = host? strlen(host) : 0;
	zend_bool section_found;
	zend_bool hotloading = MYSLQND_MS_HOTLOADING;

	DBG_ENTER("mysqlnd_ms::connect");
	if (hotloading) {
		MYSQLND_MS_CONFIG_JSON_LOCK(mysqlnd_ms_json_config);
	}
	section_found = mysqlnd_ms_config_json_section_exists(mysqlnd_ms_json_config, host, host_len, 0, hotloading? FALSE:TRUE TSRMLS_CC);
	if (MYSQLND_MS_G(force_config_usage) && FALSE == section_found) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Exclusive usage of configuration enforced but did not find the correct INI file section (%s)", host);
		if (hotloading) {
			MYSQLND_MS_CONFIG_JSON_UNLOCK(mysqlnd_ms_json_config);
		}
		SET_CLIENT_ERROR(MYSQLND_MS_ERROR_INFO(conn), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, MYSQLND_MS_ERROR_PREFIX " Exclusive usage of configuration enforced but did not find the correct INI file section");
		DBG_RETURN(FAIL);
	}
	mysqlnd_ms_conn_free_plugin_data(conn TSRMLS_CC);

	if (FALSE == section_found) {
		DBG_INF("section not found");
		ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(connect)(conn, host, user, passwd, passwd_len, db, db_len, port, socket, mysql_flags TSRMLS_CC);
	} else {
		struct st_mysqlnd_ms_config_json_entry * the_section;
		MS_LOAD_CONN_DATA(conn_data, conn);

		*conn_data = mnd_pecalloc(1, sizeof(MYSQLND_MS_CONN_DATA), conn->persistent);
		zend_llist_init(&(*conn_data)->master_connections, sizeof(MYSQLND_MS_LIST_DATA *), (llist_dtor_func_t) mysqlnd_ms_conn_list_dtor, conn->persistent);
		zend_llist_init(&(*conn_data)->slave_connections, sizeof(MYSQLND_MS_LIST_DATA *), (llist_dtor_func_t) mysqlnd_ms_conn_list_dtor, conn->persistent);

		(*conn_data)->cred.user = user? mnd_pestrdup(user, conn->persistent) : NULL;
		(*conn_data)->cred.passwd_len = passwd_len;
		(*conn_data)->cred.passwd = passwd? mnd_pestrndup(passwd, passwd_len, conn->persistent) : NULL;
		(*conn_data)->cred.db_len = db_len;
		(*conn_data)->cred.db = db? mnd_pestrndup(db, db_len, conn->persistent) : NULL;
		(*conn_data)->cred.port = port;
		(*conn_data)->cred.socket = socket? mnd_pestrdup(socket, conn->persistent) : NULL;
		(*conn_data)->cred.mysql_flags = mysql_flags;
#ifndef MYSQLND_HAS_INJECTION_FEATURE
		mysqlnd_ms_init_trx_to_null(&(*conn_data)->global_trx TSRMLS_CC);
		(*conn_data)->connection_opened = FALSE;
#endif
		(*conn_data)->initialized = TRUE;

		if (!hotloading) {
			MYSQLND_MS_CONFIG_JSON_LOCK(mysqlnd_ms_json_config);
		}

		do {
			zend_bool value_exists = FALSE, use_lazy_connections = TRUE;
			/* create master connection */
			the_section = mysqlnd_ms_config_json_section(mysqlnd_ms_json_config, host, host_len, &value_exists TSRMLS_CC);

			SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(conn));
#ifndef MYSQLND_HAS_INJECTION_FEATURE
			mysqlnd_ms_load_trx_config(the_section, &(*conn_data)->global_trx, conn->persistent TSRMLS_CC);
#endif
			{
				char * lazy_connections = mysqlnd_ms_config_json_string_from_section(the_section, LAZY_NAME, sizeof(LAZY_NAME) - 1, 0,
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
			{
				const char * const secs_to_check[] = {MASTER_NAME, SLAVE_NAME};
				unsigned int i = 0;
				for (; i < sizeof(secs_to_check) / sizeof(secs_to_check[0]); ++i) {
					size_t sec_len = strlen(secs_to_check[i]);
					if (FALSE == mysqlnd_ms_config_json_sub_section_exists(the_section, secs_to_check[i], sec_len, 0 TSRMLS_CC)) {
						char error_buf[128];
						snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Section [%s] doesn't exist for host [%s]", secs_to_check[i], host);
						error_buf[sizeof(error_buf) - 1] = '\0';
						SET_CLIENT_ERROR(MYSQLND_MS_ERROR_INFO(conn), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
						php_error_docref(NULL TSRMLS_CC, E_ERROR, "%s", error_buf);
					}
				}
			}

			DBG_INF("-------------------- MASTER CONNECTIONS ------------------");
			ret = mysqlnd_ms_connect_to_host(conn, conn, &(*conn_data)->master_connections,
											 TRUE, &(*conn_data)->cred,
											 &(*conn_data)->global_trx, the_section,
											 MASTER_NAME, sizeof(MASTER_NAME) - 1,
											 use_lazy_connections,
											 conn->persistent, MYSQLND_MS_G(multi_master) /* multimaster*/,
											 MS_STAT_NON_LAZY_CONN_MASTER_SUCCESS,
											 MS_STAT_NON_LAZY_CONN_MASTER_FAILURE,
											 &MYSQLND_MS_ERROR_INFO(conn) TSRMLS_CC);
			if (FAIL == ret || (MYSQLND_MS_ERROR_INFO(conn).error_no)) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Error while connecting to the master(s)");
				break;
			}

			SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(conn));

			DBG_INF("-------------------- SLAVE CONNECTIONS ------------------");
			ret = mysqlnd_ms_connect_to_host(conn, NULL, &(*conn_data)->slave_connections,
											 FALSE, &(*conn_data)->cred,
											 &(*conn_data)->global_trx, the_section,
											 SLAVE_NAME, sizeof(SLAVE_NAME) - 1,
											 use_lazy_connections,
											 conn->persistent, TRUE /* multi*/,
											 MS_STAT_NON_LAZY_CONN_SLAVE_SUCCESS,
											 MS_STAT_NON_LAZY_CONN_SLAVE_FAILURE,
											 &MYSQLND_MS_ERROR_INFO(conn) TSRMLS_CC);

			if (FAIL == ret || (MYSQLND_MS_ERROR_INFO(conn).error_no)) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Error while connecting to the slaves");
				break;
			}
			DBG_INF_FMT("master_list=%p count=%d", &(*conn_data)->master_connections, zend_llist_count(&(*conn_data)->master_connections));
			DBG_INF_FMT("slave_list=%p count=%d", &(*conn_data)->slave_connections, zend_llist_count(&(*conn_data)->slave_connections));
			(*conn_data)->stgy.filters = mysqlnd_ms_load_section_filters(the_section, &MYSQLND_MS_ERROR_INFO(conn),
																		 TRUE /* load all config persistently */ TSRMLS_CC);
			if (!(*conn_data)->stgy.filters) {
				ret = FAIL;
				break;
			}
			mysqlnd_ms_lb_strategy_setup(&(*conn_data)->stgy, the_section, &MYSQLND_MS_ERROR_INFO(conn) TSRMLS_CC);
		} while (0);
		mysqlnd_ms_config_json_reset_section(the_section, TRUE TSRMLS_CC);

		if (!hotloading) {
			MYSQLND_MS_CONFIG_JSON_UNLOCK(mysqlnd_ms_json_config);
		}

		if (ret == PASS) {
			(*conn_data)->connect_host = host? mnd_pestrdup(host, conn->persistent) : NULL;
		}
	}

	if (hotloading) {
		MYSQLND_MS_CONFIG_JSON_UNLOCK(mysqlnd_ms_json_config);
	}
	DBG_INF_FMT("conn=%llu old_refcount=%u", conn->thread_id, conn->refcount);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_do_send_query(MYSQLND_CONN_DATA * conn, const char * query, unsigned int query_len, zend_bool pick_server TSRMLS_DC) */
static enum_func_status
mysqlnd_ms_do_send_query(MYSQLND_CONN_DATA * conn, const char * query, unsigned int query_len, zend_bool pick_server TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, conn);
	enum_func_status ret = PASS;
	DBG_ENTER("mysqlnd_ms::do_send_query");

	if (CONN_DATA_NOT_SET(conn_data)) {
	} else if (pick_server) {
		DBG_INF("Must be async query, blocking and failing");
		if (conn) {
			char error_buf[128];
			snprintf(error_buf, sizeof(error_buf), MYSQLND_MS_ERROR_PREFIX " Asynchronous queries are not supported");
			error_buf[sizeof(error_buf) - 1] = '\0';
			SET_CLIENT_ERROR(MYSQLND_MS_ERROR_INFO(conn), CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, error_buf);
			php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "%s", error_buf);
			DBG_RETURN(FAIL);
		}
	}

	ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(send_query)(conn, query, query_len TSRMLS_CC);
	DBG_RETURN(ret);
}


/* {{{ MYSQLND_METHOD(mysqlnd_ms, send_query) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, send_query)(MYSQLND_CONN_DATA * conn, const char * query, unsigned int query_len TSRMLS_DC)
{
	return mysqlnd_ms_do_send_query(conn, query, query_len, TRUE TSRMLS_CC);
}
/* }}} */


/* {{{ MYSQLND_METHOD(mysqlnd_ms, query) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, query)(MYSQLND_CONN_DATA * conn, const char * query, unsigned int query_len TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, conn);
	MYSQLND_CONN_DATA * connection;
	enum_func_status ret = FAIL;
	smart_str multi_query = {0};
#ifdef ALL_SERVER_DISPATCH
	zend_bool use_all = 0;
#endif
	DBG_ENTER("mysqlnd_ms::query");
	DBG_INF_FMT("query=%s", query);

	if (CONN_DATA_NOT_SET(conn_data)) {
		ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(query)(conn, query, query_len TSRMLS_CC);
		DBG_RETURN(ret);
	}

	connection = mysqlnd_ms_pick_server_ex(conn, query, query_len TSRMLS_CC);
	DBG_INF_FMT("Connection %p error_no=%d", connection, connection? (MYSQLND_MS_ERROR_INFO(connection).error_no) : -1);
	/*
	  Beware : error_no is set to 0 in original->query. This, this might be a problem,
	  as we dump a connection from usage till the end of the script.
	  Lazy connections can generate connection failures, thus we need to check for them.
	  If we skip these checks we will get 2014 from original->query.
	*/
	if (!connection || (MYSQLND_MS_ERROR_INFO(connection).error_no)) {
		DBG_RETURN(ret);
	}

#ifdef ALL_SERVER_DISPATCH
	if (use_all) {
		MYSQLND_MS_CONN_DATA ** conn_data = (MYSQLND_MS_CONN_DATA **) mysqlnd_plugin_get_plugin_connection_data_data(conn, mysqlnd_ms_plugin_id);
		zend_llist * master_connections = (conn_data && *conn_data)? &(*conn_data)->master_connections : NULL;
		zend_llist * slave_connections = (conn_data && *conn_data)? &(*conn_data)->slave_connections : NULL;

		mysqlnd_ms_query_all(conn, query, query_len, master_connections, slave_connections TSRMLS_CC);
	}
#endif

	DBG_INF_FMT("conn="MYSQLND_LLU_SPEC" query=%s", connection->thread_id, query);

	/* TODO:
	  This is wrong - we should do injection *after* successful query.
	  Doing it here - for now - to avoid trouble with result sets.
	  How expensive is the load?
	*/
	MS_LOAD_CONN_DATA(conn_data, connection);

#ifndef MYSQLND_HAS_INJECTION_FEATURE
	if (CONN_DATA_TRX_SET(conn_data) && CONN_DATA_TRY_TRX_INJECTION(conn_data))
	{
		if (FALSE == (*conn_data)->stgy.in_transaction) {
			/* autocommit mode */

			if (TRUE == (*conn_data)->global_trx.use_multi_statement) {
				smart_str_appendl(&multi_query, (*conn_data)->global_trx.on_commit, (*conn_data)->global_trx.on_commit_len);
				smart_str_appendc(&multi_query, ';');
				smart_str_appendl(&multi_query, query, query_len);
				smart_str_appendc(&multi_query, '\0');
				query = multi_query.c;
				query_len = multi_query.len - 1;

				if (TRUE == (*conn_data)->global_trx.multi_statement_user_enabled) {
					/* user has activated multi statement already */
					(*conn_data)->global_trx.multi_statement_gtx_enabled = FALSE;
					ret = PASS;
				} else {
					/* we need to send server option */
					(*conn_data)->global_trx.multi_statement_gtx_enabled = TRUE;
					ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(set_server_option)(connection, MYSQL_OPTION_MULTI_STATEMENTS_ON TSRMLS_CC);
				}

				if (FAIL == ret) {
					if (TRUE == (*conn_data)->global_trx.report_error) {
						MYSQLND_MS_INC_STATISTIC(MS_STAT_GTID_AUTOCOMMIT_FAILURE);
						if (multi_query.c)
							smart_str_free(&multi_query);
						DBG_RETURN(ret);
					}

					SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(connection));
				}

			} else {

				MS_TRX_INJECT(ret, connection, conn_data);
				MYSQLND_MS_INC_STATISTIC((PASS == ret) ? MS_STAT_GTID_AUTOCOMMIT_SUCCESS :
					MS_STAT_GTID_AUTOCOMMIT_FAILURE);

				if (FAIL == ret) {
					if (TRUE == (*conn_data)->global_trx.report_error) {
						DBG_RETURN(ret);
					}

					SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(connection));
				}
			}
		} else {
			/* autocommit off */

			if ((TRUE == (*conn_data)->global_trx.multi_statement_gtx_enabled) &&
				(FALSE == (*conn_data)->global_trx.multi_statement_user_enabled))
			{
				/* no, the user has not requested multi statement, forbid it... */
				ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(set_server_option)(connection, MYSQL_OPTION_MULTI_STATEMENTS_OFF TSRMLS_CC);

				if (FAIL == ret) {
					if (TRUE == (*conn_data)->global_trx.report_error) {
						MYSQLND_MS_INC_STATISTIC(MS_STAT_GTID_AUTOCOMMIT_FAILURE);
						DBG_RETURN(ret);
					}

					SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(connection));
				}
			}
		}
	}
#endif
	if ((PASS == (ret = mysqlnd_ms_do_send_query(connection, query, query_len, FALSE TSRMLS_CC))) &&
		(PASS == (ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(reap_query)(connection TSRMLS_CC))))
	{
		if (connection->last_query_type == QUERY_UPSERT && (MYSQLND_MS_UPSERT_STATUS(connection).affected_rows)) {
			MYSQLND_INC_CONN_STATISTIC_W_VALUE(connection->stats, STAT_ROWS_AFFECTED_NORMAL, MYSQLND_MS_UPSERT_STATUS(connection).affected_rows);
		}
	}

#ifndef MYSQLND_HAS_INJECTION_FEATURE
	/* TODO: I don't think error forwarding to user will work. we probably need to add checks! */
	if (multi_query.c) {
		if (PASS == ret) {
			/* user query has not failed */
			ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(more_results)(connection TSRMLS_CC);
			if (PASS == ret)
				ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(next_result)(connection TSRMLS_CC);

			MYSQLND_MS_INC_STATISTIC((PASS == ret) ? MS_STAT_GTID_AUTOCOMMIT_SUCCESS :
				MS_STAT_GTID_AUTOCOMMIT_FAILURE);

			if ((FAIL == ret) &&  (FALSE == (*conn_data)->global_trx.report_error))
			{
				ret = PASS;
				SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(connection));
			}
		}
		smart_str_free(&multi_query);
	}
#endif

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::use_result */
static MYSQLND_RES *
MYSQLND_METHOD(mysqlnd_ms, use_result)(MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MYSQLND_RES * result;
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;
	DBG_ENTER("mysqlnd_ms::use_result");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, conn->thread_id);
	result = MS_CALL_ORIGINAL_CONN_DATA_METHOD(use_result)(conn TSRMLS_CC);
	DBG_RETURN(result);
}
/* }}} */


/* {{{ mysqlnd_ms::store_result */
static MYSQLND_RES *
MYSQLND_METHOD(mysqlnd_ms, store_result)(MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MYSQLND_RES * result;
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;
	DBG_ENTER("mysqlnd_ms::store_result");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, conn->thread_id);

	result = MS_CALL_ORIGINAL_CONN_DATA_METHOD(store_result)(conn TSRMLS_CC);
	if (conn_data && *conn_data && (*conn_data)->global_trx.use_multi_statement &&
		(FALSE == (*conn_data)->global_trx.multi_statement_user_enabled))
	{
		/* TODO: What shall happen in case of an error: how to inform the user? */
		enum_func_status ret = FAIL;
		ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(set_server_option)(conn, MYSQL_OPTION_MULTI_STATEMENTS_OFF TSRMLS_CC);
	}

	DBG_RETURN(result);
}
/* }}} */


/* {{{ mysqlnd_ms_conn_free_plugin_data */
static void
mysqlnd_ms_conn_free_plugin_data(MYSQLND_CONN_DATA * conn TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(data_pp, conn);
	DBG_ENTER("mysqlnd_ms_conn_free_plugin_data");

	DBG_INF_FMT("data_pp=%p", data_pp);
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
#ifndef MYSQLND_HAS_INJECTION_FEATURE
		if ((*data_pp)->global_trx.on_commit) {
			mnd_pefree((*data_pp)->global_trx.on_commit, conn->persistent);
			(*data_pp)->global_trx.on_commit = NULL;
			(*data_pp)->global_trx.on_commit_len = 0;
		}
		if ((*data_pp)->global_trx.fetch_last_gtid) {
			mnd_pefree((*data_pp)->global_trx.fetch_last_gtid, conn->persistent);
			(*data_pp)->global_trx.fetch_last_gtid = NULL;
			(*data_pp)->global_trx.fetch_last_gtid_len = (size_t)0;
		}
		if ((*data_pp)->global_trx.check_for_gtid) {
			mnd_pefree((*data_pp)->global_trx.check_for_gtid, conn->persistent);
			(*data_pp)->global_trx.check_for_gtid = NULL;
			(*data_pp)->global_trx.check_for_gtid_len = (size_t)0;
		}
#endif
		DBG_INF_FMT("cleaning the llists");
		zend_llist_clean(&(*data_pp)->master_connections);
		zend_llist_clean(&(*data_pp)->slave_connections);
		zend_llist_clean(&(*data_pp)->delayed_commands);

		DBG_INF_FMT("cleaning the section filters");
		if ((*data_pp)->stgy.filters) {
			DBG_INF_FMT("%d loaded filters", zend_llist_count((*data_pp)->stgy.filters));
			zend_llist_clean((*data_pp)->stgy.filters);
			mnd_pefree((*data_pp)->stgy.filters, TRUE /* all filters were loaded persistently */);
			(*data_pp)->stgy.filters = NULL;
		}

		mnd_pefree(*data_pp, conn->persistent);
		*data_pp = NULL;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms::dtor */
static void
MYSQLND_METHOD_PRIVATE(mysqlnd_ms, dtor)(MYSQLND_CONN_DATA * conn TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_ms::dtor");

	mysqlnd_ms_conn_free_plugin_data(conn TSRMLS_CC);
	MS_CALL_ORIGINAL_CONN_DATA_METHOD(dtor)(conn TSRMLS_CC);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_ms::escape_string */
static ulong
MYSQLND_METHOD(mysqlnd_ms, escape_string)(MYSQLND_CONN_DATA * const proxy_conn, char * newstr, const char * escapestr, size_t escapestr_len TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;
	ulong ret = 0;

	DBG_ENTER("mysqlnd_ms::escape_string");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, conn->thread_id);
	if (CONN_GET_STATE(conn) > CONN_ALLOCED && CONN_GET_STATE(conn) != CONN_QUIT_SENT) {
		if (conn_data && *conn_data) {
			(*conn_data)->skip_ms_calls = TRUE;
		}
		ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(escape_string)(conn, newstr, escapestr, escapestr_len TSRMLS_CC);
		if (conn_data && *conn_data) {
			(*conn_data)->skip_ms_calls = FALSE;
		}
	} else {
		newstr[0] = '\0';
		php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " string escaping doesn't work without established connection");
		SET_CLIENT_ERROR(MYSQLND_MS_ERROR_INFO(conn), CR_COMMANDS_OUT_OF_SYNC, UNKNOWN_SQLSTATE, mysqlnd_out_of_sync);
		DBG_ERR_FMT("Command out of sync. State=%u", CONN_GET_STATE(conn));
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::change_user */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, change_user)(MYSQLND_CONN_DATA * const proxy_conn,
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
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);

	DBG_ENTER("mysqlnd_ms::change_user");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, proxy_conn->thread_id);
	if (CONN_DATA_NOT_SET(conn_data)) {
#if PHP_VERSION_ID >= 50399
		DBG_RETURN(MS_CALL_ORIGINAL_CONN_DATA_METHOD(change_user)(proxy_conn, user, passwd, db, silent, passwd_len TSRMLS_CC));
#else
		DBG_RETURN(MS_CALL_ORIGINAL_CONN_DATA_METHOD(change_user)(proxy_conn, user, passwd, db, silent TSRMLS_CC));
#endif
	} else {
		MYSQLND_MS_LIST_DATA * el;
		BEGIN_ITERATE_OVER_SERVER_LISTS(el, &(*conn_data)->master_connections, &(*conn_data)->slave_connections);
		{
			MS_DECLARE_AND_LOAD_CONN_DATA(el_conn_data, el->conn);
			if (el_conn_data && *el_conn_data) {
				(*el_conn_data)->skip_ms_calls = TRUE;
			}
			if (CONN_GET_STATE(el->conn) == CONN_ALLOCED) {
				/* lazy connection */
				if (el->user) {
					mnd_pefree(el->user, el->persistent);
				}
				el->user = user? mnd_pestrdup(user, el->persistent) : NULL;

				if (el->passwd) {
					mnd_pefree(el->passwd, el->persistent);
				}
#if PHP_VERSION_ID >= 50399
				el->passwd_len = passwd_len;
#else
				el->passwd_len = strlen(passwd);
#endif
				el->passwd = passwd? mnd_pestrndup(passwd, el->passwd_len, el->persistent) : NULL;
				if (el->db) {
					mnd_pefree(el->db, el->persistent);
				}
				el->db_len = strlen(db);
				el->db = db? mnd_pestrndup(db, el->db_len, el->persistent) : NULL;
			} else {
				if (PASS != MS_CALL_ORIGINAL_CONN_DATA_METHOD(change_user)(el->conn, user, passwd, db, silent
#if PHP_VERSION_ID >= 50399
																	,passwd_len
#endif
																	TSRMLS_CC))
				{
					ret = FAIL;
				}
			}
			if (el_conn_data && *el_conn_data) {
				(*el_conn_data)->skip_ms_calls = FALSE;
			}
		}
		END_ITERATE_OVER_SERVER_LISTS;
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::ping */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, ping)(MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	enum_func_status ret;
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;
	DBG_ENTER("mysqlnd_ms::ping");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, conn->thread_id);
	ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(ping)(conn TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::kill */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, kill)(MYSQLND_CONN_DATA * proxy_conn, unsigned int pid TSRMLS_DC)
{
	enum_func_status ret;
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;
	DBG_ENTER("mysqlnd_ms::kill");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, conn->thread_id);
	ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(kill_connection)(conn, pid TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


#if 0
/* {{{ mysqlnd_ms::get_errors */
static zval *
MYSQLND_METHOD(mysqlnd_ms, get_errors)(MYSQLND_CONN_DATA * const proxy_conn, const char * const db, unsigned int db_len TSRMLS_DC)
{
	zval * ret = NULL;
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);

	DBG_ENTER("mysqlnd_ms::get_errors");
	if (conn_data && *conn_data) {
		MYSQLND_MS_LIST_DATA * el;
		array_init(ret);
		BEGIN_ITERATE_OVER_SERVER_LISTS(el, &(*conn_data)->master_connections, &(*conn_data)->slave_connections);
		{
			MS_DECLARE_AND_LOAD_CONN_DATA(el_conn_data, el->conn);
			zval * row = NULL;
			char * scheme;
			size_t scheme_len;

			if (el_conn_data && *el_conn_data) {
				(*el_conn_data)->skip_ms_calls = TRUE;
			}

			if (CONN_GET_STATE(el->conn) == CONN_ALLOCED) {
				scheme = el->emulated_scheme;
				scheme_len = el->emulated_scheme_len;
			} else {
				scheme = el->conn->scheme;
				scheme_len = el->conn->scheme_len;
			}
			array_init(row);
			add_assoc_long_ex(row, "errno", sizeof("errno") - 1, MS_CALL_ORIGINAL_CONN_DATA_METHOD(get_error_no)(el->conn TSRMLS_CC));
			{
				const char * err = MS_CALL_ORIGINAL_CONN_DATA_METHOD(get_error_str)(el->conn TSRMLS_CC);
				add_assoc_stringl_ex(row, "error", sizeof("error") - 1, (char*) err, strlen(err), 1 /*dup*/);
			}
			{
				const char * sqlstate = MS_CALL_ORIGINAL_CONN_DATA_METHOD(get_sqlstate)(el->conn TSRMLS_CC);
				add_assoc_stringl_ex(row, "sqlstate", sizeof("sqlstate") - 1, (char*) sqlstate, strlen(sqlstate), 1 /*dup*/);
			}
			add_assoc_zval_ex(ret, scheme, scheme_len, row);

			if (el_conn_data && *el_conn_data) {
				(*el_conn_data)->skip_ms_calls = FALSE;
			}
		}
		END_ITERATE_OVER_SERVER_LISTS;
	}

	DBG_RETURN(ret);
}
/* }}} */
#endif

/* {{{ mysqlnd_ms::select_db */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, select_db)(MYSQLND_CONN_DATA * const proxy_conn, const char * const db, unsigned int db_len TSRMLS_DC)
{
	enum_func_status ret = PASS;
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);

	DBG_ENTER("mysqlnd_ms::select_db");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, proxy_conn->thread_id);
	if (CONN_DATA_NOT_SET(conn_data)) {
		DBG_RETURN(MS_CALL_ORIGINAL_CONN_DATA_METHOD(select_db)(proxy_conn, db, db_len TSRMLS_CC));
	} else {
		MYSQLND_MS_LIST_DATA * el;
		BEGIN_ITERATE_OVER_SERVER_LISTS(el, &(*conn_data)->master_connections, &(*conn_data)->slave_connections);
		{
			if (CONN_GET_STATE(el->conn) > CONN_ALLOCED && CONN_GET_STATE(el->conn) != CONN_QUIT_SENT) {
				MS_DECLARE_AND_LOAD_CONN_DATA(el_conn_data, el->conn);

				if (el_conn_data && *el_conn_data) {
					(*el_conn_data)->skip_ms_calls = TRUE;
				}

				if (PASS != MS_CALL_ORIGINAL_CONN_DATA_METHOD(select_db)(el->conn, db, db_len TSRMLS_CC)) {
					ret = FAIL;
				}

				if (el_conn_data && *el_conn_data) {
					(*el_conn_data)->skip_ms_calls = FALSE;
				}
			} else if (CONN_GET_STATE(el->conn) == CONN_ALLOCED) {
				/* lazy connection */
				if (el->db) {
					mnd_pefree(el->db, el->persistent);
				}
				el->db_len = db_len;
				el->db = db? mnd_pestrndup(db, db_len, el->persistent) : NULL;
			}
		}
		END_ITERATE_OVER_SERVER_LISTS;
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::set_charset */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, set_charset)(MYSQLND_CONN_DATA * const proxy_conn, const char * const csname TSRMLS_DC)
{
	enum_func_status ret = PASS;
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);

	DBG_ENTER("mysqlnd_ms::set_charset");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, proxy_conn->thread_id);
	if (CONN_DATA_NOT_SET(conn_data)) {
		DBG_RETURN(MS_CALL_ORIGINAL_CONN_DATA_METHOD(set_charset)(proxy_conn, csname TSRMLS_CC));
	} else {
		MYSQLND_MS_LIST_DATA * el;
		BEGIN_ITERATE_OVER_SERVER_LISTS(el, &(*conn_data)->master_connections, &(*conn_data)->slave_connections);
		{
			enum_mysqlnd_connection_state state = CONN_GET_STATE(el->conn);

			if (state != CONN_QUIT_SENT) {
				MS_DECLARE_AND_LOAD_CONN_DATA(el_conn_data, el->conn);

				if (el_conn_data && *el_conn_data) {
					(*el_conn_data)->skip_ms_calls = TRUE;
#ifdef BUFFERED_COMMANDS
					if (state == CONN_ALLOCED) {
						(*el_conn_data)->on_command = MYSQLND_MS_BCAST_BUFFER_COMMAND;
					}
#endif
				}
				if (state == CONN_ALLOCED) {
					ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(set_client_option)(el->conn, MYSQL_SET_CHARSET_NAME, csname TSRMLS_CC);
				} else if (PASS != MS_CALL_ORIGINAL_CONN_DATA_METHOD(set_charset)(el->conn, csname TSRMLS_CC)) {
					ret = FAIL;
				}

				if (el_conn_data && *el_conn_data) {
#ifdef BUFFERED_COMMANDS
					if (state == CONN_ALLOCED) {
						(*el_conn_data)->on_command = MYSQLND_MS_BCAST_NOP;
						CONN_SET_STATE(el->conn, state);
					}
#endif
					(*el_conn_data)->skip_ms_calls = FALSE;
				}
			}
		}
		END_ITERATE_OVER_SERVER_LISTS;
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::set_server_option */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, set_server_option)(MYSQLND_CONN_DATA * const proxy_conn, enum_mysqlnd_server_option option TSRMLS_DC)
{
	enum_func_status ret = PASS;
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);

	DBG_ENTER("mysqlnd_ms::set_server_option");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, proxy_conn->thread_id);
	if (CONN_DATA_NOT_SET(conn_data)) {
		DBG_RETURN(MS_CALL_ORIGINAL_CONN_DATA_METHOD(set_server_option)(proxy_conn, option TSRMLS_CC));
	} else {
		MYSQLND_MS_LIST_DATA * el;
		BEGIN_ITERATE_OVER_SERVER_LISTS(el, &(*conn_data)->master_connections, &(*conn_data)->slave_connections);
		{
			if (CONN_GET_STATE(el->conn) > CONN_ALLOCED && CONN_GET_STATE(el->conn) != CONN_QUIT_SENT) {
				MS_DECLARE_AND_LOAD_CONN_DATA(el_conn_data, el->conn);

				if (el_conn_data && *el_conn_data) {
					(*el_conn_data)->skip_ms_calls = TRUE;
				}

				if (PASS != MS_CALL_ORIGINAL_CONN_DATA_METHOD(set_server_option)(el->conn, option TSRMLS_CC)) {
					ret = FAIL;
				}

				if (el_conn_data && *el_conn_data) {
					(*el_conn_data)->skip_ms_calls = FALSE;
#ifndef MYSQLND_HAS_INJECTION_FEATURE
					/* If enabled, we don't need to enable if global trx injection is using multi statement */
					switch (option) {
						case MYSQL_OPTION_MULTI_STATEMENTS_ON:
							(*el_conn_data)->global_trx.multi_statement_user_enabled = TRUE;
							break;
						case MYSQL_OPTION_MULTI_STATEMENTS_OFF:
							(*el_conn_data)->global_trx.multi_statement_user_enabled = FALSE;
							break;
					}
#endif
				}
			}
		}
		END_ITERATE_OVER_SERVER_LISTS;
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::set_client_option */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, set_client_option)(MYSQLND_CONN_DATA * const proxy_conn, enum_mysqlnd_option option, const char * const value TSRMLS_DC)
{
	enum_func_status ret = PASS;
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);

	DBG_ENTER("mysqlnd_ms::set_server_option");
	if (CONN_DATA_NOT_SET(conn_data)) {
		DBG_RETURN(MS_CALL_ORIGINAL_CONN_DATA_METHOD(set_client_option)(proxy_conn, option, value TSRMLS_CC));
	} else {
		MYSQLND_MS_LIST_DATA * el;
		BEGIN_ITERATE_OVER_SERVER_LISTS(el, &(*conn_data)->master_connections, &(*conn_data)->slave_connections);
		{
			MS_DECLARE_AND_LOAD_CONN_DATA(el_conn_data, el->conn);

			if (el_conn_data && *el_conn_data) {
				(*el_conn_data)->skip_ms_calls = TRUE;
			}

			if (PASS != MS_CALL_ORIGINAL_CONN_DATA_METHOD(set_client_option)(el->conn, option, value TSRMLS_CC)) {
				ret = FAIL;
			}

			if (el_conn_data && *el_conn_data) {
				(*el_conn_data)->skip_ms_calls = FALSE;
			}
		}
		END_ITERATE_OVER_SERVER_LISTS;
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::next_result */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, next_result)(MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	enum_func_status ret;
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	DBG_ENTER("mysqlnd_ms::next_result");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, conn->thread_id);
	ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(next_result)(conn TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::more_results */
static zend_bool
MYSQLND_METHOD(mysqlnd_ms, more_results)(const MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	zend_bool ret;
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	const MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	DBG_ENTER("mysqlnd_ms::more_results");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, conn->thread_id);
	ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(more_results)(conn TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::errno */
static unsigned int
MYSQLND_METHOD(mysqlnd_ms, error_no)(const MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	const MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	return MYSQLND_MS_ERROR_INFO(conn).error_no;
}
/* }}} */


/* {{{ mysqlnd_ms::error */
static const char *
MYSQLND_METHOD(mysqlnd_ms, error)(const MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	const MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	return MYSQLND_MS_ERROR_INFO(conn).error;
}
/* }}} */


/* {{{ mysqlnd_conn::sqlstate */
static const char *
MYSQLND_METHOD(mysqlnd_ms, sqlstate)(const MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	const MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	return (MYSQLND_MS_ERROR_INFO(conn).sqlstate[0]) ? (MYSQLND_MS_ERROR_INFO(conn).sqlstate): MYSQLND_SQLSTATE_NULL;
}
/* }}} */


/* {{{ mysqlnd_ms::field_count */
static unsigned int
MYSQLND_METHOD(mysqlnd_ms, field_count)(const MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	const MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	return conn->field_count;
}
/* }}} */


/* {{{ mysqlnd_conn::thread_id */
static uint64_t
MYSQLND_METHOD(mysqlnd_ms, thread_id)(const MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	const MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	return conn->thread_id;
}
/* }}} */


/* {{{ mysqlnd_ms::insert_id */
static uint64_t
MYSQLND_METHOD(mysqlnd_ms, insert_id)(const MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	const MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	return MYSQLND_MS_UPSERT_STATUS(conn).last_insert_id;
}
/* }}} */


/* {{{ mysqlnd_ms::affected_rows */
static uint64_t
MYSQLND_METHOD(mysqlnd_ms, affected_rows)(const MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	const MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	return MYSQLND_MS_UPSERT_STATUS(conn).affected_rows;
}
/* }}} */


/* {{{ mysqlnd_ms::warning_count */
static unsigned int
MYSQLND_METHOD(mysqlnd_ms, warning_count)(const MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	const MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	return MYSQLND_MS_UPSERT_STATUS(conn).warning_count;
}
/* }}} */


/* {{{ mysqlnd_ms::info */
static const char *
MYSQLND_METHOD(mysqlnd_ms, info)(const MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	const MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	return conn->last_message;
}
/* }}} */


#if MYSQLND_VERSION_ID >= 50009
/* {{{ MYSQLND_METHOD(mysqlnd_ms, set_autocommit) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, set_autocommit)(MYSQLND_CONN_DATA * proxy_conn, unsigned int mode TSRMLS_DC)
{
	enum_func_status ret = PASS;
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);

	DBG_ENTER("mysqlnd_ms::set_autocommit");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, proxy_conn->thread_id);
	if (CONN_DATA_NOT_SET(conn_data)) {
		DBG_INF("Using original");
		ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(set_autocommit)(proxy_conn, mode TSRMLS_CC);
		DBG_RETURN(ret);
	} else {

#ifndef MYSQLND_HAS_INJECTION_FEATURE
		if (((TRUE == (*conn_data)->stgy.in_transaction) && mode) &&
			CONN_DATA_TRY_TRX_INJECTION(conn_data))
		{
			/*
			Implicit commit when autocommit(false) ..query().. autocommit(true).
			Must inject before second=current autocommit() call.
			*/
			MS_TRX_INJECT(ret, proxy_conn, conn_data);
			MYSQLND_MS_INC_STATISTIC((PASS == ret) ? MS_STAT_GTID_IMPLICIT_COMMIT_SUCCESS :
				MS_STAT_GTID_IMPLICIT_COMMIT_FAILURE);

			if (FAIL == ret) {
				if (TRUE == (*conn_data)->global_trx.report_error)
					DBG_RETURN(ret);

				ret = PASS;
				SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(proxy_conn));
			}
		}
#endif

		MYSQLND_MS_LIST_DATA * el;
		BEGIN_ITERATE_OVER_SERVER_LISTS(el, &(*conn_data)->master_connections, &(*conn_data)->slave_connections);
		{
			if (CONN_GET_STATE(el->conn) != CONN_QUIT_SENT) {
				MYSQLND_MS_CONN_DATA ** el_conn_data = (MYSQLND_MS_CONN_DATA **) mysqlnd_plugin_get_plugin_connection_data_data(el->conn, mysqlnd_ms_plugin_id);
				if (el_conn_data && *el_conn_data) {
					(*el_conn_data)->skip_ms_calls = TRUE;
				}
				if (CONN_GET_STATE(el->conn) == CONN_ALLOCED) {
					ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(set_client_option)(el->conn, MYSQL_INIT_COMMAND,
																		  (mode) ? "SET AUTOCOMMIT=1":"SET AUTOCOMMIT=0"
																		  TSRMLS_CC);
				} else if (PASS != MS_CALL_ORIGINAL_CONN_DATA_METHOD(set_autocommit)(el->conn, mode TSRMLS_CC)) {
					ret = FAIL;
				}
				/* TODO: Can we ever have a connection with no plugin data? If so, tx handling will break! */
				if (el_conn_data && *el_conn_data) {
					(*el_conn_data)->skip_ms_calls = FALSE;
					(*el_conn_data)->stgy.in_transaction = (mode) ? FALSE : TRUE;
				}
			}
		}
		END_ITERATE_OVER_SERVER_LISTS;
	}

	MYSQLND_MS_INC_STATISTIC(mode? MS_STAT_TRX_AUTOCOMMIT_ON:MS_STAT_TRX_AUTOCOMMIT_OFF);
	DBG_INF_FMT("in_transaction = %d", (*conn_data)->stgy.in_transaction);

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_tx_commit_or_rollback */
static enum_func_status
mysqlnd_ms_tx_commit_or_rollback(MYSQLND_CONN_DATA * conn, zend_bool commit TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, conn);
	enum_func_status ret = PASS;

	DBG_ENTER("mysqlnd_ms_tx_commit_or_rollback");

	if (conn_data && *conn_data && (*conn_data)->stgy.last_used_conn) {
		conn = (*conn_data)->stgy.last_used_conn;
		MS_LOAD_CONN_DATA(conn_data, conn);
	}
	DBG_INF_FMT("conn="MYSQLND_LLU_SPEC, conn->thread_id);

	if (CONN_GET_STATE(conn) == CONN_ALLOCED && !CONN_DATA_NOT_SET(conn_data)) {
		/* TODO: what is this good for ? */
		DBG_RETURN(PASS);
	}

	/* Must add query before committing ... */
#ifndef MYSQLND_HAS_INJECTION_FEATURE
	if ((conn_data && *conn_data && TRUE == commit) &&
		((TRUE == (*conn_data)->stgy.in_transaction)) &&
		CONN_DATA_TRY_TRX_INJECTION(conn_data))
	{
		MS_TRX_INJECT(ret, conn, conn_data);
		MYSQLND_MS_INC_STATISTIC((PASS == ret) ? MS_STAT_GTID_COMMIT_SUCCESS : MS_STAT_GTID_COMMIT_FAILURE);

		if (FAIL == ret) {
			if (TRUE == (*conn_data)->global_trx.report_error)
				DBG_RETURN(ret);

			SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(conn));
		}
	}
#endif

	if (conn_data && *conn_data)
		(*conn_data)->skip_ms_calls = TRUE;
	/* TODO: the recursive rattle tail is terrible, we should optimize and call query() directly */
	ret = commit? MS_CALL_ORIGINAL_CONN_DATA_METHOD(tx_commit)(conn TSRMLS_CC) :
					MS_CALL_ORIGINAL_CONN_DATA_METHOD(tx_rollback)(conn TSRMLS_CC);
	if (conn_data && *conn_data)
		(*conn_data)->skip_ms_calls = FALSE;

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ MYSQLND_METHOD(mysqlnd_ms, tx_commit) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, tx_commit)(MYSQLND_CONN_DATA * conn TSRMLS_DC)
{
	enum_func_status ret = FAIL;

	DBG_ENTER("mysqlnd_ms::tx_commit");
	ret = mysqlnd_ms_tx_commit_or_rollback(conn, TRUE TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ MYSQLND_METHOD(mysqlnd_ms, tx_rollback) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, tx_rollback)(MYSQLND_CONN_DATA * conn TSRMLS_DC)
{
	enum_func_status ret = FAIL;

	DBG_ENTER("mysqlnd_ms::tx_rollback");
	ret = mysqlnd_ms_tx_commit_or_rollback(conn, FALSE TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */
#endif


/* {{{ mysqlnd_ms::statistic */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, get_server_statistics)(MYSQLND_CONN_DATA * proxy_conn, char **message, unsigned int * message_len TSRMLS_DC)
{
	enum_func_status ret;
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	DBG_ENTER("mysqlnd_ms::statistic");
	DBG_INF_FMT("conn="MYSQLND_LLU_SPEC, conn->thread_id);
	ret = MS_CALL_ORIGINAL_CONN_DATA_METHOD(get_server_statistics)(conn, message, message_len TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::get_server_version */
static unsigned long
MYSQLND_METHOD(mysqlnd_ms, get_server_version)(const MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	const MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	return MS_CALL_ORIGINAL_CONN_DATA_METHOD(get_server_version)(conn TSRMLS_CC);
}
/* }}} */


/* {{{ mysqlnd_ms::get_server_info */
static const char *
MYSQLND_METHOD(mysqlnd_ms, get_server_info)(const MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	const MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	return MS_CALL_ORIGINAL_CONN_DATA_METHOD(get_server_information)(conn TSRMLS_CC);
}
/* }}} */


/* {{{ mysqlnd_ms::get_host_info */
static const char *
MYSQLND_METHOD(mysqlnd_ms, get_host_info)(const MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	const MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	return MS_CALL_ORIGINAL_CONN_DATA_METHOD(get_host_information)(conn TSRMLS_CC);
}
/* }}} */


/* {{{ mysqlnd_ms::get_proto_info */
static unsigned int
MYSQLND_METHOD(mysqlnd_ms, get_proto_info)(const MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	const MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	return MS_CALL_ORIGINAL_CONN_DATA_METHOD(get_protocol_information)(conn TSRMLS_CC);
}
/* }}} */


/* {{{ mysqlnd_ms::charset_name */
static const char *
MYSQLND_METHOD(mysqlnd_ms, charset_name)(const MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	const MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	return MS_CALL_ORIGINAL_CONN_DATA_METHOD(charset_name)(conn TSRMLS_CC);
}
/* }}} */


/* {{{ mysqlnd_ms::get_connection_stats */
static void
MYSQLND_METHOD(mysqlnd_ms, get_connection_stats)(const MYSQLND_CONN_DATA * const proxy_conn, zval * return_value TSRMLS_DC ZEND_FILE_LINE_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	const MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	MS_CALL_ORIGINAL_CONN_DATA_METHOD(get_statistics)(conn, return_value TSRMLS_CC ZEND_FILE_LINE_CC);
}
/* }}} */


/* {{{ mysqlnd_ms::dump_debug_info */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, dump_debug_info)(MYSQLND_CONN_DATA * const proxy_conn TSRMLS_DC)
{
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);
	MYSQLND_CONN_DATA * conn = ((*conn_data) && (*conn_data)->stgy.last_used_conn)? (*conn_data)->stgy.last_used_conn:proxy_conn;

	DBG_ENTER("mysqlnd_ms::dump_debug_info");
	DBG_RETURN(MS_CALL_ORIGINAL_CONN_DATA_METHOD(server_dump_debug_information)(conn TSRMLS_CC));
}
/* }}} */


/* {{{ mysqlnd_ms_stmt::prepare */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms_stmt, prepare)(MYSQLND_STMT * const s, const char * const query, unsigned int query_len TSRMLS_DC)
{
	MYSQLND_MS_CONN_DATA ** conn_data = NULL;
	MYSQLND_CONN_DATA * connection = NULL;
	enum_func_status ret = FAIL;
	DBG_ENTER("mysqlnd_ms_stmt::prepare");
	DBG_INF_FMT("query=%s", query);

	if (!s || !s->data || !s->data->conn ||
		!(MS_LOAD_CONN_DATA(conn_data, s->data->conn)) ||
		!*conn_data || (*conn_data)->skip_ms_calls)
	{
		DBG_INF("skip MS");
		DBG_RETURN(ms_orig_mysqlnd_stmt_methods->prepare(s, query, query_len TSRMLS_CC));
	}

	/* this can possibly reroute us to another server */
	connection = mysqlnd_ms_pick_server_ex((*conn_data)->proxy_conn, query, query_len TSRMLS_CC);
	DBG_INF_FMT("Connection %p", connection);

	if (connection != s->data->conn) {
		/* free what we have */
		s->m->net_close(s, TRUE TSRMLS_CC);
		mnd_pefree(s->data, s->data->persistent);

		/* new handle */
		{
			MYSQLND_STMT * new_handle = MS_CALL_ORIGINAL_CONN_DATA_METHOD(stmt_init)(connection TSRMLS_CC);
			if (!new_handle || !new_handle->data) {
				DBG_ERR("new_handle is null");
				DBG_RETURN(FAIL);
			}
			s->data = new_handle->data;
			mnd_pefree(new_handle, new_handle->data->persistent);
		}
	}

	ret = ms_orig_mysqlnd_stmt_methods->prepare(s, query, query_len TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */


#ifndef MYSQLND_HAS_INJECTION_FEATURE
/* {{{ mysqlnd_ms_stmt::execute */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms_stmt, execute)(MYSQLND_STMT * const s TSRMLS_DC)
{
	enum_func_status ret = PASS;
	MYSQLND_MS_CONN_DATA ** conn_data = NULL;
	MYSQLND_CONN_DATA * connection = NULL;

	DBG_ENTER("mysqlnd_ms_stmt::execute");

	if (!s || !s->data || !s->data->conn ||
		!(MS_LOAD_CONN_DATA(conn_data, s->data->conn)) ||
		!conn_data || !*conn_data || (*conn_data)->skip_ms_calls)
	{
		DBG_INF("skip MS");
		ret = ms_orig_mysqlnd_stmt_methods->execute(s TSRMLS_CC);
		DBG_RETURN(ret);
	}
	connection = s->data->conn;
	DBG_INF_FMT("conn="MYSQLND_LLU_SPEC, connection->thread_id);

	if (CONN_DATA_TRY_TRX_INJECTION(conn_data) &&
		(FALSE == (*conn_data)->stgy.in_transaction))
	{
		/* autocommit mode */
		MS_TRX_INJECT(ret, connection, conn_data);
		MYSQLND_MS_INC_STATISTIC((PASS == ret) ? MS_STAT_GTID_AUTOCOMMIT_SUCCESS :
			MS_STAT_GTID_AUTOCOMMIT_FAILURE);

		if (FAIL == ret) {
			/* TODO: copy to stmt error? */
			if (TRUE == (*conn_data)->global_trx.report_error)
				DBG_RETURN(ret);

			/* TODO: clear stmt error? */
			SET_EMPTY_ERROR(MYSQLND_MS_ERROR_INFO(connection));
		}
	}

	ret = ms_orig_mysqlnd_stmt_methods->execute(s TSRMLS_CC);

	DBG_RETURN(ret);
}
/* }}} */
#endif

/* {{{ mysqlnd_conn::ssl_set */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, ssl_set)(MYSQLND_CONN_DATA * const proxy_conn, const char * key, const char * const cert,
									const char * const ca, const char * const capath, const char * const cipher TSRMLS_DC)
{
	enum_func_status ret = PASS;
	MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, proxy_conn);

	DBG_ENTER("mysqlnd_ms::ssl_set");
	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, proxy_conn->thread_id);
	if (CONN_DATA_NOT_SET(conn_data)) {
		DBG_RETURN(MS_CALL_ORIGINAL_CONN_DATA_METHOD(ssl_set)(proxy_conn, key, cert, ca, capath, cipher TSRMLS_CC));
	} else {
		MYSQLND_MS_LIST_DATA * el;
		BEGIN_ITERATE_OVER_SERVER_LISTS(el, &(*conn_data)->master_connections, &(*conn_data)->slave_connections);
		{
			if (PASS != MS_CALL_ORIGINAL_CONN_DATA_METHOD(ssl_set)(el->conn, key, cert, ca, capath, cipher TSRMLS_CC)) {
				ret = FAIL;
			}
		}
		END_ITERATE_OVER_SERVER_LISTS;
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms::close */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, close)(MYSQLND * conn, enum_connection_close_type close_type TSRMLS_DC)
{
	enum_func_status ret;
	DBG_ENTER("mysqlnd_ms::close");

	DBG_INF_FMT("Using thread "MYSQLND_LLU_SPEC, MS_GET_CONN_DATA_FROM_CONN(conn)->thread_id);
	/*
	  Force cleaning of the master and slave lists.
	  In the master list this connection is present and free_reference will be called, and later
	  in the original `close` the data of this connection will be destructed as refcount will become 0.
	*/
	mysqlnd_ms_conn_free_plugin_data(MS_GET_CONN_DATA_FROM_CONN(conn) TSRMLS_CC);
	ret = MS_CALL_ORIGINAL_CONN_HANDLE_METHOD(close)(conn, close_type TSRMLS_CC);
	DBG_RETURN(ret);
}
/* }}} */



/* {{{ mysqlnd_ms_register_hooks*/
void
mysqlnd_ms_register_hooks()
{
	MS_LOAD_AND_COPY_CONN_DATA_METHODS(ms_orig_mysqlnd_conn_methods, my_mysqlnd_conn_methods);

	my_mysqlnd_conn_methods.connect				= MYSQLND_METHOD(mysqlnd_ms, connect);
	my_mysqlnd_conn_methods.query				= MYSQLND_METHOD(mysqlnd_ms, query);
	my_mysqlnd_conn_methods.send_query			= MYSQLND_METHOD(mysqlnd_ms, send_query);
	my_mysqlnd_conn_methods.use_result			= MYSQLND_METHOD(mysqlnd_ms, use_result);
	my_mysqlnd_conn_methods.store_result		= MYSQLND_METHOD(mysqlnd_ms, store_result);
	my_mysqlnd_conn_methods.dtor				= MYSQLND_METHOD_PRIVATE(mysqlnd_ms, dtor);
	my_mysqlnd_conn_methods.escape_string		= MYSQLND_METHOD(mysqlnd_ms, escape_string);
	my_mysqlnd_conn_methods.change_user			= MYSQLND_METHOD(mysqlnd_ms, change_user);
	my_mysqlnd_conn_methods.ping				= MYSQLND_METHOD(mysqlnd_ms, ping);
	my_mysqlnd_conn_methods.kill_connection		= MYSQLND_METHOD(mysqlnd_ms, kill);
	my_mysqlnd_conn_methods.get_thread_id		= MYSQLND_METHOD(mysqlnd_ms, thread_id);
	my_mysqlnd_conn_methods.select_db			= MYSQLND_METHOD(mysqlnd_ms, select_db);
	my_mysqlnd_conn_methods.set_charset			= MYSQLND_METHOD(mysqlnd_ms, set_charset);
	my_mysqlnd_conn_methods.set_server_option	= MYSQLND_METHOD(mysqlnd_ms, set_server_option);
	my_mysqlnd_conn_methods.set_client_option	= MYSQLND_METHOD(mysqlnd_ms, set_client_option);
	my_mysqlnd_conn_methods.next_result			= MYSQLND_METHOD(mysqlnd_ms, next_result);
	my_mysqlnd_conn_methods.more_results		= MYSQLND_METHOD(mysqlnd_ms, more_results);
	my_mysqlnd_conn_methods.get_error_no		= MYSQLND_METHOD(mysqlnd_ms, error_no);
	my_mysqlnd_conn_methods.get_error_str		= MYSQLND_METHOD(mysqlnd_ms, error);
	my_mysqlnd_conn_methods.get_sqlstate		= MYSQLND_METHOD(mysqlnd_ms, sqlstate);

	my_mysqlnd_conn_methods.ssl_set				= MYSQLND_METHOD(mysqlnd_ms, ssl_set);


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

	my_mysqlnd_conn_methods.get_server_statistics	= MYSQLND_METHOD(mysqlnd_ms, get_server_statistics);
	my_mysqlnd_conn_methods.get_server_version		= MYSQLND_METHOD(mysqlnd_ms, get_server_version);
	my_mysqlnd_conn_methods.get_server_information	= MYSQLND_METHOD(mysqlnd_ms, get_server_info);
	my_mysqlnd_conn_methods.get_host_information	= MYSQLND_METHOD(mysqlnd_ms, get_host_info);
	my_mysqlnd_conn_methods.get_protocol_information= MYSQLND_METHOD(mysqlnd_ms, get_proto_info);
	my_mysqlnd_conn_methods.charset_name			= MYSQLND_METHOD(mysqlnd_ms, charset_name);
	my_mysqlnd_conn_methods.get_statistics			= MYSQLND_METHOD(mysqlnd_ms, get_connection_stats);
	my_mysqlnd_conn_methods.server_dump_debug_information = MYSQLND_METHOD(mysqlnd_ms, dump_debug_info);
#ifdef BUFFERED_COMMANDS
	my_mysqlnd_conn_methods.simple_command 			= MYSQLND_METHOD(mysqlnd_ms, simple_command);
#endif

	MS_SET_CONN_DATA_METHODS(&my_mysqlnd_conn_methods);

#if PHP_VERSION_ID < 50399
	my_mysqlnd_conn_methods.close				= MYSQLND_METHOD(mysqlnd_ms, close);
#else
	MS_LOAD_AND_COPY_CONN_HANDLE_METHODS(ms_orig_mysqlnd_conn_handle_methods, my_mysqlnd_conn_handle_methods);
	my_mysqlnd_conn_handle_methods.close		= MYSQLND_METHOD(mysqlnd_ms, close);
	MS_SET_CONN_HANDLE_METHODS(&my_mysqlnd_conn_handle_methods);
#endif

	ms_orig_mysqlnd_stmt_methods = mysqlnd_stmt_get_methods();
	memcpy(&my_mysqlnd_stmt_methods, ms_orig_mysqlnd_stmt_methods, sizeof(struct st_mysqlnd_stmt_methods));

	my_mysqlnd_stmt_methods.prepare = MYSQLND_METHOD(mysqlnd_ms_stmt, prepare);
#ifndef MYSQLND_HAS_INJECTION_FEATURE
	my_mysqlnd_stmt_methods.execute = MYSQLND_METHOD(mysqlnd_ms_stmt, execute);
#endif

	mysqlnd_stmt_set_methods(&my_mysqlnd_stmt_methods);
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
