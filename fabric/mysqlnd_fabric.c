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

#define FABRIC_SHARD_LOOKUP_XML "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n" \
			"<methodCall><methodName>sharding.lookup_servers</methodName><params>\n" \
			"<param><!-- table --><value><string>%s</string></value></param>\n" \
			"<param><!-- shard key --><value><string>%s</string></value></param>\n" \
			"<param><!-- hint --><value><string>%s</string></value></param>\n" \
			"<param><!-- sync --><value><boolean>1</boolean></value></param></params>\n" \
			"</methodCall>"

mysqlnd_fabric *mysqlnd_fabric_init()
{
	mysqlnd_fabric *fabric = ecalloc(1, sizeof(mysqlnd_fabric));
	return fabric;
}

void mysqlnd_fabric_free(mysqlnd_fabric *fabric)
{
	int i;
	for (i = 0; i < fabric->host_count ; ++i) {
		efree(fabric->hosts[i].hostname);
	}
	efree(fabric);
}

int mysqlnd_fabric_add_host(mysqlnd_fabric *fabric, char *hostname, int port)
{
	if (fabric->host_count >= 10) {
		return 1;
	}
	
	fabric->hosts[fabric->host_count].hostname = estrdup(hostname);
	fabric->hosts[fabric->host_count].port = port;
	fabric->host_count++;	
	
	return 0;
}

mysqlnd_fabric_server *mysqlnd_fabric_get_shard_servers(mysqlnd_fabric *fabric, const char *table, const char *key, enum mysqlnd_fabric_hint hint)
{
	char *url = NULL, *req = NULL;
	char foo[4001];
	TSRMLS_FETCH();
	
	spprintf(&url, 0, "http://%s:%d/", fabric->hosts[0].hostname, fabric->hosts[0].port);
	
	spprintf(&req, 0, FABRIC_SHARD_LOOKUP_XML, table, key, hint == LOCAL ? "LOCAL" : "GLOBAL");
//	printf("%s", req);
	
	php_stream_context *ctxt = php_stream_context_alloc(TSRMLS_C);
	zval method, content;
	ZVAL_STRING(&method, "POST", 0);
	ZVAL_STRING(&content, req, 0);
	
	/* prevent anybody from freeing these */
	Z_SET_ISREF(method);
	Z_SET_ISREF(content);
	Z_SET_REFCOUNT(method, 2);
	Z_SET_REFCOUNT(content, 2);
	
	php_stream_context_set_option(ctxt, "http", "method", &method);
	php_stream_context_set_option(ctxt, "http", "content", &content);
	
	php_stream *stream = php_stream_open_wrapper_ex(url, "rb", REPORT_ERRORS, NULL, ctxt);
	if (!stream) {
		efree(url);
		efree(req);
		
		return NULL;
	}
	
	int len = php_stream_read(stream, foo, 4000);
	foo[len] = '\0';
	/* what happens with a response > 4000 bytes ... needs to be handled once we have the dump API */

	php_stream_close(stream);
	
	efree(url);
	efree(req);
	
	return mysqlnd_fabric_parse_xml(foo, len);
}

void mysqlnd_fabric_free_server_list(mysqlnd_fabric_server *servers)
{
	mysqlnd_fabric_server *pos;
	
	if (!servers) {
		return;
	}
	
	for (pos = servers; pos->hostname; pos++) {
		efree(pos->hostname);
		efree(pos->uuid);
	}
	efree(servers);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
