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
#include "zend.h"
#include "zend_alloc.h"

#include "mysqlnd_fabric.h"
#include "mysqlnd_fabric_priv.h"

#define FABRIC_GROUP_LOOKUP_XML "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n" \
			"<methodCall><methodName>sharding.lookup_servers</methodName><params>\n" \
			"<param><!-- group --><value><string>%s</string></value></param></params>\n" \
			"</methodCall>"

#define FABRIC_SHARD_LOOKUP_XML "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n" \
			"<methodCall><methodName>sharding.lookup_servers</methodName><params>\n" \
			"<param><!-- table --><value><string>%s</string></value></param>\n" \
			"<param><!-- shard key --><value><string>%s</string></value></param>\n" \
			"<param><!-- hint --><value><string>%s</string></value></param>\n" \
			"<param><!-- sync --><value><boolean>1</boolean></value></param></params>\n" \
			"</methodCall>"

static void mysqlnd_fabric_host_shuffle(mysqlnd_fabric_rpc_host *a, size_t n)
{
	size_t i;
	
	if (n == 1) {
		return;
	}

	for (i = 0; i < n - 1; i++)  {
		size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
		mysqlnd_fabric_rpc_host t = a[j];

		a[j] = a[i];
		a[i] = t;
	}
}

static mysqlnd_fabric_server *mysqlnd_fabric_direct_do_request(mysqlnd_fabric *fabric, char *req, size_t req_len)
{
	size_t len = 0;
	char *response;
	mysqlnd_fabric_rpc_host *server;

	mysqlnd_fabric_host_shuffle(fabric->hosts, fabric->host_count);
	for (server = fabric->hosts; !len && server < fabric->hosts  + fabric->host_count; server++) {
		/* TODO: Switch to quiet mode */
        response = mysqlnd_fabric_http(fabric, server->url, req, req_len, &len);
	};

	efree(req);
	
	return mysqlnd_fabric_parse_xml(fabric, response, len);
}

static mysqlnd_fabric_server *mysqlnd_fabric_direct_get_group_servers(mysqlnd_fabric *fabric, const char *group)
{
	mysqlnd_fabric_server *retval;
	char *req = NULL;
	size_t req_len;

	req_len = spprintf(&req, 0, FABRIC_GROUP_LOOKUP_XML, group);
	retval = mysqlnd_fabric_direct_do_request(fabric, req, req_len);
	efree(req);

	return retval;
}

static mysqlnd_fabric_server *mysqlnd_fabric_direct_get_shard_servers(mysqlnd_fabric *fabric, const char *table, const char *key, enum mysqlnd_fabric_hint hint)
{
	mysqlnd_fabric_server *retval;
	char *req = NULL;
	size_t req_len;

	req_len = spprintf(&req, 0, FABRIC_SHARD_LOOKUP_XML, table, key ? key : "", hint == LOCAL ? "LOCAL" : "GLOBAL");
	retval = mysqlnd_fabric_direct_do_request(fabric, req, req_len);
	efree(req);

	return retval;
}

const myslqnd_fabric_strategy mysqlnd_fabric_strategy_direct = {
	NULL, /* init */
	NULL, /* deinit */
	mysqlnd_fabric_direct_get_group_servers,
	mysqlnd_fabric_direct_get_shard_servers
};

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
