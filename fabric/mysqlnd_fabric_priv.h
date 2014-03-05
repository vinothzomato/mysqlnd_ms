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

#ifndef MYSQLND_FABRIC_PRIV_H
#define MYSQLND_FABRIC_PRIV_H

/* Staying close to mysqlnd here for now, may change later */
#define SET_EMPTY_FABRIC_ERROR(fabric) \
{ \
	(fabric).error_no = 0; \
	(fabric).error[0] = '\0'; \
}

#define SET_FABRIC_ERROR(fabric, a_error_no, b_sqlstate, c_error) \
{\
	if (0 == (a_error_no)) { \
		SET_EMPTY_FABRIC_ERROR(fabric); \
	} else { \
		(fabric).error_no = a_error_no; \
		strlcpy((fabric).sqlstate, b_sqlstate, sizeof((fabric).sqlstate)); \
		strlcpy((fabric).error, c_error, sizeof((fabric).error)); \
	} \
}

MYSQLND_MS_FABRIC_SERVER *mysqlnd_fabric_parse_xml(MYSQLND_MS_FABRIC * fabric, char *xmlstr, int xmlstr_len);

#endif	/* MYSQLND_FABRIC_PRIV_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
