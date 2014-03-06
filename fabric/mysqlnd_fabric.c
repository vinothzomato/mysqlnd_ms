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

#include "ext/standard/php_rand.h"

#include "ext/standard/php_smart_str.h"

#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_priv.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "mysqlnd_ms_enum_n_def.h"
#if PHP_VERSION_ID >= 50400
#include "ext/mysqlnd/mysqlnd_ext_plugin.h"
#endif
#include "mysqlnd_ms.h"

#include "mysqlnd_fabric.h"
#include "mysqlnd_fabric_priv.h"

#define FABRIC_SHARDING_LOOKUP_SERVERS_XML "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n" \
			"<methodCall><methodName>sharding.lookup_servers</methodName><params>\n" \
			"<param><!-- table --><value><string>%s</string></value></param>\n" \
			"<param><!-- shard key --><value><string>%s</string></value></param>\n" \
			"<param><!-- hint --><value><string>%s</string></value></param>\n" \
			"</params>\n" \
			"</methodCall>"

static void mysqlnd_fabric_host_shuffle(MYSQLND_MS_FABRIC_HOST *a, size_t n)
{
	size_t i;

	if (n == 1) {
		return;
	}

	for (i = 0; i < n - 1; i++)  {
		size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
		MYSQLND_MS_FABRIC_HOST t = a[j];

		a[j] = a[i];
		a[i] = t;
	}
}

MYSQLND_MS_FABRIC *mysqlnd_fabric_init()
{
	MYSQLND_MS_FABRIC *fabric = ecalloc(1, sizeof(MYSQLND_MS_FABRIC));
	return fabric;
}

void mysqlnd_fabric_free(MYSQLND_MS_FABRIC *fabric)
{
	int i;
	for (i = 0; i < fabric->host_count ; ++i) {
		efree(fabric->hosts[i].hostname);
	}
	efree(fabric);
}

int mysqlnd_fabric_add_host(MYSQLND_MS_FABRIC *fabric, char *hostname, int port TSRMLS_DC)
{
	if (fabric->host_count >= 10) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, MYSQLND_MS_ERROR_PREFIX " Please report a bug: no more than 10 Fabric hosts allowed");
		return 1;
	}

	fabric->hosts[fabric->host_count].hostname = estrdup(hostname);
	fabric->hosts[fabric->host_count].port = port;
	fabric->host_count++;

	return 0;
}

int mysqlnd_fabric_host_list_apply(const MYSQLND_MS_FABRIC *fabric, mysqlnd_fabric_apply_func cb, void *data)
{
	int i;
	for (i = 0; i < fabric->host_count; ++i) {
		cb(fabric->hosts[i].hostname, fabric->hosts[i].port, data);
	}
	return i;
}

static php_stream *mysqlnd_fabric_open_stream(MYSQLND_MS_FABRIC *fabric, char *request_body)
{
	zval method, content, header;
	php_stream_context *ctxt;
	php_stream *stream = NULL;
	MYSQLND_MS_FABRIC_HOST *server;
	TSRMLS_FETCH();

	ZVAL_STRING(&method, "POST", 0);
	ZVAL_STRING(&content, request_body, 0);
	ZVAL_STRING(&header, "Content-type: text/xml", 0);

	/* prevent anybody from freeing these */
	Z_SET_ISREF(method);
	Z_SET_ISREF(content);
	Z_SET_ISREF(header);
	Z_SET_REFCOUNT(method, 2);
	Z_SET_REFCOUNT(content, 2);
	Z_SET_REFCOUNT(header, 2);

	ctxt = php_stream_context_alloc(TSRMLS_C);
	php_stream_context_set_option(ctxt, "http", "method", &method);
	php_stream_context_set_option(ctxt, "http", "content", &content);
	php_stream_context_set_option(ctxt, "http", "header", &header);

	SET_EMPTY_FABRIC_ERROR(*fabric);

	mysqlnd_fabric_host_shuffle(fabric->hosts, fabric->host_count);
	for (server = fabric->hosts; !stream && server < fabric->hosts  + fabric->host_count; server++) {
		char *url = NULL;

		spprintf(&url, 0, "http://%s:%d/", server->hostname, server->port);

		/* TODO: Switch to quiet mode */
		stream = php_stream_open_wrapper_ex(url, "rb", REPORT_ERRORS, NULL, ctxt);
		efree(url);
	};

	return stream;
}

MYSQLND_MS_FABRIC_SERVER * mysqlnd_fabric_get_shard_servers(MYSQLND_MS_FABRIC *fabric, const char *table, const char *key, enum mysqlnd_ms_fabric_hint hint TSRMLS_DC)
{
	time_t fetch_time;
	int len;
	char *req = NULL;
	char buf[2048];
	php_stream *stream;
	MYSQLND_MS_FABRIC_SERVER *retval;
	smart_str xml_str = {0, 0, 0};

	DBG_ENTER("mysqlnd_fabric_get_shard_servers");
	DBG_INF_FMT("table=%s, key=%s", table, key);
	MYSQLND_MS_STATS_TIME_SET(fetch_time);


	if (!fabric->host_count) {
		DBG_INF("No hosts");
		MYSQLND_MS_INC_STATISTIC(MS_STAT_FABRIC_SHARDING_LOOKUP_SERVERS_FAILURE);
		MYSQLND_MS_STATS_TIME_DIFF(fetch_time);
		MYSQLND_MS_INC_STATISTIC_W_VALUE(MS_STAT_FABRIC_SHARDING_LOOKUP_SERVERS_TIME_TOTAL, (uint64_t)fetch_time);
		SET_FABRIC_ERROR(*fabric, 2000, "HY000", "No fabric hosts found.");
		DBG_RETURN(NULL);
	}

	spprintf(&req, 0, FABRIC_SHARDING_LOOKUP_SERVERS_XML, table, key ? key : "''", hint == LOCAL ? "LOCAL" : "GLOBAL");
	stream = mysqlnd_fabric_open_stream(fabric, req);
	if (!stream) {
		DBG_INF("Failed to open stream");
		efree(req);
		MYSQLND_MS_INC_STATISTIC(MS_STAT_FABRIC_SHARDING_LOOKUP_SERVERS_FAILURE);
		MYSQLND_MS_STATS_TIME_DIFF(fetch_time);
		MYSQLND_MS_INC_STATISTIC_W_VALUE(MS_STAT_FABRIC_SHARDING_LOOKUP_SERVERS_TIME_TOTAL, (uint64_t)fetch_time);
		SET_FABRIC_ERROR(*fabric, 2000, "HY000", "Failed to open stream to fabric host.");
		DBG_RETURN(NULL);
	}

	while ((len = php_stream_read(stream, buf, sizeof(buf))) > 0) {
		smart_str_appendl(&xml_str, buf, len);
	}
	smart_str_appendc(&xml_str, '\0');
	php_stream_close(stream);

	MYSQLND_MS_INC_STATISTIC(MS_STAT_FABRIC_SHARDING_LOOKUP_SERVERS_SUCCESS);
	MYSQLND_MS_STATS_TIME_DIFF(fetch_time);
	MYSQLND_MS_INC_STATISTIC_W_VALUE(MS_STAT_FABRIC_SHARDING_LOOKUP_SERVERS_TIME_TOTAL, (uint64_t)fetch_time);
	MYSQLND_MS_INC_STATISTIC_W_VALUE(MS_STAT_FABRIC_SHARDING_LOOKUP_SERVERS_BYTES_TOTAL, xml_str.len);

	retval = mysqlnd_fabric_parse_xml(fabric, (xml_str.c) ? xml_str.c : "", xml_str.len);
	if (!retval) {
		MYSQLND_MS_INC_STATISTIC(MS_STAT_FABRIC_SHARDING_LOOKUP_SERVERS_XML_FAILURE);
		DBG_INF_FMT("Request %s", req);
		DBG_INF_FMT("Reply %s", xml_str.c);
	}

	efree(req);
	smart_str_free(&xml_str);

	DBG_RETURN(retval);
}

void mysqlnd_fabric_free_server_list(MYSQLND_MS_FABRIC_SERVER *servers)
{
	MYSQLND_MS_FABRIC_SERVER *pos;

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
