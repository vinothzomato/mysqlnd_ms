PHP_ARG_ENABLE(mysqlnd_ms, whether to enable mysqlnd_ms support,
[  --enable-mysqlnd-ms           Enable mysqlnd_ms support])

if test "$PHP_MYSQLND_MS" && test "$PHP_MYSQLND_MS" != "no"; then
  PHP_SUBST(MYSQLND_MS_SHARED_LIBADD)

  mysqlnd_ms_sources="php_mysqlnd_ms.c mysqlnd_ms.c mysqlnd_ms_switch.c mysqlnd_ms_config_json.c \
                  mf_wcomp.c mysqlnd_query_lexer.c mysqlnd_query_parser.c \
                  mysqlnd_ms_filter_random.c mysqlnd_ms_filter_random_once.c \
				  mysqlnd_ms_filter_round_robin.c mysqlnd_ms_filter_user.c mysqlnd_ms_filter_table_partition.c"

  PHP_ADD_EXTENSION_DEP(mysqlnd_ms, mysqlnd)
  PHP_ADD_EXTENSION_DEP(mysqlnd_ms, json)
  PHP_NEW_EXTENSION(mysqlnd_ms, $mysqlnd_ms_sources, $ext_shared)
fi
