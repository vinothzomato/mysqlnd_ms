PHP_ARG_ENABLE(mysqlnd_ms, whether to enable mysqlnd_ms support,
[  --enable-mysqlnd-ms           Enable mysqlnd_ms support])

if test "$PHP_MYSQLND_MS" && test "$PHP_MYSQLND_MS" != "no"; then
  PHP_SUBST(MYSQLND_MS_SHARED_LIBADD)

  PHP_ADD_EXTENSION_DEP(mysqlnd_ms, mysqlnd)
  PHP_NEW_EXTENSION(mysqlnd_ms, php_mysqlnd_ms.c mysqlnd_ms.c mysqlnd_ms_config_json.c \
  								mysqlnd_query_lexer.c mysqlnd_query_parser.c, $ext_shared)

fi
