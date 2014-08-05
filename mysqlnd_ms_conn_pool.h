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

#ifndef MYSQLND_MS_CONN_POOL_H
#define MYSQLND_MS_CONN_POOL_H

#ifndef SMART_STR_START_SIZE
#define SMART_STR_START_SIZE 1024
#endif
#ifndef SMART_STR_PREALLOC
#define SMART_STR_PREALLOC 256
#endif
#include "ext/standard/php_smart_str.h"
#include "Zend/zend_llist.h"
#include "Zend/zend_hash.h"

#ifdef ZTS
#include "TSRM.h"
#endif

#include "php.h"
#include "ext/standard/info.h"

#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_priv.h"

#include "mysqlnd_ms_enum_n_def.h"


/* Buffered commands for conection state alignment */

typedef enum
{
	/* CAUTION: Order matters! The pool will replay commands in the order given here.
	 * It may be that we learn about use cases where the order is not perfect...
	 */
	SET_CHARSET = 0,
	SELECT_DB = 1,
	SET_SERVER_OPTION = 2,
	SET_CLIENT_OPTION = 3,
} enum_mysqlnd_pool_cmd;

typedef enum_func_status (*cb_pool_cmd_select_db)(MYSQLND_MS_CONN_DATA ** conn_data, MYSQLND_MS_LIST_DATA * el,
												  const char * const db, unsigned int db_len TSRMLS_DC);

typedef struct st_mysqlnd_pool_cmd_select_db {
	cb_pool_cmd_select_db cb;
	char * db;
	unsigned int db_len;
} MYSQLND_MS_POOL_CMD_SELECT_DB;

typedef enum_func_status (*cb_pool_cmd_set_charset)(MYSQLND_MS_CONN_DATA ** conn_data, MYSQLND_MS_LIST_DATA * el,
													const char * const csname TSRMLS_DC);

typedef struct st_mysqlnd_pool_cmd_set_charset {
	cb_pool_cmd_set_charset cb;
	char * csname;
} MYSQLND_MS_POOL_CMD_SET_CHARSET;

typedef enum_func_status (*cb_pool_cmd_set_server_option)(MYSQLND_MS_CONN_DATA ** conn_data, MYSQLND_MS_LIST_DATA * el,
														  enum_mysqlnd_server_option option TSRMLS_DC);

typedef struct st_mysqlnd_pool_cmd_set_server_option {
	cb_pool_cmd_set_server_option cb;
	enum_mysqlnd_server_option option;
} MYSQLND_MS_POOL_CMD_SET_SERVER_OPTION;

typedef enum_func_status (*cb_pool_cmd_set_client_option)(MYSQLND_MS_CONN_DATA ** conn_data, MYSQLND_MS_LIST_DATA * el,
														  enum_mysqlnd_option option, const char * const value TSRMLS_DC);

typedef struct st_mysqlnd_pool_cmd_set_client_option {
	cb_pool_cmd_set_client_option cb;
	enum_mysqlnd_server_option option;
	char * value;
} MYSQLND_MS_POOL_CMD_SET_CLIENT_OPTION;

typedef struct st_mysqlnd_pool_cmd {
	enum_mysqlnd_pool_cmd cmd;
	void * data; /* MYSQLND_MS_POOL_CMD_SELECT_DB and friends */
	zend_bool persistent;
} MYSQLND_MS_POOL_CMD;


typedef struct st_mysqlnd_pool {

	/* private */
	struct {
		zend_bool persistent;

		/* Dtor from MS for master and slave list entries */
		dtor_func_t ms_list_data_dtor;

		zend_llist replace_listener;

		/* _All_ connection (currently used and inactive) - list of POOL_ENTRY */
		HashTable master_list;
		HashTable slave_list;

		/* Buffered selection of active connections: those MS shall use at the very
		 * moment for picking a server, those that belong to the currently used cluster.
		 * List of MS_LIST_DATA (as ever before)
		 */
		zend_llist active_master_list;
		zend_llist active_slave_list;

		HashTable buffered_cmds;

	} data;

	/* public methods */

	/* From an MS point of view: flush all connections, clear all slaves and masters
	 * The connections will not necessarily be closed but marked inactive.
	 * Assume Fabric constantly jumps between two shard groups, if we closed all
	 * connections on flush, we would have to open them the very next minute again.
	 * Or, let there be an XA transaction going on and Fabric flushes all connections
	 * including the ones of the XA transaction...
	 */
	enum_func_status (*flush_active)(struct st_mysqlnd_pool * pool TSRMLS_DC);

	/* Clear all connections: active (see flush_active) and inactive. But only those
	 * which have a reference counter of zero. After the call the active list will always
	 * be empty but the inactive list may still hold some connections.
	 * Returns the number of connections not cleared and still in the pool (for use)
	 * because the reference counter was not zero.
	 */
	enum_func_status (*clear_all)(struct st_mysqlnd_pool * pool, unsigned int * referenced TSRMLS_DC);

	/* Creating a hash key for a pooled connection */
	/* Note: this is the only place where MYSQLND_MS_LIST_DATA is inspected and modified */
	void (*init_pool_hash_key)(MYSQLND_MS_LIST_DATA * data);
	void (*get_conn_hash_key)(smart_str * hash_key /* out */,
							const char * unique_name_from_config,
							const char * host, const char * user,
							const char * passwd, size_t passwd_len,
							unsigned int port, char * socket,
							char * db, size_t db_len, unsigned long connect_flags,
							zend_bool persistent);

	/* Add connections to the pool */
	enum_func_status (*add_slave)(struct st_mysqlnd_pool * pool, smart_str * hash_key,
								  MYSQLND_MS_LIST_DATA * data, zend_bool persistent TSRMLS_DC);
	enum_func_status (*add_master)(struct st_mysqlnd_pool * pool, smart_str * hash_key,
								   MYSQLND_MS_LIST_DATA * data, zend_bool persistent TSRMLS_DC);

	/* Test whether a connection is already in the pool
	 * data, is_master, is_active are out parameters
	 */
	zend_bool (*connection_exists)(struct st_mysqlnd_pool * pool, smart_str * hash_key,
								  MYSQLND_MS_LIST_DATA **data,
								  zend_bool * is_master, zend_bool * is_active TSRMLS_DC);

	/* Reactivate a connection: move it from the inactive to the active list
	 *
	 * Assume you are replacing the active list frequently, e.g. because the user
	 * switches from Fabric shard group A to B and back all the time. To
	 * replace the currently active server list, say A's, with a new one, say B's,
	 * do flush(). Then, before opening new connections to B, use connection_exists()
	 * to see whether the pool already has a connection open to the desired server.
	 * If so, call connection_reactivate() to put it from the pool into the active
	 * list again. add_*() will fail for hash_keys that already exist. Don't open
	 * new connections for the same key...
	 * Note: roles (master/slave) cannot change - saw no need, change if you like...
	 */
	enum_func_status (*connection_reactivate)(struct st_mysqlnd_pool * pool,
									  smart_str * hash_key, zend_bool is_master TSRMLS_DC);


	enum_func_status (*register_replace_listener)(struct st_mysqlnd_pool * pool,
												void (*f)(struct st_mysqlnd_pool * pool, void * data TSRMLS_DC),
												void * data TSRMLS_DC);

	enum_func_status (*notify_replace_listener)(struct st_mysqlnd_pool * pool TSRMLS_DC);


	/* Get list of active connections. Active connections belong to the
	 * currently selected cluster (e.g. shard group).
	 * TODO: returning zend_llist - can we prevent the user from changing llist? */
	zend_llist * (*get_active_masters)(struct st_mysqlnd_pool * pool TSRMLS_DC);
	zend_llist * (*get_active_slaves)(struct st_mysqlnd_pool * pool TSRMLS_DC);

	/* Reference counting for connections. Should a module require a
	 * connection to stay open, it can set a reference.
	 *
	 * TODO See implementation - semantics are a bit unclear
	 */
	enum_func_status (*add_reference)(struct st_mysqlnd_pool * pool,
									  MYSQLND_CONN_DATA * const conn TSRMLS_DC);
	enum_func_status (*free_reference)(struct st_mysqlnd_pool * pool,
										 MYSQLND_CONN_DATA * const conn TSRMLS_DC);


	/* By default MS will align the state (charset, options, ...) of all servers
	 * currently used. When the active server list is replaced (flush()),
	 * the new connections/servers may be in a different state than the ones
	 * that have been used before. MS lets the pool know which calls it has dispatched
	 * to the currently active servers. Then, when the list of active servers
	 * is replaced, it is up to the user of the pool to decide whether to
	 * replay the previously dispatched commands (replay_cmds()).
	 * The pool will not replay commands automatically and implicitly.
	 * TODO: create a way to replay selected commands
	 * FIXME: Likely, the order in which commands are replayed matters. It can't be set.
	 */
	enum_func_status (*dispatch_select_db)(struct st_mysqlnd_pool * pool,
											cb_pool_cmd_select_db cb,
											const char * db,
											unsigned int db_len TSRMLS_DC);

	enum_func_status (*dispatch_set_charset)(struct st_mysqlnd_pool * pool,
											cb_pool_cmd_set_charset cb,
											const char * const csname TSRMLS_DC);

	enum_func_status (*dispatch_set_server_option)(struct st_mysqlnd_pool * pool,
												cb_pool_cmd_set_server_option cb,
												enum_mysqlnd_server_option option TSRMLS_DC);

	enum_func_status (*dispatch_set_client_option)(struct st_mysqlnd_pool *pool,
												cb_pool_cmd_set_client_option cb,
												enum_mysqlnd_option option,
												const char * const value TSRMLS_DC);

	enum_func_status (*replay_cmds)(struct st_mysqlnd_pool * pool,
									MYSQLND_MS_CONN_DATA ** proxy_conn_data TSRMLS_DC);

	void (*dtor)(struct st_mysqlnd_pool * pool TSRMLS_DC);


} MYSQLND_MS_POOL;


typedef void (*func_pool_replace_listener)(MYSQLND_MS_POOL * pool, void * data TSRMLS_DC);
typedef struct st_mysqlnd_pool_listener {
	func_pool_replace_listener listener;
	void * data;
} MYSQLND_MS_POOL_LISTENER;


MYSQLND_MS_POOL * mysqlnd_ms_pool_ctor(dtor_func_t ms_list_data_dtor, zend_bool persistent TSRMLS_DC);

typedef struct st_mysqlnd_pool_entry {
	/* This is the data we keep on behalf of the MS core */
	MYSQLND_MS_LIST_DATA * data;
	/* dtor the MS core provides to free LIST_DATA member */
	/* TODO: can we simplify the pointer logic, one level less? */
	dtor_func_t ms_list_data_dtor;

	/* Private entries that make the pool logic itself */

	/* Use persistent memory? */
	zend_bool persistent;
	/* Does this connection belong to the currently used cluster? */
	zend_bool active;
	/* How often has the cluster this connection belongs to been used */
	unsigned int activation_counter;
	/* How many MS components (e.g. XA, core) reference this connection */
	unsigned int ref_counter;
} MYSQLND_MS_POOL_ENTRY;


#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
