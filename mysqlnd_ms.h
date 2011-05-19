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
#ifndef MYSQLND_MS_H
#define MYSQLND_MS_H

#define SMART_STR_START_SIZE 1024
#define SMART_STR_PREALLOC 256
#include "ext/standard/php_smart_str.h"

#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_statistics.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_priv.h"

#ifdef ZTS
#include "TSRM.h"
#endif

#define MYSQLND_MS_CONFIG_FORMAT "json"

ZEND_BEGIN_MODULE_GLOBALS(mysqlnd_ms)
	zend_bool enable;
	zend_bool force_config_usage;
	const char * ini_file;
	zval * user_pick_server;
	zend_bool collect_statistics;
ZEND_END_MODULE_GLOBALS(mysqlnd_ms)


#ifdef ZTS
#define MYSQLND_MS_G(v) TSRMG(mysqlnd_ms_globals_id, zend_mysqlnd_ms_globals *, v)
#else
#define MYSQLND_MS_G(v) (mysqlnd_ms_globals.v)
#endif

#define MASTER_SWITCH "ms=master"
#define SLAVE_SWITCH "ms=slave"
#define LAST_USED_SWITCH "ms=last_used"
#define ALL_SERVER_SWITCH "ms=all"

#define MYSQLND_MS_VERSION "1.0.2-alpha"
#define MYSQLND_MS_VERSION_ID 10002

#define MYSQLND_MS_ERROR_PREFIX "(mysqlnd_ms)"

extern MYSQLND_STATS * mysqlnd_ms_stats;


typedef enum mysqlnd_ms_collected_stats
{
	MS_STAT_USE_SLAVE,
	MS_STAT_USE_MASTER,
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
	MS_STAT_LAST /* Should be always the last */
} enum_mysqlnd_ms_collected_stats;

#define MYSQLND_MS_INC_STATISTIC(stat) MYSQLND_INC_STATISTIC(MYSQLND_MS_G(collect_statistics), mysqlnd_ms_stats, (stat))
#define MYSQLND_MS_INC_STATISTIC_W_VALUE(stat, value) MYSQLND_INC_STATISTIC_W_VALUE(MYSQLND_MS_G(collect_statistics), mysqlnd_ms_stats, (stat), (value))


/*
  ALREADY FIXED:
  Keep it false for now or we will have races in connect,
  where multiple instance can read the slave[] values and so
  move the pointer of each other. Need to find better implementation of
  hotloading.
  Maybe not use `hotloading? FALSE:TRUE` but an expclicit lock around
  the array extraction of master[] and slave[] and pass FALSE to
  mysqlnd_ms_json_config_string(), meaning it should not try to get a lock.
*/
#define MYSLQND_MS_HOTLOADING FALSE

enum enum_which_server
{
	USE_MASTER,
	USE_SLAVE,
	USE_LAST_USED,
	USE_ALL
};

extern unsigned int mysqlnd_ms_plugin_id;
extern struct st_mysqlnd_ms_json_config * mysqlnd_ms_json_config;
ZEND_EXTERN_MODULE_GLOBALS(mysqlnd_ms)


void mysqlnd_ms_register_hooks();
void mysqlnd_ms_conn_list_dtor(void * pDest);


struct st_qc_token_and_value
{
	unsigned int token;
	zval value;
};


struct st_mysqlnd_query_scanner
{
	void * scanner;
	zval * token_value;
};

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
	STATEMENT_DROP
} enum_mysql_statement_type;

struct st_mysqlnd_parse_info
{
	char * db;
	char * table;
	char * org_table;
	enum_mysql_statement_type statement;
	zend_bool persistent;
};

struct st_mysqlnd_query_parser
{
	struct st_mysqlnd_query_scanner * scanner;
	struct st_mysqlnd_parse_info parse_info;
};


#endif	/* MYSQLND_MS_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
