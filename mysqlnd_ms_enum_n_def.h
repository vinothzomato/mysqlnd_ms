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


#define MASTER_NAME				"master"
#define SLAVE_NAME				"slave"
#define PICK_RANDOM				"random"
#define PICK_ONCE				"sticky"
#define PICK_RROBIN				"roundrobin"
#define PICK_USER				"user"
#define PICK_USER_MULTI			"user_multi"
#define PICK_TABLE				"table"
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


enum mysqlnd_ms_server_pick_strategy
{
	SERVER_PICK_RROBIN,
	SERVER_PICK_RANDOM,
	SERVER_PICK_USER,
	SERVER_PICK_USER_MULTI,
	SERVER_PICK_TABLE,
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

typedef struct st_mysqlnd_ms_list_data
{
	char * name_from_config;
	MYSQLND * conn;
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
	zend_bool persistent;
} MYSQLND_MS_FILTER_DATA;


typedef struct st_mysqlnd_ms_filter_user_data
{
	MYSQLND_MS_FILTER_DATA parent;
	zval * user_callback;
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


struct mysqlnd_ms_lb_strategies
{
	HashTable table_filters;

	enum mysqlnd_ms_server_failover_strategy failover_strategy;

	zend_bool mysqlnd_ms_flag_master_on_write;
	zend_bool master_used;

	enum mysqlnd_ms_trx_stickiness_strategy trx_stickiness_strategy;
	zend_bool in_transaction;

	MYSQLND * last_used_conn;
	MYSQLND * random_once_slave;

	zend_llist * filters;
};


typedef struct st_mysqlnd_ms_conn_data
{
	char * connect_host;
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

extern struct st_mysqlnd_conn_methods * ms_orig_mysqlnd_conn_methods;

#endif /* MYSQLND_MS_ENUM_N_DEF_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
