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
  | Author: Johannes Schlueter <johannes@php.net>                        |
  +----------------------------------------------------------------------+
*/

#include <stddef.h>
#include <alloca.h>
#include <strings.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

#include "mysqlnd_fabric.h"
#include "mysqlnd_fabric_priv.h"

#include "Zend/zend.h"
#include "Zend/zend_API.h"
#include "Zend/zend_alloc.h"
#include "Zend/zend_interfaces.h"

/*
    When allocated we allocate one buffer for all four tables so we have one
    continues block of memory which can be cached easily, see commented
    members and  tmp_fabric_dump as example for the full structure.
    For accessing all elements fabric_dump_index is being used which points
    at the actual elements. index->shard_table == raw.shard_table
 */
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

/* This variable is a sample and has no further sense */
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


static void fabric_create_index(fabric_dump_index *index, const fabric_dump_raw *fabric_dump)
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

int fill_shard_table_entry(void *pDest TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key) 
{
	zval *data;
	HashTable *source = Z_ARRVAL_P((zval*)pDest);
	mysqlnd_fabric_shard_table *target;
	zend_bool *success;
	
	if (num_args != 2) {
		/* This should never ever happen */
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Wrong number of arguments to internal fill_shard_table_entry from zend_hash_apply call");
	}
	
	target =  va_arg(args, mysqlnd_fabric_shard_table *);
	success = va_arg(args, zend_bool *); /* Will be false by default, only change on success */
	
	
	zend_hash_internal_pointer_reset(source);
	zend_hash_get_current_data(source, (void**)&data);
	if (Z_TYPE_P(data) != IS_STRING || Z_STRLEN_P(data) + 1 > sizeof(target->schema_name)) {
		return ZEND_HASH_APPLY_STOP;
	}
	memcpy(target->schema_name, Z_STRVAL_P(data), Z_STRLEN_P(data) + 1);
	
	zend_hash_move_forward(source);
	zend_hash_get_current_data(source, (void**)&data);
	if (Z_TYPE_P(data) != IS_STRING || Z_STRLEN_P(data) + 1 > sizeof(target->table_name)) {
		return ZEND_HASH_APPLY_STOP;
	}
	memcpy(target->table_name, Z_STRVAL_P(data), Z_STRLEN_P(data) + 1);
	
	zend_hash_move_forward(source);
	zend_hash_get_current_data(source, (void**)&data);
	if (Z_TYPE_P(data) != IS_STRING || Z_STRLEN_P(data) + 1 > sizeof(target->column_name)) {
		return ZEND_HASH_APPLY_STOP;
	}
	memcpy(target->column_name, Z_STRVAL_P(data), Z_STRLEN_P(data) + 1);
	
	zend_hash_move_forward(source);
	convert_to_long(data);
	target->shard_mapping_id = Z_LVAL_P(data);
	
	*success = 1;
	return ZEND_HASH_APPLY_KEEP;
}

int fill_shard_mapping_entry(void *pDest TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key) 
{
	zval *data;
	HashTable *source = Z_ARRVAL_P((zval*)pDest);
	mysqlnd_fabric_shard_mapping *target;
	zend_bool *success;
	
	if (num_args != 2) {
		/* This should never ever happen */
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Wrong number of arguments to internal fill_shard_mapping_entry from zend_hash_apply call");
	}
	
	target =  va_arg(args, mysqlnd_fabric_shard_mapping *);
	success = va_arg(args, zend_bool *); /* Will be false by default, only change on success */
	
	zend_hash_internal_pointer_reset(source);
	zend_hash_get_current_data(source, (void**)&data);
	convert_to_long(data);
	target->shard_mapping_id = Z_LVAL_P(data);
	
	zend_hash_move_forward(source);
	zend_hash_get_current_data(source, (void**)&data);
	if (Z_TYPE_P(data) != IS_STRING || Z_STRLEN_P(data) != sizeof("RANGE")-1 || strcmp("RANGE", Z_STRVAL_P(data))) {
		return ZEND_HASH_APPLY_STOP;
	}
	target->type_name = RANGE;
	
	zend_hash_move_forward(source);
	zend_hash_get_current_data(source, (void**)&data);
	if (Z_TYPE_P(data) != IS_STRING || Z_STRLEN_P(data) + 1 > sizeof(target->global_group)) {
		return ZEND_HASH_APPLY_STOP;
	}
	memcpy(target->global_group, Z_STRVAL_P(data), Z_STRLEN_P(data) + 1);
	
	*success = 1;
	return ZEND_HASH_APPLY_KEEP;
}

int fill_shard_index_entry(void *pDest, void *argument TSRMLS_DC) {}
int fill_server_entry(void *pDest, void *argument TSRMLS_DC) {}

void fabric_set_raw_data_from_xmlstr(mysqlnd_fabric *fabric,
	const char *shard_table_xml, size_t shard_table_len,
	const char *shard_mapping_xml, size_t shard_mapping_len,
	const char *shard_index_xml, size_t shard_index_len,
	const char *server_xml, size_t server_len)
{
	zend_function *func_cache;
	zval *z_shard_table, *z_shard_mapping, *z_shard_index, *z_server;
	zval arg;
	zval *tmp;
	
	int shard_table_count;
	int shard_mapping_count;
	int shard_index_count;
	int server_count;
	long pos;
	void *vpos;
	zend_bool success = 0;
	
	fabric_dump_data *dump_data = (fabric_dump_data*)fabric->strategy_data;
	
	/* TODO: We might be more graceful on errors instead of bailing out, with
	 * this way a failing Fabric might shutdown everything */
	
	/* 1. Parse all XML files */
		
	ALLOC_INIT_ZVAL(tmp)
		
	INIT_ZVAL(arg);
	ZVAL_STRINGL(&arg, shard_table_xml, shard_table_len, 0);
	zend_call_method_with_1_params(NULL, NULL, &func_cache, "xmlrpc_decode", &tmp, &arg);
	if (Z_TYPE_P(tmp) != IS_ARRAY) {
		zval_dtor(tmp);
		php_error_docref(NULL, E_ERROR, "Failed to decode  XML-RPC response while handling shard_table");
	}
	if (zend_hash_index_find(Z_ARRVAL_P(tmp), 3, (void**)&z_shard_table) == FAILURE) {
		zval_dtor(tmp);
		php_error_docref(NULL, E_ERROR, "Invalid response from XML-RPC while handling shard_table");
	}
	Z_ADDREF_P(z_shard_table);
	zval_dtor(tmp);

	INIT_ZVAL(arg);
	ZVAL_STRINGL(&arg, shard_mapping_xml, shard_mapping_len, 0);
	zend_call_method_with_1_params(NULL, NULL, &func_cache, "xmlrpc_decode", &tmp, &arg);
		if (Z_TYPE_P(tmp) != IS_ARRAY) {
		zval_dtor(tmp);
		zval_dtor(z_shard_table);
		php_error_docref(NULL, E_ERROR, "Failed to decode  XML-RPC response while handling shard_mapping");
	}
	if (zend_hash_index_find(Z_ARRVAL_P(tmp), 3, (void**)&z_shard_mapping) == FAILURE) {
		zval_dtor(tmp);
		zval_dtor(z_shard_table);
		php_error_docref(NULL, E_ERROR, "Invalid response from XML-RPC while handling shard_mapping");
	}
	Z_ADDREF_P(z_shard_mapping);
	zval_dtor(tmp);
	
	INIT_ZVAL(arg);
	ZVAL_STRINGL(&arg, shard_index_xml, shard_index_len, 0);
	zend_call_method_with_1_params(NULL, NULL, &func_cache, "xmlrpc_decode", &tmp, &arg);
	zend_call_method_with_1_params(NULL, NULL, &func_cache, "xmlrpc_decode", &tmp, &arg);
	if (Z_TYPE_P(tmp) != IS_ARRAY) {
		zval_dtor(tmp);
		zval_dtor(z_shard_table);
		zval_dtor(z_shard_mapping);
		php_error_docref(NULL, E_ERROR, "Failed to decode  XML-RPC response while handling shard_index");
	}
	if (zend_hash_index_find(Z_ARRVAL_P(tmp), 3, (void**)&z_shard_index) == FAILURE) {
		zval_dtor(tmp);
		zval_dtor(z_shard_table);
		zval_dtor(z_shard_mapping);
		php_error_docref(NULL, E_ERROR, "Invalid response from XML-RPC while handling shard_index");
	}
	Z_ADDREF_P(z_shard_index);
	zval_dtor(tmp);

	INIT_ZVAL(arg);
	ZVAL_STRINGL(&arg, server_xml, server_len, 0);
	zend_call_method_with_1_params(NULL, NULL, &func_cache, "xmlrpc_decode", &tmp, &arg);
	if (Z_TYPE_P(tmp) != IS_ARRAY) {
		zval_dtor(tmp);
		zval_dtor(z_shard_table);
		zval_dtor(z_shard_mapping);
		zval_dtor(z_shard_index);
		php_error_docref(NULL, E_ERROR, "Failed to decode  XML-RPC response while handling server list");
	}
	if (zend_hash_index_find(Z_ARRVAL_P(tmp), 3, (void**)&z_shard_mapping) == FAILURE) {
		zval_dtor(tmp);
		zval_dtor(z_shard_table);
		zval_dtor(z_shard_mapping);
		zval_dtor(z_shard_index);
		php_error_docref(NULL, E_ERROR, "Invalid response from XML-RPC while handling server list");
	}
	zend_hash_index_find(Z_ARRVAL_P(tmp), 3, (void**)&z_server);
	Z_ADDREF_P(z_server);
	zval_dtor(tmp);
	
	/* 2. Check all files for their number of elements */
	shard_table_count = zend_hash_num_elements(Z_ARRVAL_P(z_shard_table));
	shard_mapping_count = zend_hash_num_elements(Z_ARRVAL_P(z_shard_mapping));
	shard_index_count = zend_hash_num_elements(Z_ARRVAL_P(z_shard_index));
	server_count = zend_hash_num_elements(Z_ARRVAL_P(z_server));
	
	/* 3. Allocate buffer and create index */
	
	dump_data->raw = (fabric_dump_raw*)emalloc(
		  shard_table_count * sizeof(mysqlnd_fabric_shard_table)
		+ shard_mapping_count * sizeof(mysqlnd_fabric_shard_mapping)
		+ shard_index_count * sizeof(mysqlnd_fabric_shard_index)
		+ server_count * sizeof(mysqlnd_fabric_server));
	dump_data->raw->shard_table_count = shard_table_count;
	
	dump_data->index.shard_table_count = shard_table_count;
	dump_data->index.shard_table = dump_data->raw->shard_table;
	
	pos = (size_t)dump_data->raw->shard_table + (shard_table_count * sizeof(mysqlnd_fabric_shard_table));
	dump_data->index.shard_mapping_count = *(int*)pos = shard_mapping_count;
	
	pos += sizeof(int);
	dump_data->index.shard_mapping = (mysqlnd_fabric_shard_mapping *)pos;
	
	pos += shard_mapping_count * sizeof(mysqlnd_fabric_shard_mapping);
	dump_data->index.shard_index_count = *(int*)pos = shard_index_count;
	
	pos += sizeof(int);
	dump_data->index.shard_index = (mysqlnd_fabric_shard_index *)pos;
	
	pos += shard_index_count * sizeof(mysqlnd_fabric_shard_index);
	dump_data->index.server_count = *(int*)pos = shard_index_count;
	
	pos += sizeof(int);
	dump_data->index.server = (mysqlnd_fabric_server *)pos;
	
	/* 4. fill buffer */
	
	vpos = (void*)dump_data->index.shard_table;
	zend_hash_apply_with_arguments(Z_ARRVAL_P(z_shard_table), (apply_func_args_t)fill_shard_table_entry, 2, &vpos, &success);
	if (success == 0) {
		goto cleanup;
	}

	vpos = (void*)dump_data->index.shard_mapping;
	zend_hash_apply_with_arguments(Z_ARRVAL_P(z_shard_mapping), (apply_func_args_t)fill_shard_mapping_entry, 2, &vpos, &success);
	if (success == 0) {
		goto cleanup;
	}

	vpos = (void*)dump_data->index.shard_index;
	zend_hash_apply_with_arguments(Z_ARRVAL_P(z_shard_index), (apply_func_args_t)fill_shard_index_entry, 2, &vpos, &success);
	if (success == 0) {
		goto cleanup;
	}

	vpos = (void*)dump_data->index.server;
	zend_hash_apply_with_arguments(Z_ARRVAL_P(z_server), (apply_func_args_t)fill_server_entry, 2, &vpos, &success);
	if (success == 0) {
		goto cleanup;
	}

	
	/* 5. clean up */
cleanup:	
	zval_ptr_dtor(&z_shard_table);
	zval_ptr_dtor(&z_shard_mapping);
	zval_ptr_dtor(&z_shard_index);
	zval_ptr_dtor(&z_server);
	
	if (success == 0) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Received invalid data from Fabric");
	}
}

#define FABRIC_LOOKUP_XML "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n" \
			"<methodCall><methodName>%s</methodName><params></params></methodCall>"

void fabric_set_raw_data_from_fabric(mysqlnd_fabric *fabric)
{
	char *mysqlnd_fabric_http(mysqlnd_fabric *fabric, char *url, char *request_body, size_t request_body_len, size_t *response_len);
	
	char *request;
	size_t request_len;
}

static void fabric_set_raw_data_for_gdb(mysqlnd_fabric *fabric)
{
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
		if (   strlen(index->shard_table[i].schema_name) == table-schema-1
			&& strlen(index->shard_table[i].table_name) == table_len - (table-schema)
			&& !strncmp(index->shard_table[i].schema_name, schema, table-schema-1)
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
	size_t group_len = strlen(group);
	const fabric_dump_index *index = &((const fabric_dump_data*)fabric->strategy_data)->index;
	mysqlnd_fabric_server *retval = safe_emalloc(10, sizeof(mysqlnd_fabric_server), 0); /* FIXME!!!!! */
	memset(retval, 0, 10 * sizeof(mysqlnd_fabric_server));
	
	for (i = 0; i < index->server_count; ++i) {
		if (index->server[i].group_len == group_len && !strcmp(index->server[i].group, group)) {
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

static mysqlnd_fabric_server *mysqlnd_fabric_dump_get_group_servers(mysqlnd_fabric *fabric, const char *group)
{
	return mysqlnd_fabric_get_server_for_group(fabric, group);
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
	mysqlnd_fabric_dump_get_group_servers,
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
