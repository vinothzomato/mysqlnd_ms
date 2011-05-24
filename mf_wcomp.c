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
#include "php.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_priv.h"
#include "ext/mysqlnd/mysqlnd_debug.h"

/* $Id: mysqlnd_ms_config_json.c 311386 2011-05-24 12:10:21Z andrey $ */

/*
	The code has been customized by A. Hristov for the PHP environment.
	It is based from code with the following copyright and license
*/

/* Copyright Abandoned 1996 TCX DataKonsult AB & Monty Program KB & Detron HB
This file is public domain and comes with NO WARRANTY of any kind */


/* Funktions for comparing with wild-cards */

				/* Test if a string is "comparable" to a wild-card string */
				/* returns 0 if the strings are "comparable" */

static char wild_many='*';
static char wild_one='?';
static char wild_prefix=0;


/* {{{ mysqlnd_ms_wild_compare */
PHPAPI
int mysqlnd_ms_wild_compare(const char * str, const char * wildstr TSRMLS_DC)
{
	int flag;
	DBG_ENTER("mysqlnd_ms_wild_compare");

	while (*wildstr) {
		while (*wildstr && *wildstr != wild_many && *wildstr != wild_one) {
			if (*wildstr == wild_prefix && wildstr[1]) {
				wildstr++;
			}
			if (*wildstr++ != *str++) {
				DBG_RETURN(1);
			}
		}
		if (!*wildstr) {
			 DBG_RETURN(*str != 0);
		}
		if (*wildstr++ == wild_one) {
			if (!*str++) {
				DBG_RETURN(1);	/* One char; skipp */
			}
		} else {		/* Found '*' */
			if (!*wildstr) {
				DBG_RETURN(0);		/* '*' as last char: OK */
			}
			flag = (*wildstr != wild_many && *wildstr != wild_one);
			do {
				if (flag) {
					char cmp;
					if ((cmp= *wildstr) == wild_prefix && wildstr[1]) {
						cmp=wildstr[1];
					}
					while (*str && *str != cmp) {
						str++;
					}
					if (!*str) {
						DBG_RETURN(1);
					}
				}
				if (mysqlnd_ms_wild_compare(str, wildstr TSRMLS_CC) == 0) {
					DBG_RETURN(0);
				}
			} while (*str++ && wildstr[0] != wild_many);
			DBG_RETURN(1);
		}
	}
	DBG_RETURN(*str != '\0');
} /* wild_compare */
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
