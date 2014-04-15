/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2013 The PHP Group                                |
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

#include "zend.h"
#include "zend_alloc.h"
#include "main/php.h"
#include "main/spprintf.h"
#include "main/php_streams.h"

#include "mysqlnd_fabric.h"
#include "mysqlnd_fabric_priv.h"

static const struct fabric_dump {
	int shard_table_count;
	mysqlnd_fabric_shard_table shard_table[2];
	int shard_mapping_count;
	mysqlnd_fabric_shard_mapping shard_mapping[2];
	int shard_index_count;
	mysqlnd_fabric_shard_index shard_index[4];
	int server_count;
	mysqlnd_fabric_server server[10];
} fabric_dump = {
	2,
	{
		{ 1, "test", "fabric", "id" },
		{ 2, "foo", "bar", "id" }
	},
	2,
	{
		{ 1, RANGE, "global1" },
		{ 2, RANGE, "global2" },
	},
	4,
	{
		{ 1,    1, 1, "shard1_1" },
		{ 1, 1000, 2, "shard1_2" },
		{ 2,    1, 3, "shard2_1" },
		{ 2, 1000, 4, "shard2_2" }
	},
	10,
	{
		{ 9, "0000-0001", 7, "global1",  9, "localhost", 3301, 3, 3, 1.0 },
		{ 9, "0000-0002", 7, "global2",  9, "localhost", 3302, 3, 3, 1.0 },
		{ 9, "0000-0003", 7, "shard1_1", 9, "localhost", 3303, 3, 3, 1.0 },
		{ 9, "0000-0004", 7, "shard1_1", 9, "localhost", 3304, 1, 2, 1.0 },
		{ 9, "0000-0005", 7, "shard1_2", 9, "localhost", 3305, 3, 3, 1.0 },
		{ 9, "0000-0006", 7, "shard1_2", 9, "localhost", 3306, 1, 2, 1.0 },
		{ 9, "0000-0007", 7, "shard2_1", 9, "localhost", 3307, 3, 3, 1.0 },
		{ 9, "0000-0008", 7, "shard2_1", 9, "localhost", 3308, 1, 2, 1.0 },
		{ 9, "0000-0009", 7, "shard2_2", 9, "localhost", 3309, 3, 3, 1.0 },
		{ 9, "0000-0010", 7, "shard2_2", 9, "localhost", 3310, 1, 2, 1.0 }
	}
};


static mysqlnd_fabric_server *mysqlnd_fabric_dump_get_shard_servers(mysqlnd_fabric *fabric, const char *table, const char *key, enum mysqlnd_fabric_hint hint)
{
	
}

const myslqnd_fabric_strategy mysqlnd_fabric_strategy_dump = {
	NULL, /* init */
	NULL, /* deinit */
	mysqlnd_fabric_dump_get_shard_servers
};

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
