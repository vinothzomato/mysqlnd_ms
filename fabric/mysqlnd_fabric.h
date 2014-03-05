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

#ifndef MYSQLND_FABRIC_H
#define MYSQLND_FABRIC_H

MYSQLND_MS_FABRIC *mysqlnd_fabric_init();
void mysqlnd_fabric_free(MYSQLND_MS_FABRIC *fabric);
int mysqlnd_fabric_add_host(MYSQLND_MS_FABRIC *fabric, char *hostname, int port);

typedef void(*mysqlnd_fabric_apply_func)(const char *hostname, unsigned int port, void *data);

int mysqlnd_fabric_host_list_apply(const MYSQLND_MS_FABRIC *fabric, mysqlnd_fabric_apply_func cb, void *data);

MYSQLND_MS_FABRIC_SERVER *mysqlnd_fabric_get_shard_servers(MYSQLND_MS_FABRIC *fabric, const char *table, const char *key, enum mysqlnd_ms_fabric_hint hint TSRMLS_DC);
void mysqlnd_fabric_free_server_list(MYSQLND_MS_FABRIC_SERVER *servers);

#endif	/* MYSQLND_FABRIC_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
