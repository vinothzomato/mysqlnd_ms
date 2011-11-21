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

/* $Id: mysqlnd_ms.h 311091 2011-05-16 15:42:48Z andrey $ */
#ifndef MYSQLND_MS_ENUM_N_DEF_H
#define MYSQLND_MS_ENUM_N_DEF_H

#if MYSQLND_VERSION_ID < 50010
typedef MYSQLND MYSQLND_CONN_DATA;
#endif

#if MYSQLND_VERSION_ID >= 50010
#define MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, connection) \
	MYSQLND_MS_CONN_DATA ** conn_data = \
		(MYSQLND_MS_CONN_DATA **) mysqlnd_plugin_get_plugin_connection_data_data((connection), mysqlnd_ms_plugin_id)

#define MS_LOAD_CONN_DATA(conn_data, connection) \
	(conn_data) = (MYSQLND_MS_CONN_DATA **) mysqlnd_plugin_get_plugin_connection_data_data((connection), mysqlnd_ms_plugin_id)

#define MS_CALL_ORIGINAL_CONN_HANDLE_METHOD(method) ms_orig_mysqlnd_conn_handle_methods->method
#define MS_CALL_ORIGINAL_CONN_DATA_METHOD(method) ms_orig_mysqlnd_conn_methods->method
extern struct st_mysqlnd_conn_data_methods * ms_orig_mysqlnd_conn_methods;
extern struct st_mysqlnd_conn_methods * ms_orig_mysqlnd_conn_handle_methods;

#else

#define MS_DECLARE_AND_LOAD_CONN_DATA(conn_data, connection) \
	MYSQLND_MS_CONN_DATA ** conn_data = \
		(MYSQLND_MS_CONN_DATA **) mysqlnd_plugin_get_plugin_connection_data((connection), mysqlnd_ms_plugin_id)

#define MS_LOAD_CONN_DATA(conn_data, connection) \
	(conn_data) = (MYSQLND_MS_CONN_DATA **) mysqlnd_plugin_get_plugin_connection_data((connection), mysqlnd_ms_plugin_id)

#define MS_CALL_ORIGINAL_CONN_HANDLE_METHOD(method) ms_orig_mysqlnd_conn_methods->method
#define MS_CALL_ORIGINAL_CONN_DATA_METHOD(method) ms_orig_mysqlnd_conn_methods->method
extern struct st_mysqlnd_conn_methods * ms_orig_mysqlnd_conn_methods;
#endif

#ifndef MYSQLND_HAS_INJECTION_FEATURE
#define MS_LOAD_STMT_DATA(stmt_data, statement) \
	(stmt_data) = \
		(MYSQLND_MS_STMT_DATA **)  mysqlnd_plugin_get_plugin_stmt_data((statement), mysqlnd_ms_plugin_id);
#endif

#define BEGIN_ITERATE_OVER_SERVER_LISTS(el, masters, slaves) \
{ \
	DBG_INF_FMT("master(%p) has %d, slave(%p) has %d", (masters), zend_llist_count((masters)), (slaves), zend_llist_count((slaves))); \
	{ \
		MYSQLND_MS_LIST_DATA ** el_pp;\
		zend_llist * lists[] = {NULL, (masters), (slaves), NULL}; \
		zend_llist ** list = lists; \
		while (*++list) { \
			zend_llist_position	pos; \
			/* search the list of easy handles hanging off the multi-handle */ \
			for (el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(*list, &pos); \
				 el_pp && ((el) = *el_pp) && (el)->conn; \
				 el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(*list, &pos)) \
			{ \

#define END_ITERATE_OVER_SERVER_LISTS \
			} \
		} \
	} \
}


#define BEGIN_ITERATE_OVER_SERVER_LISTS_NEW(el, masters, slaves) \
{ \
	DBG_INF_FMT("master(%p) has %d, slave(%p) has %d", (masters), zend_llist_count((masters)), (slaves), zend_llist_count((slaves))); \
	{ \
		MYSQLND_MS_LIST_DATA ** el_pp; \
		zend_llist * internal_master_list = (masters); \
		zend_llist * internal_slave_list = (slaves); \
		zend_llist * internal_list = internal_master_list; \
		zend_llist_position	pos; \
		/* search the list of easy handles hanging off the multi-handle */ \
		for ((el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(internal_list, &pos)) \
			  || ((internal_list = internal_slave_list) \
			      && \
				  (el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(internal_list, &pos))) ; \
			 el_pp && ((el) = *el_pp) && (el)->conn; \
		 	 (el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex(internal_list, &pos)) \
			 || \
			 ( \
				(internal_list == internal_master_list) \
				&& \
				/* yes, we need an assignment */ \
				(internal_list = internal_slave_list) \
				&& \
				(el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex(internal_slave_list, &pos)) \
			 ) \
			) \
		{ \

#define END_ITERATE_OVER_SERVER_LISTS_NEW \
		} \
	} \
}

#define BEGIN_ITERATE_OVER_SERVER_LIST(el, list) \
{ \
	DBG_INF_FMT("list(%p) has %d", (list), zend_llist_count((list))); \
	{ \
		MYSQLND_MS_LIST_DATA ** MACRO_el_pp;\
		zend_llist_position	MACRO_pos; \
		/* search the list of easy handles hanging off the multi-handle */ \
		for (((el) = NULL), MACRO_el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_first_ex((list), &MACRO_pos); \
			 MACRO_el_pp && ((el) = *MACRO_el_pp) && (el)->conn; \
			 ((el) = NULL), MACRO_el_pp = (MYSQLND_MS_LIST_DATA **) zend_llist_get_next_ex((list), &MACRO_pos)) \
		{ \

#define END_ITERATE_OVER_SERVER_LIST \
		} \
	} \
}

#define MASTER_SWITCH "ms=master"
#define SLAVE_SWITCH "ms=slave"
#define LAST_USED_SWITCH "ms=last_used"
#define ALL_SERVER_SWITCH "ms=all"


#define MASTER_NAME				"master"
#define SLAVE_NAME				"slave"
#define PICK_RANDOM				"random"
#define PICK_ONCE				"sticky"
#define PICK_RROBIN				"roundrobin"
#define PICK_USER				"user"
#define PICK_USER_MULTI			"user_multi"
#define PICK_TABLE				"table"
#define PICK_QOS				"quality_of_service"
#define LAZY_NAME				"lazy_connections"
#define FAILOVER_NAME			"failover"
#define FAILOVER_DISABLED 		"disabled"
#define FAILOVER_MASTER			"master"
#define MASTER_ON_WRITE_NAME	"master_on_write"
#define TRX_STICKINESS_NAME		"trx_stickiness"
#define TRX_STICKINESS_MASTER	"master"
#define TABLE_RULES				"rules"
#define SECT_HOST_NAME			"host"
#define SECT_PORT_NAME			"port"
#define SECT_SOCKET_NAME		"socket"
#define SECT_USER_NAME			"user"
#define SECT_PASS_NAME			"password"
#define SECT_DB_NAME			"db"
#define SECT_CONNECT_FLAGS_NAME	"connect_flags"
#define SECT_FILTER_PRIORITY_NAME "priority"
#define SECT_FILTER_NAME		"filters"
#define SECT_USER_CALLBACK		"callback"
#define SECT_QOS_STRONG			"strong_consistency"
#define SECT_QOS_SESSION		"session_consistency"
#define SECT_QOS_EVENTUAL		"eventual_consistency"
#define SECT_G_TRX_NAME			"global_transaction_id_injection"
#define SECT_G_TRX_ON_CONNECT	"on_connect"
#define SECT_G_TRX_ON_COMMIT	"on_commit"
#define SECT_G_TRX_SET_ON_SLAVE "set_on_slave"
#define SECT_G_TRX_REPORT_ERROR "report_error"
#define SECT_G_TRX_MULTI_STMT   "use_multi_statement"


typedef enum
{
	STATEMENT_SELECT,
	STATEMENT_INSERT,
	STATEMENT_UPDATE,
	STATEMENT_DELETE,
	STATEMENT_TRUNCATE,
	STATEMENT_REPLACE,
	STATEMENT_RENAME,
	STATEMENT_ALTER,
	STATEMENT_DROP,
	STATEMENT_CREATE
} enum_mysql_statement_type;


enum enum_which_server
{
	USE_MASTER,
	USE_SLAVE,
	USE_LAST_USED,
	USE_ALL
};


enum mysqlnd_ms_server_pick_strategy
{
	SERVER_PICK_RROBIN,
	SERVER_PICK_RANDOM,
	SERVER_PICK_USER,
	SERVER_PICK_USER_MULTI,
	SERVER_PICK_TABLE,
	SERVER_PICK_QOS,
	SERVER_PICK_LAST_ENUM_ENTRY
};

/* it should work also without any params, json config to the ctor will be NULL */
#define DEFAULT_PICK_STRATEGY SERVER_PICK_RANDOM

enum mysqlnd_ms_server_failover_strategy
{
	SERVER_FAILOVER_DISABLED,
	SERVER_FAILOVER_MASTER
};

#define DEFAULT_FAILOVER_STRATEGY SERVER_FAILOVER_DISABLED

enum mysqlnd_ms_trx_stickiness_strategy
{
	TRX_STICKINESS_STRATEGY_DISABLED,
	TRX_STICKINESS_STRATEGY_MASTER
};
#define DEFAULT_TRX_STICKINESS_STRATEGY TRX_STICKINESS_STRATEGY_DISABLED


typedef enum mysqlnd_ms_collected_stats
{
	MS_STAT_USE_SLAVE,
	MS_STAT_USE_MASTER,
	MS_STAT_USE_SLAVE_GUESS,
	MS_STAT_USE_MASTER_GUESS,
	MS_STAT_USE_SLAVE_FORCED,
	MS_STAT_USE_MASTER_FORCED,
	MS_STAT_USE_LAST_USED_FORCED,
	MS_STAT_USE_SLAVE_CALLBACK,
	MS_STAT_USE_MASTER_CALLBACK,
	MS_STAT_NON_LAZY_CONN_SLAVE_SUCCESS,
	MS_STAT_NON_LAZY_CONN_SLAVE_FAILURE,
	MS_STAT_NON_LAZY_CONN_MASTER_SUCCESS,
	MS_STAT_NON_LAZY_CONN_MASTER_FAILURE,
	MS_STAT_LAZY_CONN_SLAVE_SUCCESS,
	MS_STAT_LAZY_CONN_SLAVE_FAILURE,
	MS_STAT_LAZY_CONN_MASTER_SUCCESS,
	MS_STAT_LAZY_CONN_MASTER_FAILURE,
	MS_STAT_TRX_AUTOCOMMIT_ON,
	MS_STAT_TRX_AUTOCOMMIT_OFF,
	MS_STAT_TRX_MASTER_FORCED,
#ifndef MYSQLND_HAS_INJECTION_FEATURE
	MS_STAT_GTID_AUTOCOMMIT_SUCCESS,
	MS_STAT_GTID_AUTOCOMMIT_FAILURE,
	MS_STAT_GTID_COMMIT_SUCCESS,
	MS_STAT_GTID_COMMIT_FAILURE,
#endif
	MS_STAT_LAST /* Should be always the last */
} enum_mysqlnd_ms_collected_stats;

#define MYSQLND_MS_INC_STATISTIC(stat) MYSQLND_INC_STATISTIC(MYSQLND_MS_G(collect_statistics), mysqlnd_ms_stats, (stat))
#define MYSQLND_MS_INC_STATISTIC_W_VALUE(stat, value) MYSQLND_INC_STATISTIC_W_VALUE(MYSQLND_MS_G(collect_statistics), mysqlnd_ms_stats, (stat), (value))


typedef struct st_mysqlnd_ms_list_data
{
	char * name_from_config;
	MYSQLND_CONN_DATA * conn;
	char * host;
	char * user;
	char * passwd;
	size_t passwd_len;
	unsigned int port;
	char * socket;
	char * db;
	size_t db_len;
	unsigned long connect_flags;
	char * emulated_scheme;
	size_t emulated_scheme_len;
	zend_bool persistent;

} MYSQLND_MS_LIST_DATA;


typedef struct st_mysqlnd_ms_filter_data
{
	void (*specific_dtor)(struct st_mysqlnd_ms_filter_data * TSRMLS_DC);
	char * name;
	size_t name_len;
	enum mysqlnd_ms_server_pick_strategy pick_type;
	zend_bool multi_filter;
	zend_bool persistent;
} MYSQLND_MS_FILTER_DATA;


typedef struct st_mysqlnd_ms_filter_user_data
{
	MYSQLND_MS_FILTER_DATA parent;
	zval * user_callback;
	zend_bool callback_valid;
} MYSQLND_MS_FILTER_USER_DATA;


typedef struct st_mysqlnd_ms_filter_table_data
{
	MYSQLND_MS_FILTER_DATA parent;
	HashTable master_rules;
	HashTable slave_rules;
} MYSQLND_MS_FILTER_TABLE_DATA;


typedef struct st_mysqlnd_ms_filter_rr_data
{
	MYSQLND_MS_FILTER_DATA parent;
	HashTable master_context;
	HashTable slave_context;
} MYSQLND_MS_FILTER_RR_DATA;


typedef struct st_mysqlnd_ms_filter_random_data
{
	MYSQLND_MS_FILTER_DATA parent;
	struct {
		HashTable master_context;
		HashTable slave_context;
		zend_bool once;
	} sticky;
} MYSQLND_MS_FILTER_RANDOM_DATA;


enum mysqlnd_ms_filter_qos_consistency
{
	CONSISTENCY_STRONG,
	CONSISTENCY_SESSION,
	CONSISTENCY_EVENTUAL,
	CONSISTENCY_LAST_ENUM_ENTRY
};


typedef struct st_mysqlnd_ms_filter_qos_data
{
	MYSQLND_MS_FILTER_DATA parent;
	enum mysqlnd_ms_filter_qos_consistency consistency;
} MYSQLND_MS_FILTER_QOS_DATA;


enum mysqlnd_ms_on_broadcast_command
{
	MYSQLND_MS_BCAST_NOP = 0,
	MYSQLND_MS_BCAST_BUFFER_COMMAND,
	MYSQLND_MS_BCAST_AUTO_CONNECT
};


struct mysqlnd_ms_lb_strategies
{
	HashTable table_filters;

	enum mysqlnd_ms_server_failover_strategy failover_strategy;

	zend_bool mysqlnd_ms_flag_master_on_write;
	zend_bool master_used;

	enum mysqlnd_ms_trx_stickiness_strategy trx_stickiness_strategy;
	zend_bool in_transaction;

	MYSQLND_CONN_DATA * last_used_conn;
	MYSQLND_CONN_DATA * random_once_slave;

	zend_llist * filters;
};


typedef struct st_mysqlnd_ms_conn_data
{
	zend_bool initialized;
	zend_bool skip_ms_calls;
	enum mysqlnd_ms_on_broadcast_command on_command;
	MYSQLND_CONN_DATA * proxy_conn;
	char * connect_host;
	zend_llist delayed_commands;
	zend_llist master_connections;
	zend_llist slave_connections;

	struct mysqlnd_ms_lb_strategies stgy;

	struct st_mysqlnd_ms_conn_credentials {
		char * user;
		char * passwd;
		size_t passwd_len;
		char * db;
		size_t db_len;
		unsigned int port;
		char * socket;
		unsigned long mysql_flags;
	} cred;

#ifndef MYSQLND_HAS_INJECTION_FEATURE
	struct st_mysqlnd_ms_global_trx_injection {
		char * on_commit;
		size_t on_commit_len;
		zend_bool is_master;
		zend_bool set_on_slave;
		zend_bool report_error;
		zend_bool use_multi_statement;
		zend_bool multi_statement_user_enabled;
		zend_bool multi_statement_gtx_enabled;
	} global_trx;
	zend_bool connection_opened;
#endif

} MYSQLND_MS_CONN_DATA;

typedef struct st_mysqlnd_ms_table_filter
{
	char * host_id;
	size_t host_id_len;
	char * wild;
	size_t wild_len;
#ifdef WE_NEED_NEXT
	struct st_mysqlnd_ms_table_filter * next;
#endif
	unsigned int priority;
	zend_bool persistent;
} MYSQLND_MS_TABLE_FILTER;


typedef struct st_mysqlnd_ms_command
{
	enum php_mysqlnd_server_command command;
	zend_uchar * payload;
	size_t payload_len;
	enum mysqlnd_packet_type ok_packet;
	zend_bool silent;
	zend_bool ignore_upsert_status;
	zend_bool persistent;
} MYSQLND_MS_COMMAND;

#ifndef MYSQLND_HAS_INJECTION_FEATURE
typedef struct st_mysqlnd_ms_stmt_data {
	zend_bool skip_ms_calls;
	zend_bool use_buffered_result;
	zend_bool buffered_result_fetched;
	MYSQLND_RES * global_trx_injection_res;
} MYSQLND_MS_STMT_DATA;
#endif

#endif /* MYSQLND_MS_ENUM_N_DEF_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
