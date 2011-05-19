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

#define MASTER_NAME				"master"
#define SLAVE_NAME				"slave"
#define PICK_NAME				"pick"
#define PICK_RANDOM				"random"
#define PICK_RANDOM_ONCE 		"random_once"
#define PICK_RROBIN				"roundrobin"
#define PICK_USER				"user"
#define LAZY_NAME				"lazy_connections"
#define FAILOVER_NAME			"failover"
#define FAILOVER_DISABLED 		"disabled"
#define FAILOVER_MASTER			"master"
#define MASTER_ON_WRITE_NAME	"master_on_write"
#define TRX_STICKINESS_NAME		"trx_stickiness"
#define TRX_STICKINESS_MASTER	"master"


enum mysqlnd_ms_server_pick_strategy
{
	SERVER_PICK_RROBIN,
	SERVER_PICK_RANDOM,
	SERVER_PICK_RANDOM_ONCE,
	SERVER_PICK_USER
};

#define DEFAULT_PICK_STRATEGY SERVER_PICK_RANDOM_ONCE

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
	MYSQLND * conn;
	char * host;
	unsigned int port;
	char * socket;
	char * emulated_scheme;
	size_t emulated_scheme_len;
	zend_bool persistent;
} MYSQLND_MS_LIST_DATA;

typedef struct st_mysqlnd_ms_connection_data
{
	char * connect_host;
	zend_llist master_connections;
	zend_llist slave_connections;
	MYSQLND * last_used_connection;
	MYSQLND * random_once;

	struct mysqlnd_ms_lb_strategies{
		enum mysqlnd_ms_server_pick_strategy pick_strategy;
		enum mysqlnd_ms_server_pick_strategy fallback_pick_strategy;

		enum mysqlnd_ms_server_failover_strategy failover_strategy;

		zend_bool mysqlnd_ms_flag_master_on_write;
		zend_bool master_used;

		enum mysqlnd_ms_trx_stickiness_strategy trx_stickiness_strategy;
		zend_bool in_transaction;
	} stgy;

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
} MYSQLND_MS_CONNECTION_DATA;

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
