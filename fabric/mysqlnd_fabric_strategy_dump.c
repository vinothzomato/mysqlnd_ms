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

#include <stddef.h>
#include <alloca.h>
#include <strings.h>

#include "mysqlnd_fabric.h"
#include "mysqlnd_fabric_priv.h"

typedef struct {
	int shard_table_count;
	mysqlnd_fabric_shard_table shard_table[];
	/*
	int shard_mapping_count;
	mysqlnd_fabric_shard_mapping shard_mapping[];
	int shard_index_count;
	mysqlnd_fabric_shard_index shard_index[];
	int server_count;
	mysqlnd_fabric_server server[];
	*/
} fabric_dump_raw;

static const struct {
	int shard_table_count;
	mysqlnd_fabric_shard_table shard_table[2];
	int shard_mapping_count;
	mysqlnd_fabric_shard_mapping shard_mapping[2];
	int shard_index_count;
	mysqlnd_fabric_shard_index shard_index[5];
	int server_count;
	mysqlnd_fabric_server server[12];
} tmp_fabric_dump = {
	2,
	{
		{ 1, "test", "fabrictest", "id" },
		{ 2, "foo", "bar", "id" }
	},
	2,
	{
		{ 1, RANGE, "global1" },
		{ 2, RANGE, "global2" },
	},
	5,
	{	
		{ 1,     1, 1, "shard1_1" },
		{ 1, 30000, 2, "shard1_2" },
		{ 1, 40000, 2, "shard1_3" },
		{ 2,     1, 3, "shard2_1" },
		{ 2,  1000, 4, "shard2_2" }
	},
	12,
	{
		{ 9, "0000-0001", 7, "global1",  9, "localhost", 3301, 3, 3, 1.0 },
		{ 9, "0000-0002", 7, "global2",  9, "localhost", 3302, 3, 3, 1.0 },
		{ 9, "0000-0003", 8, "shard1_1", 9, "localhost", 3303, 3, 3, 1.0 },
		{ 9, "0000-0004", 8, "shard1_1", 9, "localhost", 3304, 1, 2, 1.0 },
		{ 9, "0000-0005", 8, "shard1_2", 9, "localhost", 3305, 3, 3, 1.0 },
		{ 9, "0000-0006", 8, "shard1_2", 9, "localhost", 3306, 1, 2, 1.0 },
		{ 9, "0000-0007", 8, "shard1_3", 9, "localhost", 3307, 3, 3, 1.0 },
		{ 9, "0000-0008", 8, "shard1_3", 9, "localhost", 3308, 1, 2, 1.0 },
		{ 9, "0000-0009", 8, "shard2_1", 9, "localhost", 3309, 3, 3, 1.0 },
		{ 9, "0000-0010", 8, "shard2_1", 9, "localhost", 3310, 1, 2, 1.0 },
		{ 9, "0000-0011", 8, "shard2_2", 9, "localhost", 3311, 3, 3, 1.0 },
		{ 9, "0000-0012", 8, "shard2_2", 9, "localhost", 3312, 1, 2, 1.0 }
	}
};


typedef struct {
	int shard_table_count;
	const mysqlnd_fabric_shard_table *shard_table;
	int shard_mapping_count;
	const mysqlnd_fabric_shard_mapping *shard_mapping;
	int shard_index_count;
	const mysqlnd_fabric_shard_index *shard_index;
	int server_count;
	const mysqlnd_fabric_server *server;	
} fabric_dump_index;

typedef struct {
	fabric_dump_raw *raw;
	fabric_dump_index index;
} fabric_dump_data;

/*******************
 * Data processing *
 *******************/


static fabric_create_index(fabric_dump_index *index, const fabric_dump_raw *fabric_dump)
{
	size_t pos;
	index->shard_table_count = fabric_dump->shard_table_count;
	index->shard_table = fabric_dump->shard_table;
	
	pos = (size_t)fabric_dump->shard_table + (index->shard_table_count * sizeof(mysqlnd_fabric_shard_table));
	index->shard_mapping_count = *(int*)pos;
	
	pos += sizeof(int);
	index->shard_mapping = (mysqlnd_fabric_shard_mapping *)pos;
	
	pos += index->shard_mapping_count * sizeof(mysqlnd_fabric_shard_mapping);
	index->shard_index_count = *(int*)pos;
	
	pos += sizeof(int);
	index->shard_index = (mysqlnd_fabric_shard_index *)pos;
	
	pos += index->shard_index_count * sizeof(mysqlnd_fabric_shard_index);
	index->server_count = *(int*)pos;
	
	pos += sizeof(int);
	index->server = (mysqlnd_fabric_server *)pos;
}

/* TODO: Do we have to copy the data or how do we manage ownership? */
void fabric_set_raw_data(mysqlnd_fabric *fabric, char *data, size_t data_len)
{
	fabric_dump_data *dump_data = (fabric_dump_data*)fabric->strategy_data;
	dump_data->raw = (fabric_dump_raw*)estrndup(data, data_len);
	fabric_create_index(&dump_data->index, dump_data->raw);
}

static void fabric_set_raw_data_for_gdb(mysqlnd_fabric *fabric) {
	/* TODO: Remove me. I'm only used during development from inside gdb sessions */
	fabric_set_raw_data(fabric, (char *)&tmp_fabric_dump, sizeof(tmp_fabric_dump));
}

/**********
 * Lookup *
 **********/

static int mysqlnd_fabric_get_shard_for_table(const mysqlnd_fabric *fabric, const char *table_i, size_t table_len)
{
	int i;
	char *table;
	const fabric_dump_index *index = &((const fabric_dump_data*)fabric->strategy_data)->index;
	char *schema = alloca(table_len+1);
	strlcpy(schema, table_i, table_len+1);
	
	table = strstr(schema, ".");
	if (!table) {
		return 0;
	}
	
	*table = '\0';
	table++;
	
	for (i = 0; i < index->shard_table_count; ++i) {
		if (   !strncmp(index->shard_table[i].schema_name, schema, table-schema-1)
			&& !strncmp(index->shard_table[i].table_name,  table, table_len - (table-schema)))
		{
			return index->shard_table[i].shard_mapping_id;
		}
	}
	
	return 0;
}

static const char *mysqlnd_fabric_get_global_group(const mysqlnd_fabric *fabric, int shard_mapping_id)
{
	int i;
	const fabric_dump_index *index = &((const fabric_dump_data*)fabric->strategy_data)->index;
	
	for (i = 0; i < index->shard_mapping_count; ++i) {
		if (index->shard_mapping[i].shard_mapping_id == shard_mapping_id) {
			return index->shard_mapping[i].global_group;
		}
	}
	return NULL;
}

static const char *mysqlnd_fabric_get_shard_group_for_key(const mysqlnd_fabric *fabric, int shard_mapping_id, int key)
{
	int i;
	const fabric_dump_index *index = &((const fabric_dump_data*)fabric->strategy_data)->index;
	const char *retval = NULL;
	for (i = 0; i < index->shard_index_count; ++i) {
		if (index->shard_index[i].shard_mapping_id == shard_mapping_id) {
			/* This assumes we have our data sorted by shard key, this is guaranteed by fabric */
			if (index->shard_index[i].lower_bound <= key) {
				retval = index->shard_index[i].group;
			}
		}
	}
	return retval;	
}

static mysqlnd_fabric_server *mysqlnd_fabric_get_server_for_group(mysqlnd_fabric *fabric, const char *group)
{
	int i, count = 0;
	const fabric_dump_index *index = &((const fabric_dump_data*)fabric->strategy_data)->index;
	mysqlnd_fabric_server *retval = safe_emalloc(10, sizeof(mysqlnd_fabric_server), 0); /* FIXME!!!!! */
	memset(retval, 0, 10 * sizeof(mysqlnd_fabric_server));
	
	for (i = 0; i < index->server_count; ++i) {
		if (!strcmp(index->server[i].group, group)) {
			memcpy(&retval[count++], &index->server[i], sizeof(mysqlnd_fabric_server));
		}
	}
	
	return retval;
}

/******************
 * Infrastructure *
 ******************/

static void fabric_dump_init(mysqlnd_fabric *fabric)
{
	fabric->strategy_data = ecalloc(sizeof(fabric_dump_data), 1);
}

static void fabric_dump_deinit(mysqlnd_fabric *fabric)
{
	if (((fabric_dump_data*)fabric->strategy_data)->raw) {
		/* TODO: See fabric_set_raw_data() for ownership question */
		efree(((fabric_dump_data*)fabric->strategy_data)->raw);
	}
	efree(fabric->strategy_data);
}

static mysqlnd_fabric_server *mysqlnd_fabric_dump_get_shard_servers(mysqlnd_fabric *fabric, const char *table, const char *key, enum mysqlnd_fabric_hint hint)
{
	const char *group = NULL;
	int shard_mapping_id = mysqlnd_fabric_get_shard_for_table(fabric, table, strlen(table));
	if (!shard_mapping_id) {
		return NULL;
	}
	
	switch (hint) {
	case GLOBAL:
		group = mysqlnd_fabric_get_global_group(fabric, shard_mapping_id);
		break;
	case LOCAL:
		{
			int key_i = atoi(key);
			group = mysqlnd_fabric_get_shard_group_for_key(fabric, shard_mapping_id, key_i);
			break;
		}
	}

	if (!group) {
		return;
	}
	
	return mysqlnd_fabric_get_server_for_group(fabric, group);
}

const myslqnd_fabric_strategy mysqlnd_fabric_strategy_dump = {
	fabric_dump_init,
	fabric_dump_deinit,
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
