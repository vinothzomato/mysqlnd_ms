PHP_ARG_ENABLE(mysqlnd_ms, whether to enable mysqlnd_ms support,
[  --enable-mysqlnd-ms           Enable mysqlnd_ms support])

PHP_ARG_ENABLE(mysqlnd_ms_table_filter, whether to enable table filter in mysqlnd_ms,
[  --enable-mysqlnd-ms-table-filter   Enable support for table filter in mysqlnd_ms (EXPERIMENTAL - do not use!)], no, no)
PHP_ARG_ENABLE(mysqlnd_ms_cache_support, whether to query caching through mysqlnd_qc in mysqlnd_ms,
[  --enable-mysqlnd-ms-cache-support   Enable query caching through mysqlnd_qc (EXPERIMENTAL - do not use!)], no, no)
PHP_ARG_WITH(mysqlnd-qc-dir,  for mysqlnd_qc,
[  --with-mysqlnd-qc-dir[=DIR]   Set mysqlnd_qc source.], yes)

if test "$PHP_MYSQLND_MS" && test "$PHP_MYSQLND_MS" != "no"; then
  PHP_SUBST(MYSQLND_MS_SHARED_LIBADD)

  mysqlnd_ms_sources="php_mysqlnd_ms.c mysqlnd_ms.c mysqlnd_ms_switch.c mysqlnd_ms_config_json.c \
                  mf_wcomp.c mysqlnd_query_lexer.c \
                  mysqlnd_ms_filter_random.c mysqlnd_ms_filter_round_robin.c \
                  mysqlnd_ms_filter_user.c mysqlnd_ms_filter_qos.c"

  if test "$PHP_MYSQLND_MS_TABLE_FILTER" && test "$PHP_MYSQLND_MS_TABLE_FILTER" != "no"; then
    AC_DEFINE([MYSQLND_MS_HAVE_FILTER_TABLE_PARTITION], 1, [Enable table partition support])
    mysqlnd_ms_sources="$mysqlnd_ms_sources mysqlnd_query_parser.c mysqlnd_ms_filter_table_partition.c"
  fi

  if test "$PHP_MYSQLND_MS_CACHE_SUPPORT" && test "$PHP_MYSQLND_MS_CACHE_SUPPORT" != "no"; then
    AC_MSG_CHECKING([for mysqlnd_qc header location]);
    if test "$PHP_MYSQLND_QC_DIR" = "no" || test "$PHP_MYSQLND_QC_DIR" = ""; then
      AC_MSG_RESULT([Using bundled])
      if test -r "$abs_srcdir/ext/mysqlnd_qc/mysqlnd_qc.h" ; then
        PHP_ADD_INCLUDE("$abs_srcdir/ext/mysqlnd_qc/mysqlnd_qc.h")
      else
        AC_MSG_ERROR([mysqlnd_qc header not found, please use --with-mysqlnd-qc-dir=DIR])
      fi
    else
      if test -r "$PHP_MYSQLND_QC_DIR/mysqlnd_qc.h" ; then
        AC_MSG_RESULT([$PHP_MYSQLND_QC_DIR])
        PHP_ADD_INCLUDE($PHP_MYSQLND_QC_DIR)
      else
        AC_MSG_ERROR([Wrong: mysqlnd_qc header not found, please use --with-mysqlnd-qc-dir=DIR])
      fi
    fi

    PHP_ADD_EXTENSION_DEP(mysqlnd_ms, mysqlnd_qc)
    AC_DEFINE([MYSQLND_MS_HAVE_MYSQLND_QC], 1, [Whether mysqlnd_qc is enabled])
  fi

  PHP_ADD_EXTENSION_DEP(mysqlnd_ms, mysqlnd)
  PHP_ADD_EXTENSION_DEP(mysqlnd_ms, json)
  PHP_NEW_EXTENSION(mysqlnd_ms, $mysqlnd_ms_sources, $ext_shared)
fi
