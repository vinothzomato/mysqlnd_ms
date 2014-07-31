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


typedef struct st_mysqlnd_pool {

	/* private */
	struct {
		zend_bool persistent;

		/* Dtor from MS for master and slave list entries */
		llist_dtor_func_t ms_list_data_dtor;

		/* _All_ connection (currently used and inactive) - list of POOL_ENTRY */
		HashTable master_list;
		HashTable slave_list;

		/* Buffered selection of active connections: those MS shall use at the very
		 * moment for picking a server, those that belong to the currently used cluster.
		 * List of MS_LIST_DATA (as ever before)
		 */
		zend_llist active_master_list;
		zend_llist active_slave_list;

	} data;

	/* public methods */

	/* From an MS point of view: flush all connections, clear all slaves and masters
	 * The connections will not necessarily be closed but marked inactive.
	 * Assume Fabric constantly jumps between two shard groups, if we closed all
	 * connections on flush, we would have to open them the very next minute again.
	 * Or, let there be an XA transaction going on and Fabric flushes all connections
	 * including the ones of the XA transaction...
	 */
	enum_func_status (*flush)(struct st_mysqlnd_pool * pool TSRMLS_DC);

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

	/* Test whether a connection is already in the pool */
	zend_bool (*connection_exists)(struct st_mysqlnd_pool * pool, smart_str * hash_key,
								  zend_bool * is_master, zend_bool * is_active TSRMLS_DC);

	/* Get list of active connections. Active connections belong to the
	 * currently selected cluster (e.g. shard group).
	 * TODO: returning zend_llist - can we prevent the user from changing llist? */
	zend_llist * (*get_active_masters)(struct st_mysqlnd_pool * pool TSRMLS_DC);
	zend_llist * (*get_active_slaves)(struct st_mysqlnd_pool * pool TSRMLS_DC);

	/* Reference counting for connections. Should a module require a
	 * connection to stay open, it can set a reference */
	enum_func_status (*add_reference)(struct st_mysqlnd_pool * pool,
									  MYSQLND_MS_CONN_DATA * conn TSRMLS_DC);
	enum_func_status (*free_reference)(struct st_mysqlnd_pool * pool,
										 MYSQLND_MS_CONN_DATA * conn TSRMLS_DC);

} MYSQLND_MS_POOL;


MYSQLND_MS_POOL * mysqlnd_ms_pool_ctor(llist_dtor_func_t ms_list_data_dtor, zend_bool persistent TSRMLS_DC);
void mysqlnd_ms_pool_dtor(MYSQLND_MS_POOL * pool TSRMLS_DC);


typedef struct st_mysqlnd_pool_entry {
	/* This is the data we keep on behalf of the MS core */
	MYSQLND_MS_LIST_DATA * data;
	/* dtor the MS core provides to free LIST_DATA member */
	/* TODO: can we simplify the pointer logic, one level less? */
	llist_dtor_func_t ms_list_data_dtor;

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
