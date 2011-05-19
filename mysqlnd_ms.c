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
static void
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

#if 1
/* {{{ mysqlnd_ms_connect_to_host */
static enum_func_status
mysqlnd_ms_connect_to_host(MYSQLND * conn, char * host, zend_llist * conn_list, struct st_mysqlnd_ms_conn_credentials * cred,
						   zend_bool use_lazy_connections, zend_bool persistent TSRMLS_DC)
{
	enum_func_status ret;
	unsigned int port = cred->port;
	char * socket = cred->socket;
	char * colon_pos;

	DBG_ENTER("mysqlnd_ms_connect_to_host");
	if (host && NULL != (colon_pos = strchr(host, ':'))) {
		if (colon_pos[1] == '/') {
			/* unix path */
			socket = colon_pos + 1;
			DBG_INF_FMT("overwriting socket : %s", socket);
		} else if (isdigit(colon_pos[1])) {
			/* port */
			port = atoi(colon_pos+1);
			DBG_INF_FMT("overwriting port : %d", port);
		}
		*colon_pos = '\0'; /* strip the tail */
	}

	if (use_lazy_connections) {
		DBG_INF("Lazy connection");
		ret = PASS;
	} else {
		ret = ms_orig_mysqlnd_conn_methods->connect(conn, host, cred->user, cred->passwd, cred->passwd_len, cred->db, cred->db_len,
													port, socket, cred->mysql_flags TSRMLS_CC);
	}

	if (ret == PASS) {
		MYSQLND_MS_LIST_DATA new_element = {0};
		new_element.conn = conn;
		new_element.host = host? mnd_pestrdup(host, persistent) : NULL;
		new_element.persistent = persistent;
		new_element.port = port;
		new_element.socket = socket? mnd_pestrdup(socket, conn->persistent) : NULL;
		new_element.emulated_scheme_len = mysqlnd_ms_get_scheme_from_list_data(&new_element, &new_element.emulated_scheme,
																			   persistent TSRMLS_CC);
		zend_llist_add_element(conn_list, &new_element);
	}
	DBG_INF_FMT("ret=%s", ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */
#endif

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
		ret = ms_orig_mysqlnd_conn_methods->connect(conn, host, user, passwd, passwd_len, db, db_len, port, socket, mysql_flags TSRMLS_CC);
		if (ret == PASS) {
			MYSQLND_MS_LIST_DATA new_element = {0};
			new_element.conn = conn;
			new_element.host = host? mnd_pestrdup(host, conn->persistent) : NULL;
			new_element.persistent = conn->persistent;
			new_element.emulated_scheme_len = mysqlnd_ms_get_scheme_from_list_data(&new_element, &new_element.emulated_scheme,
																				   conn->persistent TSRMLS_CC);
			zend_llist_add_element(&(*conn_data_pp)->master_connections, &new_element);
		}
	} else {
		do {
			unsigned int port_to_use;
			const char * socket_to_use;
			zend_bool value_exists = FALSE, is_list_value = FALSE, use_lazy_connections = TRUE, use_lazy_connections_list_value = FALSE, have_slaves = FALSE;
			char * lazy_connections;
			/* create master connection */
			char * master;

			if (!hotloading) {
				MYSQLND_MS_CONFIG_JSON_LOCK(mysqlnd_ms_json_config);
			}

			lazy_connections = mysqlnd_ms_config_json_string(mysqlnd_ms_json_config, host, host_len, LAZY_NAME, sizeof(LAZY_NAME) - 1,
													 &use_lazy_connections, &use_lazy_connections_list_value, FALSE TSRMLS_CC);
			/* ignore if lazy_connections ini entry exists or not */
			use_lazy_connections = TRUE;
			if (lazy_connections) {
				/* lazy_connections ini entry exists, disabled? */
				use_lazy_connections = !mysqlnd_ms_config_json_string_is_bool_false(lazy_connections);
				mnd_efree(lazy_connections);
				lazy_connections = NULL;
			}

			master = mysqlnd_ms_config_json_string(mysqlnd_ms_json_config, host, host_len, MASTER_NAME, sizeof(MASTER_NAME) - 1,
												  &value_exists, &is_list_value, FALSE TSRMLS_CC);
			if (FALSE == value_exists) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Cannot find master section in config");
				SET_CLIENT_ERROR(conn->error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, MYSQLND_MS_ERROR_PREFIX " Cannot find master section in config");
				DBG_RETURN(FAIL);
			}
			ret = mysqlnd_ms_connect_to_host(conn, master, &(*conn_data_pp)->master_connections, &(*conn_data_pp)->cred,
											 use_lazy_connections, conn->persistent TSRMLS_CC);
			mnd_efree(master);
			master = NULL;
			if (ret != PASS) {
				MYSQLND_MS_INC_STATISTIC(MS_STAT_NON_LAZY_CONN_MASTER_FAILURE);
				break;
			}
			if (!use_lazy_connections) {
				MYSQLND_MS_INC_STATISTIC(MS_STAT_NON_LAZY_CONN_MASTER_SUCCESS);
			}
			DBG_INF_FMT("Master connection "MYSQLND_LLU_SPEC" established", conn->m->get_thread_id(conn TSRMLS_CC));
#ifdef MYSQLND_MS_MULTIMASTER_ENABLED
			/* More master connections ? */
			if (is_list_value) {
				DBG_INF("We have more master connections. Connect...");
				do {
					master = mysqlnd_ms_config_json_string(mysqlnd_ms_json_config, host, host_len,
														   MASTER_NAME, sizeof(MASTER_NAME) - 1,
														   &value_exists, &is_list_value, FALSE TSRMLS_CC);
					DBG_INF_FMT("value_exists=%d master=%s", value_exists, master);
					if (value_exists && master) {
						MYSQLND * tmp_conn = mysqlnd_init(conn->persistent);

						ret = mysqlnd_ms_connect_to_host(tmp_conn, master, &(*conn_data_pp)->master_connections, &(*conn_data_pp)->cred,
														 use_lazy_connections, conn->persistent TSRMLS_CC);
						mnd_efree(master);
						master = NULL;
						if (ret != PASS) {
							tmp_conn->m->dtor(tmp_conn TSRMLS_CC);
							MYSQLND_MS_INC_STATISTIC(MS_STAT_NON_LAZY_CONN_MASTER_FAILURE);
						} else {
							DBG_INF_FMT("Further master connection "MYSQLND_LLU_SPEC" established", tmp_conn->thread_id);
							if (!use_lazy_connections) {
								MYSQLND_MS_INC_STATISTIC(MS_STAT_NON_LAZY_CONN_MASTER_SUCCESS);
							}
						}
					} /* if value_exists */
				} while (value_exists);
			}
#endif
			/* create slave slave_connections */
			do {
				char * slave = mysqlnd_ms_config_json_string(mysqlnd_ms_json_config, host, host_len, SLAVE_NAME, sizeof(SLAVE_NAME) - 1,
													 &value_exists, &is_list_value, FALSE TSRMLS_CC);
				if (value_exists && is_list_value && slave) {
					MYSQLND * tmp_conn = mysqlnd_init(conn->persistent);
					have_slaves = TRUE;

					{
						char * colon_pos;
						port_to_use = port;
						socket_to_use = socket;

						if (NULL != (colon_pos = strchr(slave, ':'))) {
							if (colon_pos[1] == '/') {
								/* unix path */
								socket_to_use = colon_pos + 1;
								DBG_INF_FMT("overwriting socket : %s", socket_to_use);
							} else if (isdigit(colon_pos[1])) {
								/* port */
								port_to_use = atoi(colon_pos+1);
								DBG_INF_FMT("overwriting port : %d", port_to_use);
							}
							*colon_pos = '\0'; /* strip the tail */
						}
					}

					if (use_lazy_connections) {
						DBG_INF("Lazy slave connections");
						ret = PASS;
					} else {
						ret = ms_orig_mysqlnd_conn_methods->connect(tmp_conn, slave, user, passwd, passwd_len, db, db_len, port_to_use, socket_to_use, mysql_flags TSRMLS_CC);
					}

					if (ret == PASS) {
						MYSQLND_MS_LIST_DATA new_element = {0};
						new_element.conn = tmp_conn;
						new_element.host = slave? mnd_pestrdup(slave, conn->persistent) : NULL;
						new_element.persistent = conn->persistent;
						new_element.port = port_to_use;
						new_element.socket = socket_to_use? mnd_pestrdup(socket_to_use, conn->persistent) : NULL;
						new_element.emulated_scheme_len = mysqlnd_ms_get_scheme_from_list_data(&new_element, &(new_element.emulated_scheme),
																							   conn->persistent TSRMLS_CC);
						zend_llist_add_element(&(*conn_data_pp)->slave_connections, &new_element);
						DBG_INF_FMT("Slave connection "MYSQLND_LLU_SPEC" established", tmp_conn->m->get_thread_id(tmp_conn TSRMLS_CC));

						if (!use_lazy_connections) {
							MYSQLND_MS_INC_STATISTIC(MS_STAT_NON_LAZY_CONN_SLAVE_SUCCESS);
						}
					} else {
						MYSQLND_MS_INC_STATISTIC(MS_STAT_NON_LAZY_CONN_SLAVE_FAILURE);
						tmp_conn->m->dtor(tmp_conn TSRMLS_CC);
					}
					mnd_efree(slave);
					slave = NULL;
				}
			} while (value_exists);

			if (FALSE == have_slaves) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " Cannot find slaves section in config");
				SET_CLIENT_ERROR(conn->error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, MYSQLND_MS_ERROR_PREFIX " Cannot find slaves section in config");
				DBG_RETURN(FAIL);
			}

			{
				char * pick_strategy = mysqlnd_ms_config_json_string(mysqlnd_ms_json_config, host, host_len, PICK_NAME, sizeof(PICK_NAME) - 1,
												  				&value_exists, &is_list_value, FALSE TSRMLS_CC);
				(*conn_data_pp)->stgy.pick_strategy = (*conn_data_pp)->stgy.fallback_pick_strategy = DEFAULT_PICK_STRATEGY;

				if (value_exists && pick_strategy) {
					/* random is a substing of random_once thus we check first for random_once */
					if (!strncasecmp(PICK_RROBIN, pick_strategy, sizeof(PICK_RROBIN) - 1)) {
						(*conn_data_pp)->stgy.pick_strategy = (*conn_data_pp)->stgy.fallback_pick_strategy = SERVER_PICK_RROBIN;
					} else if (!strncasecmp(PICK_RANDOM_ONCE, pick_strategy, sizeof(PICK_RANDOM_ONCE) - 1)) {
						(*conn_data_pp)->stgy.pick_strategy = (*conn_data_pp)->stgy.fallback_pick_strategy = SERVER_PICK_RANDOM_ONCE;
					} else if (!strncasecmp(PICK_RANDOM, pick_strategy, sizeof(PICK_RANDOM) - 1)) {
						(*conn_data_pp)->stgy.pick_strategy = (*conn_data_pp)->stgy.fallback_pick_strategy = SERVER_PICK_RANDOM;
					} else if (!strncasecmp(PICK_USER, pick_strategy, sizeof(PICK_USER) - 1)) {
						(*conn_data_pp)->stgy.pick_strategy = SERVER_PICK_USER;
						if (is_list_value) {
							mnd_efree(pick_strategy);
							pick_strategy = mysqlnd_ms_config_json_string(mysqlnd_ms_json_config, host, host_len, PICK_NAME, sizeof(PICK_NAME) - 1,
												  				&value_exists, &is_list_value, FALSE TSRMLS_CC);
							if (pick_strategy) {
								if (!strncasecmp(PICK_RANDOM_ONCE, pick_strategy, sizeof(PICK_RANDOM_ONCE) - 1)) {
									(*conn_data_pp)->stgy.fallback_pick_strategy = SERVER_PICK_RANDOM_ONCE;
								} else if (!strncasecmp(PICK_RANDOM, pick_strategy, sizeof(PICK_RANDOM) - 1)) {
									(*conn_data_pp)->stgy.fallback_pick_strategy = SERVER_PICK_RANDOM;
								} else if (!strncasecmp(PICK_RROBIN, pick_strategy, sizeof(PICK_RROBIN) - 1)) {
									(*conn_data_pp)->stgy.fallback_pick_strategy = SERVER_PICK_RROBIN;
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
				char * failover_strategy = mysqlnd_ms_config_json_string(mysqlnd_ms_json_config, host, host_len, FAILOVER_NAME, sizeof(FAILOVER_NAME) - 1,
												  				&value_exists, &is_list_value, FALSE TSRMLS_CC);
				(*conn_data_pp)->stgy.failover_strategy = DEFAULT_FAILOVER_STRATEGY;

				if (value_exists && failover_strategy) {
					if (!strncasecmp(FAILOVER_DISABLED, failover_strategy, sizeof(FAILOVER_DISABLED) - 1)) {
						(*conn_data_pp)->stgy.failover_strategy = SERVER_FAILOVER_DISABLED;
					} else if (!strncasecmp(FAILOVER_MASTER, failover_strategy, sizeof(FAILOVER_MASTER) - 1)) {
						(*conn_data_pp)->stgy.failover_strategy = SERVER_FAILOVER_MASTER;
					}
				}
				if (failover_strategy) {
					mnd_efree(failover_strategy);
				}
			}

			{
				char * master_on_write = mysqlnd_ms_config_json_string(mysqlnd_ms_json_config, host, host_len, MASTER_ON_WRITE_NAME, sizeof(MASTER_ON_WRITE_NAME) - 1,
													 &value_exists, &is_list_value, FALSE TSRMLS_CC);

				(*conn_data_pp)->stgy.mysqlnd_ms_flag_master_on_write = FALSE;
				(*conn_data_pp)->stgy.master_used = FALSE;

				if (value_exists && master_on_write) {
					DBG_INF("Master on write active");
					(*conn_data_pp)->stgy.mysqlnd_ms_flag_master_on_write = !mysqlnd_ms_config_json_string_is_bool_false(master_on_write);
				}
				if (master_on_write) {
					mnd_efree(master_on_write);
				}
			}

			{
				char * trx_strategy = mysqlnd_ms_config_json_string(mysqlnd_ms_json_config, host, host_len, TRX_STICKINESS_NAME, sizeof(TRX_STICKINESS_NAME) - 1,
												  				&value_exists, &is_list_value, FALSE TSRMLS_CC);
				(*conn_data_pp)->stgy.trx_stickiness_strategy = DEFAULT_TRX_STICKINESS_STRATEGY;
				(*conn_data_pp)->stgy.in_transaction = FALSE;

				if (value_exists && trx_strategy) {
# if PHP_VERSION_ID >= 50399
					if (!strncasecmp(TRX_STICKINESS_MASTER, trx_strategy, sizeof(TRX_STICKINESS_MASTER) - 1)) {
						DBG_INF("Transaction strickiness strategy = master");
						(*conn_data_pp)->stgy.trx_stickiness_strategy = TRX_STICKINESS_STRATEGY_MASTER;
					}
# else
					php_error_docref(NULL TSRMLS_CC, E_WARNING, MYSQLND_MS_ERROR_PREFIX " trx_stickiness_strategy is not supported before PHP 5.3.99");
# endif
				}
				if (trx_strategy) {
					mnd_efree(trx_strategy);
				}
			}

		} while (0);
		mysqlnd_ms_config_json_reset_section(mysqlnd_ms_json_config, host, host_len, FALSE TSRMLS_CC);
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


#if 0
/* {{{ MYSQLND_METHOD(mysqlnd_ms, query2) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_ms, query2)(MYSQLND * conn, const char * query, unsigned int query_len TSRMLS_DC)
{
	int ret;
	struct st_mysqlnd_query_parser * parser;
	DBG_ENTER("mysqlnd_ms::query2");
	parser = mysqlnd_qp_create_parser(TSRMLS_C);
	if (parser) {
		ret = mysqlnd_qp_start_parser(parser, query, query_len TSRMLS_CC);
		DBG_INF_FMT("mysqlnd_qp_start_parser=%d", ret);
		DBG_INF_FMT("db=%s table=%s org_table=%s",
				parser->parse_info.db? parser->parse_info.db:"n/a",
				parser->parse_info.table? parser->parse_info.table:"n/a",
				parser->parse_info.org_table? parser->parse_info.org_table:"n/a"
			);
		mysqlnd_qp_free_parser(parser TSRMLS_CC);
		DBG_RETURN(PASS);
	}
	DBG_RETURN(FAIL);
}
/* }}} */
#endif

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
