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
  | Authors: Andrey Hristov <andrey@mysql.com>                           |
  |          Ulf Wendel <uwendel@mysql.com>                              |
  +----------------------------------------------------------------------+
*/

/* $Id: header 252479 2008-02-07 19:39:50Z iliaa $ */

#ifndef MYSQLND_MS_TOKENIZE_H
#define MYSQLND_MS_TOKENIZE_H


PHPAPI struct st_qc_token_and_value mysqlnd_ms_get_token(const char **p, size_t * query_len, const MYSQLND_CHARSET * cset TSRMLS_DC);

PHPAPI struct st_mysqlnd_tok_scanner * mysqlnd_tok_create_scanner(const char * const query, const size_t query_len TSRMLS_DC);
PHPAPI void mysqlnd_tok_free_scanner(struct st_mysqlnd_tok_scanner * scanner TSRMLS_DC);
PHPAPI struct st_qc_token_and_value mysqlnd_tok_get_token(struct st_mysqlnd_tok_scanner * scanner TSRMLS_DC);


PHPAPI struct st_mysqlnd_tok_parser * mysqlnd_par_tok_create_parser(TSRMLS_D);
PHPAPI void mysqlnd_par_tok_free_parser(struct st_mysqlnd_tok_parser * parser TSRMLS_DC);
PHPAPI int mysqlnd_par_tok_start_parser(struct st_mysqlnd_tok_parser * parser, const char * const query, const size_t query_len TSRMLS_DC);

#endif /* MYSQLND_MS_TOKENIZE_H */
