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
  +----------------------------------------------------------------------+
*/

/* $Id: header 252479 2008-02-07 19:39:50Z iliaa $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "mysqlnd_ms.h"

static const char * qc_tokens_names[]=
{
"QC_TOKEN_ABORT"					 /* INTERNAL (used in lex) */
,"QC_TOKEN_ACCESSIBLE"
,"QC_TOKEN_ACTION"						/* SQL-2003-N */
,"QC_TOKEN_ADD"						   /* SQL-2003-R */
,"QC_TOKEN_ADDDATE"				   /* MYSQL-FUNC */
,"QC_TOKEN_AFTER"					 /* SQL-2003-N */
,"QC_TOKEN_AGAINST"
,"QC_TOKEN_AGGREGATE"
,"QC_TOKEN_ALGORITHM"
,"QC_TOKEN_ALL"						   /* SQL-2003-R */
,"QC_TOKEN_ALTER"						 /* SQL-2003-R */
,"QC_TOKEN_ANALYZE"
,"QC_TOKEN_AND_AND"				   /* OPERATOR */
,"QC_TOKEN_AND"					   /* SQL-2003-R */
,"QC_TOKEN_ANY"					   /* SQL-2003-R */
,"QC_TOKEN_AS"							/* SQL-2003-R */
,"QC_TOKEN_ASC"						   /* SQL-2003-N */
,"QC_TOKEN_ASCII"					 /* MYSQL-FUNC */
,"QC_TOKEN_ASENSITIVE"				/* FUTURE-USE */
,"QC_TOKEN_AT"						/* SQL-2003-R */
,"QC_TOKEN_AUTHORS"
,"QC_TOKEN_AUTOEXTEND_SIZE"
,"QC_TOKEN_AUTO_INC"
,"QC_TOKEN_AVG_ROW_LENGTH"
,"QC_TOKEN_AVG"					   /* SQL-2003-N */
,"QC_TOKEN_BACKUP"
,"QC_TOKEN_BEFORE"					/* SQL-2003-N */
,"QC_TOKEN_BEGIN"					 /* SQL-2003-R */
,"QC_TOKEN_BETWEEN"				   /* SQL-2003-R */
,"QC_TOKEN_BIGINT"						/* SQL-2003-R */
,"QC_TOKEN_BINARY"						/* SQL-2003-R */
,"QC_TOKEN_BINLOG"
,"QC_TOKEN_BIN_NUM"
,"QC_TOKEN_BIT_AND"					   /* MYSQL-FUNC */
,"QC_TOKEN_BIT_OR"						/* MYSQL-FUNC */
,"QC_TOKEN_BIT"					   /* MYSQL-FUNC */
,"QC_TOKEN_BIT_XOR"					   /* MYSQL-FUNC */
,"QC_TOKEN_BLOB"					  /* SQL-2003-R */
,"QC_TOKEN_BLOCK"
,"QC_TOKEN_BOOLEAN"				   /* SQL-2003-R */
,"QC_TOKEN_BOOL"
,"QC_TOKEN_BOTH"						  /* SQL-2003-R */
,"QC_TOKEN_BTREE"
,"QC_TOKEN_BY"							/* SQL-2003-R */
,"QC_TOKEN_BYTE"
,"QC_TOKEN_CACHE"
,"QC_TOKEN_CALL"					  /* SQL-2003-R */
,"QC_TOKEN_CASCADE"					   /* SQL-2003-N */
,"QC_TOKEN_CASCADED"					  /* SQL-2003-R */
,"QC_TOKEN_CASE"					  /* SQL-2003-R */
,"QC_TOKEN_CAST"					  /* SQL-2003-R */
,"QC_TOKEN_CATALOG_NAME"			  /* SQL-2003-N */
,"QC_TOKEN_CHAIN"					 /* SQL-2003-N */
,"QC_TOKEN_CHANGE"
,"QC_TOKEN_CHANGED"
,"QC_TOKEN_CHARSET"
,"QC_TOKEN_CHAR"					  /* SQL-2003-R */
,"QC_TOKEN_CHECKSUM"
,"QC_TOKEN_CHECK"					 /* SQL-2003-R */
,"QC_TOKEN_CIPHER"
,"QC_TOKEN_CLASS_ORIGIN"			  /* SQL-2003-N */
,"QC_TOKEN_CLIENT"
,"QC_TOKEN_CLOSE"					 /* SQL-2003-R */
,"QC_TOKEN_COALESCE"					  /* SQL-2003-N */
,"QC_TOKEN_CODE"
,"QC_TOKEN_COLLATE"				   /* SQL-2003-R */
,"QC_TOKEN_COLLATION"				 /* SQL-2003-N */
,"QC_TOKEN_COLUMNS"
,"QC_TOKEN_COLUMN"					/* SQL-2003-R */
,"QC_TOKEN_COLUMN_NAME"			   /* SQL-2003-N */
,"QC_TOKEN_COMMENT"
,"QC_TOKEN_COMMITTED"				 /* SQL-2003-N */
,"QC_TOKEN_COMMIT"					/* SQL-2003-R */
,"QC_TOKEN_COMPACT"
,"QC_TOKEN_COMPLETION"
,"QC_TOKEN_COMPRESSED"
,"QC_TOKEN_CONCURRENT"
,"QC_TOKEN_CONDITION"				 /* SQL-2003-R," SQL-2008-R */
,"QC_TOKEN_CONNECTION"
,"QC_TOKEN_CONSISTENT"
,"QC_TOKEN_CONSTRAINT"					/* SQL-2003-R */
,"QC_TOKEN_CONSTRAINT_CATALOG"		/* SQL-2003-N */
,"QC_TOKEN_CONSTRAINT_NAME"		   /* SQL-2003-N */
,"QC_TOKEN_CONSTRAINT_SCHEMA"		 /* SQL-2003-N */
,"QC_TOKEN_CONTAINS"				  /* SQL-2003-N */
,"QC_TOKEN_CONTEXT"
,"QC_TOKEN_CONTINUE"				  /* SQL-2003-R */
,"QC_TOKEN_CONTRIBUTORS"
,"QC_TOKEN_CONVERT"				   /* SQL-2003-N */
,"QC_TOKEN_COUNT"					 /* SQL-2003-N */
,"QC_TOKEN_CPU"
,"QC_TOKEN_CREATE"						/* SQL-2003-R */
,"QC_TOKEN_CROSS"						 /* SQL-2003-R */
,"QC_TOKEN_CUBE"					  /* SQL-2003-R */
,"QC_TOKEN_CURDATE"					   /* MYSQL-FUNC */
,"QC_TOKEN_CURRENT_USER"				  /* SQL-2003-R */
,"QC_TOKEN_CURSOR"					/* SQL-2003-R */
,"QC_TOKEN_CURSOR_NAME"			   /* SQL-2003-N */
,"QC_TOKEN_CURTIME"					   /* MYSQL-FUNC */
,"QC_TOKEN_DATABASE"
,"QC_TOKEN_DATABASES"
,"QC_TOKEN_DATAFILE"
,"QC_TOKEN_DATA"					  /* SQL-2003-N */
,"QC_TOKEN_DATETIME"
,"QC_TOKEN_DATE_ADD_INTERVAL"			 /* MYSQL-FUNC */
,"QC_TOKEN_DATE_SUB_INTERVAL"			 /* MYSQL-FUNC */
,"QC_TOKEN_DATE"					  /* SQL-2003-R */
,"QC_TOKEN_DAY_HOUR"
,"QC_TOKEN_DAY_MICROSECOND"
,"QC_TOKEN_DAY_MINUTE"
,"QC_TOKEN_DAY_SECOND"
,"QC_TOKEN_DAY"					   /* SQL-2003-R */
,"QC_TOKEN_DEALLOCATE"				/* SQL-2003-R */
,"QC_TOKEN_DECIMAL_NUM"
,"QC_TOKEN_DECIMAL"				   /* SQL-2003-R */
,"QC_TOKEN_DECLARE"				   /* SQL-2003-R */
,"QC_TOKEN_DEFAULT"					   /* SQL-2003-R */
,"QC_TOKEN_DEFINER"
,"QC_TOKEN_DELAYED"
,"QC_TOKEN_DELAY_KEY_WRITE"
,"QC_TOKEN_DELETE"					/* SQL-2003-R */
,"QC_TOKEN_DESC"						  /* SQL-2003-N */
,"QC_TOKEN_DESCRIBE"					  /* SQL-2003-R */
,"QC_TOKEN_DES_KEY_FILE"
,"QC_TOKEN_DETERMINISTIC"			 /* SQL-2003-R */
,"QC_TOKEN_DIRECTORY"
,"QC_TOKEN_DISABLE"
,"QC_TOKEN_DISCARD"
,"QC_TOKEN_DISK"
,"QC_TOKEN_DISTINCT"					  /* SQL-2003-R */
,"QC_TOKEN_DIV"
,"QC_TOKEN_DOUBLE"					/* SQL-2003-R */
,"QC_TOKEN_DO"
,"QC_TOKEN_DROP"						  /* SQL-2003-R */
,"QC_TOKEN_DUAL"
,"QC_TOKEN_DUMPFILE"
,"QC_TOKEN_DUPLICATE"
,"QC_TOKEN_DYNAMIC"				   /* SQL-2003-R */
,"QC_TOKEN_EACH"					  /* SQL-2003-R */
,"QC_TOKEN_ELSE"						  /* SQL-2003-R */
,"QC_TOKEN_ELSEIF"
,"QC_TOKEN_ENABLE"
,"QC_TOKEN_ENCLOSED"
,"QC_TOKEN_END"						   /* SQL-2003-R */
,"QC_TOKEN_ENDS"
,"QC_TOKEN_END_OF_INPUT"				  /* INTERNAL */
,"QC_TOKEN_ENGINES"
,"QC_TOKEN_ENGINE"
,"QC_TOKEN_ENUM"
,"QC_TOKEN_EQ"							/* OPERATOR */
,"QC_TOKEN_EQUAL"					 /* OPERATOR */
,"QC_TOKEN_ERRORS"
,"QC_TOKEN_ESCAPED"
,"QC_TOKEN_ESCAPE"					/* SQL-2003-R */
,"QC_TOKEN_EVENTS"
,"QC_TOKEN_EVENT"
,"QC_TOKEN_EVERY"					 /* SQL-2003-N */
,"QC_TOKEN_EXECUTE"				   /* SQL-2003-R */
,"QC_TOKEN_EXISTS"						/* SQL-2003-R */
,"QC_TOKEN_EXIT"
,"QC_TOKEN_EXPANSION"
,"QC_TOKEN_EXTENDED"
,"QC_TOKEN_EXTENT_SIZE"
,"QC_TOKEN_EXTRACT"				   /* SQL-2003-N */
,"QC_TOKEN_FALSE"					 /* SQL-2003-R */
,"QC_TOKEN_FAST"
,"QC_TOKEN_FAULTS"
,"QC_TOKEN_FETCH"					 /* SQL-2003-R */
,"QC_TOKEN_FILE"
,"QC_TOKEN_FIRST"					 /* SQL-2003-N */
,"QC_TOKEN_FIXED"
,"QC_TOKEN_FLOAT_NUM"
,"QC_TOKEN_FLOAT"					 /* SQL-2003-R */
,"QC_TOKEN_FLUSH"
,"QC_TOKEN_FORCE"
,"QC_TOKEN_FOREIGN"					   /* SQL-2003-R */
,"QC_TOKEN_FOR"					   /* SQL-2003-R */
,"QC_TOKEN_FOUND"					 /* SQL-2003-R */
,"QC_TOKEN_FRAC_SECOND"
,"QC_TOKEN_FROM"
,"QC_TOKEN_FULL"						  /* SQL-2003-R */
,"QC_TOKEN_FULLTEXT"
,"QC_TOKEN_FUNCTION"				  /* SQL-2003-R */
,"QC_TOKEN_GE"
,"QC_TOKEN_GEOMETRYCOLLECTION"
,"QC_TOKEN_GEOMETRY"
,"QC_TOKEN_GET_FORMAT"					/* MYSQL-FUNC */
,"QC_TOKEN_GLOBAL"					/* SQL-2003-R */
,"QC_TOKEN_GRANT"						 /* SQL-2003-R */
,"QC_TOKEN_GRANTS"
,"QC_TOKEN_GROUP"					 /* SQL-2003-R */
,"QC_TOKEN_GROUP_CONCAT"
,"QC_TOKEN_GT"						/* OPERATOR */
,"QC_TOKEN_HANDLER"
,"QC_TOKEN_HASH"
,"QC_TOKEN_HAVING"						/* SQL-2003-R */
,"QC_TOKEN_HELP"
,"QC_TOKEN_HEX_NUM"
,"QC_TOKEN_HIGH_PRIORITY"
,"QC_TOKEN_HOST"
,"QC_TOKEN_HOSTS"
,"QC_TOKEN_HOUR_MICROSECOND"
,"QC_TOKEN_HOUR_MINUTE"
,"QC_TOKEN_HOUR_SECOND"
,"QC_TOKEN_HOUR"					  /* SQL-2003-R */
,"QC_TOKEN_IDENT"
,"QC_TOKEN_IDENTIFIED"
,"QC_TOKEN_IDENT_QUOTED"
,"QC_TOKEN_IF"
,"QC_TOKEN_IGNORE"
,"QC_TOKEN_IGNORE_SERVER_IDS"
,"QC_TOKEN_IMPORT"
,"QC_TOKEN_INDEXES"
,"QC_TOKEN_INDEX"
,"QC_TOKEN_INFILE"
,"QC_TOKEN_INITIAL_SIZE"
,"QC_TOKEN_INNER"					 /* SQL-2003-R */
,"QC_TOKEN_INOUT"					 /* SQL-2003-R */
,"QC_TOKEN_INSENSITIVE"			   /* SQL-2003-R */
,"QC_TOKEN_INSERT"						/* SQL-2003-R */
,"QC_TOKEN_INSERT_METHOD"
,"QC_TOKEN_INSTALL"
,"QC_TOKEN_INTERVAL"				  /* SQL-2003-R */
,"QC_TOKEN_INTO"						  /* SQL-2003-R */
,"QC_TOKEN_INT"					   /* SQL-2003-R */
,"QC_TOKEN_INVOKER"
,"QC_TOKEN_IN"						/* SQL-2003-R */
,"QC_TOKEN_IO"
,"QC_TOKEN_IPC"
,"QC_TOKEN_IS"							/* SQL-2003-R */
,"QC_TOKEN_ISOLATION"					 /* SQL-2003-R */
,"QC_TOKEN_ISSUER"
,"QC_TOKEN_ITERATE"
,"QC_TOKEN_JOIN"					  /* SQL-2003-R */
,"QC_TOKEN_KEYS"
,"QC_TOKEN_KEY_BLOCK_SIZE"
,"QC_TOKEN_KEY"					   /* SQL-2003-N */
,"QC_TOKEN_KILL"
,"QC_TOKEN_LANGUAGE"				  /* SQL-2003-R */
,"QC_TOKEN_LAST"					  /* SQL-2003-N */
,"QC_TOKEN_LE"							/* OPERATOR */
,"QC_TOKEN_LEADING"					   /* SQL-2003-R */
,"QC_TOKEN_LEAVES"
,"QC_TOKEN_LEAVE"
,"QC_TOKEN_LEFT"						  /* SQL-2003-R */
,"QC_TOKEN_LESS"
,"QC_TOKEN_LEVEL"
,"QC_TOKEN_LEX_HOSTNAME"
,"QC_TOKEN_LIKE"						  /* SQL-2003-R */
,"QC_TOKEN_LIMIT"
,"QC_TOKEN_LINEAR"
,"QC_TOKEN_LINES"
,"QC_TOKEN_LINESTRING"
,"QC_TOKEN_LIST"
,"QC_TOKEN_LOAD"
,"QC_TOKEN_LOCAL"					 /* SQL-2003-R */
,"QC_TOKEN_LOCATOR"				   /* SQL-2003-N */
,"QC_TOKEN_LOCKS"
,"QC_TOKEN_LOCK"
,"QC_TOKEN_LOGFILE"
,"QC_TOKEN_LOGS"
,"QC_TOKEN_LONGBLOB"
,"QC_TOKEN_LONGTEXT"
,"QC_TOKEN_LONG_NUM"
,"QC_TOKEN_LONG"
,"QC_TOKEN_LOOP"
,"QC_TOKEN_LOW_PRIORITY"
,"QC_TOKEN_LT"							/* OPERATOR */
,"QC_TOKEN_MASTER_CONNECT_RETRY"
,"QC_TOKEN_MASTER_HOST"
,"QC_TOKEN_MASTER_LOG_FILE"
,"QC_TOKEN_MASTER_LOG_POS"
,"QC_TOKEN_MASTER_PASSWORD"
,"QC_TOKEN_MASTER_PORT"
,"QC_TOKEN_MASTER_SERVER_ID"
,"QC_TOKEN_MASTER_SSL_CAPATH"
,"QC_TOKEN_MASTER_SSL_CA"
,"QC_TOKEN_MASTER_SSL_CERT"
,"QC_TOKEN_MASTER_SSL_CIPHER"
,"QC_TOKEN_MASTER_SSL_KEY"
,"QC_TOKEN_MASTER_SSL"
,"QC_TOKEN_MASTER_SSL_VERIFY_SERVER_CERT"
,"QC_TOKEN_MASTER"
,"QC_TOKEN_MASTER_USER"
,"QC_TOKEN_MASTER_HEARTBEAT_PERIOD"
,"QC_TOKEN_MATCH"						 /* SQL-2003-R */
,"QC_TOKEN_MAX_CONNECTIONS_PER_HOUR"
,"QC_TOKEN_MAX_QUERIES_PER_HOUR"
,"QC_TOKEN_MAX_ROWS"
,"QC_TOKEN_MAX_SIZE"
,"QC_TOKEN_MAX"					   /* SQL-2003-N */
,"QC_TOKEN_MAX_UPDATES_PER_HOUR"
,"QC_TOKEN_MAX_USER_CONNECTIONS"
,"QC_TOKEN_MAX_VALUE"				 /* SQL-2003-N */
,"QC_TOKEN_MEDIUMBLOB"
,"QC_TOKEN_MEDIUMINT"
,"QC_TOKEN_MEDIUMTEXT"
,"QC_TOKEN_MEDIUM"
,"QC_TOKEN_MEMORY"
,"QC_TOKEN_MERGE"					 /* SQL-2003-R */
,"QC_TOKEN_MESSAGE_TEXT"			  /* SQL-2003-N */
,"QC_TOKEN_MICROSECOND"			   /* MYSQL-FUNC */
,"QC_TOKEN_MIGRATE"
,"QC_TOKEN_MINUTE_MICROSECOND"
,"QC_TOKEN_MINUTE_SECOND"
,"QC_TOKEN_MINUTE"					/* SQL-2003-R */
,"QC_TOKEN_MIN_ROWS"
,"QC_TOKEN_MIN"					   /* SQL-2003-N */
,"QC_TOKEN_MODE"
,"QC_TOKEN_MODIFIES"				  /* SQL-2003-R */
,"QC_TOKEN_MODIFY"
,"QC_TOKEN_MOD"					   /* SQL-2003-N */
,"QC_TOKEN_MONTH"					 /* SQL-2003-R */
,"QC_TOKEN_MULTILINESTRING"
,"QC_TOKEN_MULTIPOINT"
,"QC_TOKEN_MULTIPOLYGON"
,"QC_TOKEN_MUTEX"
,"QC_TOKEN_MYSQL_ERRNO"
,"QC_TOKEN_NAMES"					 /* SQL-2003-N */
,"QC_TOKEN_NAME"					  /* SQL-2003-N */
,"QC_TOKEN_NATIONAL"				  /* SQL-2003-R */
,"QC_TOKEN_NATURAL"					   /* SQL-2003-R */
,"QC_TOKEN_NCHAR_STRING"
,"QC_TOKEN_NCHAR"					 /* SQL-2003-R */
,"QC_TOKEN_NDBCLUSTER"
,"QC_TOKEN_NE"							/* OPERATOR */
,"QC_TOKEN_NEG"
,"QC_TOKEN_NEW"					   /* SQL-2003-R */
,"QC_TOKEN_NEXT"					  /* SQL-2003-N */
,"QC_TOKEN_NODEGROUP"
,"QC_TOKEN_NONE"					  /* SQL-2003-R */
,"QC_TOKEN_NOT2"
,"QC_TOKEN_NOT"					   /* SQL-2003-R */
,"QC_TOKEN_NOW"
,"QC_TOKEN_NO"						/* SQL-2003-R */
,"QC_TOKEN_NO_WAIT"
,"QC_TOKEN_NO_WRITE_TO_BINLOG"
,"QC_TOKEN_NULL"					  /* SQL-2003-R */
,"QC_TOKEN_NUM"
,"QC_TOKEN_NUMERIC"				   /* SQL-2003-R */
,"QC_TOKEN_NVARCHAR"
,"QC_TOKEN_OFFSET"
,"QC_TOKEN_OLD_PASSWORD"
,"QC_TOKEN_ON"							/* SQL-2003-R */
,"QC_TOKEN_ONE_SHOT"
,"QC_TOKEN_ONE"
,"QC_TOKEN_OPEN"					  /* SQL-2003-R */
,"QC_TOKEN_OPTIMIZE"
,"QC_TOKEN_OPTIONS"
,"QC_TOKEN_OPTION"						/* SQL-2003-N */
,"QC_TOKEN_OPTIONALLY"
,"QC_TOKEN_OR2"
,"QC_TOKEN_ORDER"					 /* SQL-2003-R */
,"QC_TOKEN_OR_OR"					 /* OPERATOR */
,"QC_TOKEN_OR"						/* SQL-2003-R */
,"QC_TOKEN_OUTER"
,"QC_TOKEN_OUTFILE"
,"QC_TOKEN_OUT"					   /* SQL-2003-R */
,"QC_TOKEN_OWNER"
,"QC_TOKEN_PACK_KEYS"
,"QC_TOKEN_PAGE"
,"QC_TOKEN_PARAM_MARKER"
,"QC_TOKEN_PARSER"
,"QC_TOKEN_PARTIAL"					   /* SQL-2003-N */
,"QC_TOKEN_PARTITIONING"
,"QC_TOKEN_PARTITIONS"
,"QC_TOKEN_PARTITION"				 /* SQL-2003-R */
,"QC_TOKEN_PASSWORD"
,"QC_TOKEN_PHASE"
,"QC_TOKEN_PLUGINS"
,"QC_TOKEN_PLUGIN"
,"QC_TOKEN_POINT"
,"QC_TOKEN_POLYGON"
,"QC_TOKEN_PORT"
,"QC_TOKEN_POSITION"				  /* SQL-2003-N */
,"QC_TOKEN_PRECISION"					 /* SQL-2003-R */
,"QC_TOKEN_PREPARE"				   /* SQL-2003-R */
,"QC_TOKEN_PRESERVE"
,"QC_TOKEN_PREV"
,"QC_TOKEN_PRIMARY"				   /* SQL-2003-R */
,"QC_TOKEN_PRIVILEGES"					/* SQL-2003-N */
,"QC_TOKEN_PROCEDURE"					 /* SQL-2003-R */
,"QC_TOKEN_PROCESS"
,"QC_TOKEN_PROCESSLIST"
,"QC_TOKEN_PROFILE"
,"QC_TOKEN_PROFILES"
,"QC_TOKEN_PURGE"
,"QC_TOKEN_QUARTER"
,"QC_TOKEN_QUERY"
,"QC_TOKEN_QUICK"
,"QC_TOKEN_RANGE"					 /* SQL-2003-R */
,"QC_TOKEN_READS"					 /* SQL-2003-R */
,"QC_TOKEN_READ_ONLY"
,"QC_TOKEN_READ"					  /* SQL-2003-N */
,"QC_TOKEN_READ_WRITE"
,"QC_TOKEN_REAL"						  /* SQL-2003-R */
,"QC_TOKEN_REBUILD"
,"QC_TOKEN_RECOVER"
,"QC_TOKEN_REDOFILE"
,"QC_TOKEN_REDO_BUFFER_SIZE"
,"QC_TOKEN_REDUNDANT"
,"QC_TOKEN_REFERENCES"					/* SQL-2003-R */
,"QC_TOKEN_REGEXP"
,"QC_TOKEN_RELAYLOG"
,"QC_TOKEN_RELAY_LOG_FILE"
,"QC_TOKEN_RELAY_LOG_POS"
,"QC_TOKEN_RELAY_THREAD"
,"QC_TOKEN_RELEASE"				   /* SQL-2003-R */
,"QC_TOKEN_RELOAD"
,"QC_TOKEN_REMOVE"
,"QC_TOKEN_RENAME"
,"QC_TOKEN_REORGANIZE"
,"QC_TOKEN_REPAIR"
,"QC_TOKEN_REPEATABLE"				/* SQL-2003-N */
,"QC_TOKEN_REPEAT"					/* MYSQL-FUNC */
,"QC_TOKEN_REPLACE"					   /* MYSQL-FUNC */
,"QC_TOKEN_REPLICATION"
,"QC_TOKEN_REQUIRE"
,"QC_TOKEN_RESET"
,"QC_TOKEN_RESIGNAL"				  /* SQL-2003-R */
,"QC_TOKEN_RESOURCES"
,"QC_TOKEN_RESTORE"
,"QC_TOKEN_RESTRICT"
,"QC_TOKEN_RESUME"
,"QC_TOKEN_RETURNS"				   /* SQL-2003-R */
,"QC_TOKEN_RETURN"					/* SQL-2003-R */
,"QC_TOKEN_REVOKE"						/* SQL-2003-R */
,"QC_TOKEN_RIGHT"						 /* SQL-2003-R */
,"QC_TOKEN_ROLLBACK"				  /* SQL-2003-R */
,"QC_TOKEN_ROLLUP"					/* SQL-2003-R */
,"QC_TOKEN_ROUTINE"				   /* SQL-2003-N */
,"QC_TOKEN_ROWS"					  /* SQL-2003-R */
,"QC_TOKEN_ROW_FORMAT"
,"QC_TOKEN_ROW"					   /* SQL-2003-R */
,"QC_TOKEN_RTREE"
,"QC_TOKEN_SAVEPOINT"				 /* SQL-2003-R */
,"QC_TOKEN_SCHEDULE"
,"QC_TOKEN_SCHEMA_NAME"			   /* SQL-2003-N */
,"QC_TOKEN_SECOND_MICROSECOND"
,"QC_TOKEN_SECOND"					/* SQL-2003-R */
,"QC_TOKEN_SECURITY"				  /* SQL-2003-N */
,"QC_TOKEN_SELECT"					/* SQL-2003-R */
,"QC_TOKEN_SENSITIVE"				 /* FUTURE-USE */
,"QC_TOKEN_SEPARATOR"
,"QC_TOKEN_SERIALIZABLE"			  /* SQL-2003-N */
,"QC_TOKEN_SERIAL"
,"QC_TOKEN_SESSION"				   /* SQL-2003-N */
,"QC_TOKEN_SERVER"
,"QC_TOKEN_SERVER_OPTIONS"
,"QC_TOKEN_SET"						   /* SQL-2003-R */
,"QC_TOKEN_SET_VAR"
,"QC_TOKEN_SHARE"
,"QC_TOKEN_SHIFT_LEFT"					/* OPERATOR */
,"QC_TOKEN_SHIFT_RIGHT"				   /* OPERATOR */
,"QC_TOKEN_SHOW"
,"QC_TOKEN_SHUTDOWN"
,"QC_TOKEN_SIGNAL"					/* SQL-2003-R */
,"QC_TOKEN_SIGNED"
,"QC_TOKEN_SIMPLE"					/* SQL-2003-N */
,"QC_TOKEN_SLAVE"
,"QC_TOKEN_SMALLINT"					  /* SQL-2003-R */
,"QC_TOKEN_SNAPSHOT"
,"QC_TOKEN_SOCKET"
,"QC_TOKEN_SONAME"
,"QC_TOKEN_SOUNDS"
,"QC_TOKEN_SOURCE"
,"QC_TOKEN_SPATIAL"
,"QC_TOKEN_SPECIFIC"				  /* SQL-2003-R */
,"QC_TOKEN_SQLEXCEPTION"			  /* SQL-2003-R */
,"QC_TOKEN_SQLSTATE"				  /* SQL-2003-R */
,"QC_TOKEN_SQLWARNING"				/* SQL-2003-R */
,"QC_TOKEN_SQL_BIG_RESULT"
,"QC_TOKEN_SQL_BUFFER_RESULT"
,"QC_TOKEN_SQL_CACHE"
,"QC_TOKEN_SQL_CALC_FOUND_ROWS"
,"QC_TOKEN_SQL_NO_CACHE"
,"QC_TOKEN_SQL_SMALL_RESULT"
,"QC_TOKEN_SQL"					   /* SQL-2003-R */
,"QC_TOKEN_SQL_THREAD"
,"QC_TOKEN_SSL"
,"QC_TOKEN_STARTING"
,"QC_TOKEN_STARTS"
,"QC_TOKEN_START"					 /* SQL-2003-R */
,"QC_TOKEN_STATUS"
,"QC_TOKEN_STDDEV_SAMP"			   /* SQL-2003-N */
,"QC_TOKEN_STD"
,"QC_TOKEN_STOP"
,"QC_TOKEN_STORAGE"
,"QC_TOKEN_STRAIGHT_JOIN"
,"QC_TOKEN_STRING"
,"QC_TOKEN_SUBCLASS_ORIGIN"		   /* SQL-2003-N */
,"QC_TOKEN_SUBDATE"
,"QC_TOKEN_SUBJECT"
,"QC_TOKEN_SUBPARTITIONS"
,"QC_TOKEN_SUBPARTITION"
,"QC_TOKEN_SUBSTRING"					 /* SQL-2003-N */
,"QC_TOKEN_SUM"					   /* SQL-2003-N */
,"QC_TOKEN_SUPER"
,"QC_TOKEN_SUSPEND"
,"QC_TOKEN_SWAPS"
,"QC_TOKEN_SWITCHES"
,"QC_TOKEN_SYSDATE"
,"QC_TOKEN_TABLES"
,"QC_TOKEN_TABLESPACE"
,"QC_TOKEN_TABLE_REF_PRIORITY"
,"QC_TOKEN_TABLE"					 /* SQL-2003-R */
,"QC_TOKEN_TABLE_CHECKSUM"
,"QC_TOKEN_TABLE_NAME"				/* SQL-2003-N */
,"QC_TOKEN_TEMPORARY"					 /* SQL-2003-N */
,"QC_TOKEN_TEMPTABLE"
,"QC_TOKEN_TERMINATED"
,"QC_TOKEN_TEXT_STRING"
,"QC_TOKEN_TEXT"
,"QC_TOKEN_THAN"
,"QC_TOKEN_THEN"					  /* SQL-2003-R */
,"QC_TOKEN_TIMESTAMP"					 /* SQL-2003-R */
,"QC_TOKEN_TIMESTAMP_ADD"
,"QC_TOKEN_TIMESTAMP_DIFF"
,"QC_TOKEN_TIME"					  /* SQL-2003-R */
,"QC_TOKEN_TINYBLOB"
,"QC_TOKEN_TINYINT"
,"QC_TOKEN_TINYTEXT"
,"QC_TOKEN_TO"						/* SQL-2003-R */
,"QC_TOKEN_TRAILING"					  /* SQL-2003-R */
,"QC_TOKEN_TRANSACTION"
,"QC_TOKEN_TRIGGERS"
,"QC_TOKEN_TRIGGER"				   /* SQL-2003-R */
,"QC_TOKEN_TRIM"						  /* SQL-2003-N */
,"QC_TOKEN_TRUE"					  /* SQL-2003-R */
,"QC_TOKEN_TRUNCATE"
,"QC_TOKEN_TYPES"
,"QC_TOKEN_TYPE"					  /* SQL-2003-N */
,"QC_TOKEN_UDF_RETURNS"
,"QC_TOKEN_ULONGLONG_NUM"
,"QC_TOKEN_UNCOMMITTED"			   /* SQL-2003-N */
,"QC_TOKEN_UNDEFINED"
,"QC_TOKEN_UNDERSCORE_CHARSET"
,"QC_TOKEN_UNDOFILE"
,"QC_TOKEN_UNDO_BUFFER_SIZE"
,"QC_TOKEN_UNDO"					  /* FUTURE-USE */
,"QC_TOKEN_UNICODE"
,"QC_TOKEN_UNINSTALL"
,"QC_TOKEN_UNION"					 /* SQL-2003-R */
,"QC_TOKEN_UNIQUE"
,"QC_TOKEN_UNKNOWN"				   /* SQL-2003-R */
,"QC_TOKEN_UNLOCK"
,"QC_TOKEN_UNSIGNED"
,"QC_TOKEN_UNTIL"
,"QC_TOKEN_UPDATE"					/* SQL-2003-R */
,"QC_TOKEN_UPGRADE"
,"QC_TOKEN_USAGE"						 /* SQL-2003-N */
,"QC_TOKEN_USER"						  /* SQL-2003-R */
,"QC_TOKEN_USE_FRM"
,"QC_TOKEN_USE"
,"QC_TOKEN_USING"						 /* SQL-2003-R */
,"QC_TOKEN_UTC_DATE"
,"QC_TOKEN_UTC_TIMESTAMP"
,"QC_TOKEN_UTC_TIME"
,"QC_TOKEN_VALUES"						/* SQL-2003-R */
,"QC_TOKEN_VALUE"					 /* SQL-2003-R */
,"QC_TOKEN_VARBINARY"
,"QC_TOKEN_VARCHAR"					   /* SQL-2003-R */
,"QC_TOKEN_VARIABLES"
,"QC_TOKEN_VARIANCE"
,"QC_TOKEN_VARYING"					   /* SQL-2003-R */
,"QC_TOKEN_VAR_SAMP"
,"QC_TOKEN_VIEW"					  /* SQL-2003-N */
,"QC_TOKEN_WAIT"
,"QC_TOKEN_WARNINGS"
,"QC_TOKEN_WEEK"
,"QC_TOKEN_WHEN"					  /* SQL-2003-R */
,"QC_TOKEN_WHERE"						 /* SQL-2003-R */
,"QC_TOKEN_WHILE"
,"QC_TOKEN_WITH"						  /* SQL-2003-R */
,"QC_TOKEN_WITH_CUBE"				 /* INTERNAL */
,"QC_TOKEN_WITH_ROLLUP"			   /* INTERNAL */
,"QC_TOKEN_WORK"					  /* SQL-2003-N */
,"QC_TOKEN_WRAPPER"
,"QC_TOKEN_WRITE"					 /* SQL-2003-N */
,"QC_TOKEN_X509"
,"QC_TOKEN_XA"
,"QC_TOKEN_XML"
,"QC_TOKEN_XOR"
,"QC_TOKEN_YEAR_MONTH"
,"QC_TOKEN_YEAR"					  /* SQL-2003-R */
,"QC_TOKEN_ZEROFILL"

	,"QC_TOKEN_CLIENT_FLAG"
	,"QC_TOKEN_GLOBAL_VAR"
	,"QC_TOKEN_SESSION_VAR"
	,"QC_TOKEN_BRACKET_OPEN"
	,"QC_TOKEN_BRACKET_CLOSE"
	,"QC_TOKEN_PLUS"
	,"QC_TOKEN_MINUS"
	,"QC_TOKEN_STAR"
	,"QC_TOKEN_COMMA"
	,"QC_TOKEN_DOT"
	,"QC_TOKEN_SEMICOLON"
	,"QC_TOKEN_NO_MORE"
};

struct name_value_pair {
	const char * name;
	enum qc_tokens value;
};

static struct name_value_pair static_tokens[] =
{
	{"DROP",	QC_TOKEN_DROP},
	{"TABLE",	QC_TOKEN_TABLE},
	{"IF",		QC_TOKEN_IF},
	{"NOT",		QC_TOKEN_NOT},
	{"EXISTS",	QC_TOKEN_EXISTS},
	{"CREATE",	QC_TOKEN_CREATE},
	{"INT",		QC_TOKEN_INT},
	{"INTEGER",	QC_TOKEN_INT},
	{"CHAR",	QC_TOKEN_CHAR},
	{"VARCHAR",	QC_TOKEN_VARCHAR},
	{"TINYTEXT",QC_TOKEN_TEXT},
	{"TEXT",	QC_TOKEN_TEXT},
	{"MEDIUMTEXT",QC_TOKEN_TEXT},
	{"LONGTEXT",QC_TOKEN_TEXT},
	{"TINYBLOB",QC_TOKEN_BLOB},
	{"BLOB",	QC_TOKEN_BLOB},
	{"MEDIUMBLOB",QC_TOKEN_BLOB},
	{"LONGBLOB",QC_TOKEN_BLOB},
	{"SELECT",	QC_TOKEN_SELECT},
	{"AS",		QC_TOKEN_AS},
	{"FROM",	QC_TOKEN_FROM},
	{"WHERE",	QC_TOKEN_WHERE},
	{"AND",		QC_TOKEN_AND},
	{"OR",		QC_TOKEN_OR},
	{"HAVING",	QC_TOKEN_HAVING},
	{"GROUP",	QC_TOKEN_GROUP},
	{"BY",		QC_TOKEN_BY},
	{"LIMIT",	QC_TOKEN_LIMIT},
	{"INSERT",	QC_TOKEN_INSERT},
	{"INTO",	QC_TOKEN_INTO},
	{"UPDATE",	QC_TOKEN_UPDATE},
	{"SET",		QC_TOKEN_SET},
	{"DISTINCT",QC_TOKEN_DISTINCT},
	{"DISTINCTROW",			QC_TOKEN_DISTINCT},
	{"HIGH_PRIORITY",		QC_TOKEN_HIGH_PRIORITY},
	{"STRAIGHT_JOIN",		QC_TOKEN_STRAIGHT_JOIN},
	{"SQL_SMALL_RESULT",	QC_TOKEN_SQL_SMALL_RESULT},
	{"SQL_BIG_RESULT",		QC_TOKEN_SQL_BIG_RESULT},
	{"SQL_BUFFER_RESULT",	QC_TOKEN_SQL_BUFFER_RESULT},
	{"SQL_CACHE",			QC_TOKEN_SQL_CACHE},
	{"SQL_NO_CACHE",		QC_TOKEN_SQL_NO_CACHE},
	{"SQL_CALC_FOUND_ROWS",	QC_TOKEN_SQL_CALC_FOUND_ROWS},
	{"LEFT",				QC_TOKEN_LEFT},
	{"RIGHT",				QC_TOKEN_RIGHT},
	{"JOIN",				QC_TOKEN_JOIN},
	{"ON",					QC_TOKEN_ON},
	{"USING",				QC_TOKEN_USING}
};


/* {{{ mysqlnd_ms_get_token */
PHPAPI struct st_qc_token_and_value
mysqlnd_ms_get_token(const char **p, size_t * query_len, const MYSQLND_CHARSET * cset TSRMLS_DC)
{
	const char * orig_p;
	struct st_qc_token_and_value ret;

	DBG_ENTER("mysqlnd_ms_get_token");
	DBG_INF_FMT("p=%s query_len=%d", *p, *query_len);
	ret.token = QC_TOKEN_NO_MORE;

	ZVAL_NULL(&ret.value);
	if (!*query_len) {
		DBG_RETURN(ret);	
	}

	while ((*query_len) && isspace(**p)) {
		++*p;
		(*query_len)--;
	}

	/* c++ comments */
	if ((*query_len) > 1 && **p == '/' && *(*p+1) == '*') {
		++*p; /* skip / */
		++*p; /* skip * */
		(*query_len)-=2;
		orig_p = *p;
		while ((*query_len) > 1 && **p != '*' && (*(*p+1) != '/')) {
			++*p;
			(*query_len)--;
		}
		ret.token = QC_TOKEN_COMMENT;
		ZVAL_STRINGL(&ret.value, orig_p, *p - orig_p, 1);

		if ((*query_len) > 1) {
			++*p;
			++*p;
			(*query_len) -= 2;
		}

		DBG_RETURN(ret);
	}

	/* line comments */
	if ((*query_len) > 1 && **p == '-' && *(*p+1) == '-') {
		++*p; /* skip / */
		++*p; /* skip * */
		(*query_len)-=2;
		orig_p = *p;
		while ((*query_len) && **p != '\n') {
			++*p;
			--(*query_len);
		}

		ret.token = QC_TOKEN_COMMENT;
		ZVAL_STRINGL(&ret.value, orig_p, *p - orig_p, 1);

		if ((*query_len)) {
			++*p;
			--(*query_len);
		}

		DBG_RETURN(ret);
	}

	/* PUNCTUATION */
	if ((*query_len)) {
		switch (**p) {
			case '\\':++*p;--(*query_len);if (*query_len) { ++*p;--(*query_len);}ret.token = QC_TOKEN_UNKNOWN; DBG_RETURN(ret);
			case ';':++*p;--(*query_len);ret.token = QC_TOKEN_SEMICOLON; DBG_RETURN(ret);
			case '.':++*p;--(*query_len);ret.token = QC_TOKEN_DOT; DBG_RETURN(ret);
			case ',':++*p;--(*query_len);ret.token = QC_TOKEN_COMMA; DBG_RETURN(ret);
			case '*':++*p;--(*query_len);ret.token = QC_TOKEN_STAR; DBG_RETURN(ret);
			case '=':++*p;--(*query_len);ret.token = QC_TOKEN_EQ; DBG_RETURN(ret);
			case '+':++*p;--(*query_len);ret.token = QC_TOKEN_PLUS; DBG_RETURN(ret);
			case '-':++*p;--(*query_len);ret.token = QC_TOKEN_MINUS; DBG_RETURN(ret);
			case '%':++*p;--(*query_len);ret.token = QC_TOKEN_MOD; DBG_RETURN(ret);
			case '/':++*p;--(*query_len);ret.token = QC_TOKEN_DIV; DBG_RETURN(ret);
			case '(':++*p;--(*query_len);ret.token = QC_TOKEN_BRACKET_OPEN; DBG_RETURN(ret);
			case ')':++*p;--(*query_len);ret.token = QC_TOKEN_BRACKET_CLOSE; DBG_RETURN(ret);
			case '^':++*p;--(*query_len);ret.token = QC_TOKEN_XOR; DBG_RETURN(ret);
			case '<':
				++*p;
				--(*query_len);
				if (*query_len && **p == '=') {
					++*p;
					--(*query_len);
					ret.token = QC_TOKEN_LE;
					ZVAL_STRINGL(&ret.value, "<=", sizeof("<=") - 1, 1);
				} else {
					ZVAL_STRINGL(&ret.value, "<", sizeof("<") - 1, 1);
					ret.token = QC_TOKEN_LT;
				}
				DBG_RETURN(ret);
			case '>':
				++*p;
				--(*query_len);
				if (*query_len && **p == '=') {
					++*p;
					--(*query_len);
					ret.token = QC_TOKEN_GE;
					ZVAL_STRINGL(&ret.value, ">=", sizeof(">=") - 1, 1);
				} else {
					ZVAL_STRINGL(&ret.value, ">", sizeof(">") - 1, 1);
					ret.token = QC_TOKEN_GT;
				}
				DBG_RETURN(ret);
			case '|':
				++*p;
				--(*query_len);
				if (*query_len && **p == '|') {
					++*p;
					--(*query_len);
					ret.token = QC_TOKEN_OR;
					ZVAL_STRINGL(&ret.value, "OR", sizeof("OR") - 1, 1);
				} else {
					ZVAL_STRINGL(&ret.value, "|", sizeof("|") - 1, 1);
					ret.token = QC_TOKEN_BIT_OR;
				}
				DBG_RETURN(ret);
			case '&':
				++*p;
				--(*query_len);
				if (*query_len && **p == '&') {
					++*p;
					--(*query_len);
					ret.token = QC_TOKEN_AND;
					ZVAL_STRINGL(&ret.value, "AND", sizeof("AND") - 1, 1);
				} else {
					ZVAL_STRINGL(&ret.value, "&", sizeof("&") - 1, 1);
					ret.token = QC_TOKEN_BIT_AND;
				}
				DBG_RETURN(ret);
			case '@':
				++*p;
				--(*query_len);
				if (*query_len && **p == '@') {
					++*p;
					--(*query_len);
					ret.token = QC_TOKEN_GLOBAL_VAR;
					ZVAL_STRINGL(&ret.value, "@@", sizeof("@") - 1, 1);
				} else {
					ZVAL_STRINGL(&ret.value, "@", sizeof("@") - 1, 1);
					ret.token = QC_TOKEN_SESSION_VAR;
				}
				DBG_RETURN(ret);
			default:
				break;
		}
	}

		/* STRINGS */
	if ((*query_len) && (**p == '"' || **p == '\'')) {
		char expected_quote = **p;
		++*p;
		--(*query_len);
		orig_p = *p;
		if (!(*query_len)) {
			DBG_RETURN(ret);
		}
		while ((*query_len) && (**p != expected_quote || (*(*p-1) == '\\'))) {
			++*p;
			(*query_len)--;
		}
		ret.token = QC_TOKEN_STRING;
		ZVAL_STRINGL(&ret.value, orig_p, *p - orig_p, 1);
		if ((*query_len)) {
			++*p;
			--(*query_len);
		}
		DBG_RETURN(ret);
	}


	/* NUMBERS */
	{
		zend_bool is_digit = FALSE;
		zend_bool is_float = FALSE;
		orig_p = *p;
		while ((*query_len) && (isdigit(**p) || **p == '.')) {
			is_digit = TRUE;
			if (**p == '.') {
				is_float = TRUE;
			}
			++*p;
			--(*query_len);
		}
		if (is_digit) {
			char convert_buffer[*p - orig_p + 1];
			memcpy(convert_buffer, orig_p, *p - orig_p);
			convert_buffer[*p - orig_p] = '\0';
			if (is_float) {
				ZVAL_DOUBLE(&ret.value, atof(convert_buffer));
			} else {
				ZVAL_LONG(&ret.value, atol(convert_buffer));
			}
			ret.token = QC_TOKEN_NUM;
			DBG_RETURN(ret);
		}
	}

	/* IDENTIFIERS */
	{
		zend_bool tick_found = FALSE;
		if ((*query_len) && **p == '`') {
			tick_found = TRUE;
			++*p;
			--(*query_len);
		}
		orig_p = *p;

		while ((*query_len) && (isalpha(**p) || isdigit(**p) || **p == '_' || **p == '$' || cset->mb_charlen((zend_uchar)**p) > 1)) {
			size_t mblen = cset->mb_charlen((zend_uchar)**p);
			(*p)+= mblen;
			(*query_len) -= mblen;
		}
		/* MAYBE IT IS A KEYWORD */
		{
			char keyword_buffer[*p - orig_p + 1];
			uint i = 0;
			memcpy(keyword_buffer, orig_p, *p - orig_p);
			keyword_buffer[*p - orig_p] = '\0';
			for (; i < sizeof(static_tokens)/sizeof(static_tokens[0]); i++) {
				if (!strcasecmp(keyword_buffer, static_tokens[i].name)) {
					ret.token = static_tokens[i].value;
					ZVAL_STRINGL(&ret.value, static_tokens[i].name, *p - orig_p, 1);
					DBG_RETURN(ret);
				}
			}
		}

		ZVAL_STRINGL(&ret.value, orig_p, *p - orig_p, 1);
		ret.token = QC_TOKEN_IDENT;

		if ((*query_len) && tick_found && **p == '`') {
			++*p;
			--(*query_len);
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_ms_query_tokenize */
PHPAPI smart_str *
mysqlnd_ms_query_tokenize(const char * const query, size_t query_len TSRMLS_DC)
{
	size_t len = query_len;
	const char * p = query;
	struct st_qc_token_and_value token;
	smart_str * normalized_query = ecalloc(1, sizeof(smart_str));
	smart_str * first_db = ecalloc(1, sizeof(smart_str));
	smart_str * first_table = ecalloc(1, sizeof(smart_str));
	const MYSQLND_CHARSET * cset = mysqlnd_find_charset_name("utf8");

	DBG_ENTER("mysqlnd_ms_query_tokenize");
	DBG_INF_FMT("query=%s", query);
	do {
		zend_bool expect_table_name = FALSE;
		zend_bool expect_dot_part_of_table = FALSE;
		zend_bool db_found = FALSE;
		zend_bool jump_to_skip_init;
skip_init:
		jump_to_skip_init = FALSE;
		token = mysqlnd_ms_get_token(&p, &len, cset TSRMLS_CC);
		DBG_INF_FMT("token=%s", qc_tokens_names[token.token]);
		if (Z_TYPE(token.value) == IS_STRING) {
			DBG_INF_FMT("value=%s", Z_STRVAL(token.value));
		} else 	if (Z_TYPE(token.value) == IS_LONG) {
			DBG_INF_FMT("value=%d", Z_LVAL(token.value));
		} else 	if (Z_TYPE(token.value) == IS_DOUBLE) {
			DBG_INF_FMT("value=%f", Z_DVAL(token.value));
		}

		if (token.token == QC_TOKEN_NO_MORE) {
			break;
		}
		switch (token.token) {
			case QC_TOKEN_ABORT:					 /* INTERNAL (used in lex) */
			case QC_TOKEN_ACCESSIBLE:
			case QC_TOKEN_ACTION:						/* SQL-2003-N */
			case QC_TOKEN_ADD:						   /* SQL-2003-R */
			case QC_TOKEN_ADDDATE:				   /* MYSQL-FUNC */
			case QC_TOKEN_AFTER:					 /* SQL-2003-N */
			case QC_TOKEN_AGAINST:
			case QC_TOKEN_AGGREGATE:
			case QC_TOKEN_ALGORITHM:
			case QC_TOKEN_ALL:						   /* SQL-2003-R */
			case QC_TOKEN_ALTER:						 /* SQL-2003-R */
			case QC_TOKEN_ANALYZE:
			case QC_TOKEN_AND_AND:				   /* OPERATOR */
			case QC_TOKEN_AND:					   /* SQL-2003-R */
			case QC_TOKEN_ANY:					   /* SQL-2003-R */
			case QC_TOKEN_AS:							/* SQL-2003-R */
			case QC_TOKEN_ASC:						   /* SQL-2003-N */
			case QC_TOKEN_ASCII:					 /* MYSQL-FUNC */
			case QC_TOKEN_ASENSITIVE:				/* FUTURE-USE */
			case QC_TOKEN_AT:						/* SQL-2003-R */
			case QC_TOKEN_AUTHORS:
			case QC_TOKEN_AUTOEXTEND_SIZE:
			case QC_TOKEN_AUTO_INC:
			case QC_TOKEN_AVG_ROW_LENGTH:
			case QC_TOKEN_AVG:					   /* SQL-2003-N */
			case QC_TOKEN_BACKUP:
			case QC_TOKEN_BEFORE:					/* SQL-2003-N */
			case QC_TOKEN_BEGIN:					 /* SQL-2003-R */
			case QC_TOKEN_BETWEEN:				   /* SQL-2003-R */
			case QC_TOKEN_BIGINT:						/* SQL-2003-R */
			case QC_TOKEN_BINARY:						/* SQL-2003-R */
			case QC_TOKEN_BINLOG:
			case QC_TOKEN_BIN_NUM:
			case QC_TOKEN_BIT_AND:					   /* MYSQL-FUNC */
			case QC_TOKEN_BIT_OR:						/* MYSQL-FUNC */
			case QC_TOKEN_BIT:					   /* MYSQL-FUNC */
			case QC_TOKEN_BIT_XOR:					   /* MYSQL-FUNC */
			case QC_TOKEN_BLOB:					  /* SQL-2003-R */
			case QC_TOKEN_BLOCK:
			case QC_TOKEN_BOOLEAN:				   /* SQL-2003-R */
			case QC_TOKEN_BOOL:
			case QC_TOKEN_BOTH:						  /* SQL-2003-R */
			case QC_TOKEN_BTREE:
			case QC_TOKEN_BY:							/* SQL-2003-R */
			case QC_TOKEN_BYTE:
			case QC_TOKEN_CACHE:
			case QC_TOKEN_CALL:					  /* SQL-2003-R */
			case QC_TOKEN_CASCADE:					   /* SQL-2003-N */
			case QC_TOKEN_CASCADED:					  /* SQL-2003-R */
			case QC_TOKEN_CASE:						/* SQL-2003-R */
			case QC_TOKEN_CAST:					  /* SQL-2003-R */
			case QC_TOKEN_CATALOG_NAME:			  /* SQL-2003-N */
			case QC_TOKEN_CHAIN:					 /* SQL-2003-N */
			case QC_TOKEN_CHANGE:
			case QC_TOKEN_CHANGED:
			case QC_TOKEN_CHARSET:
			case QC_TOKEN_CHAR:					  /* SQL-2003-R */
			case QC_TOKEN_CHECKSUM:
			case QC_TOKEN_CHECK:					 /* SQL-2003-R */
			case QC_TOKEN_CIPHER:
			case QC_TOKEN_CLASS_ORIGIN:			  /* SQL-2003-N */
			case QC_TOKEN_CLIENT:
			case QC_TOKEN_CLOSE:					 /* SQL-2003-R */
			case QC_TOKEN_COALESCE:					  /* SQL-2003-N */
			case QC_TOKEN_CODE:
			case QC_TOKEN_COLLATE:				   /* SQL-2003-R */
			case QC_TOKEN_COLLATION:				 /* SQL-2003-N */
			case QC_TOKEN_COLUMNS:
			case QC_TOKEN_COLUMN:					/* SQL-2003-R */
			case QC_TOKEN_COLUMN_NAME:			   /* SQL-2003-N */
			case QC_TOKEN_COMMITTED:				 /* SQL-2003-N */
			case QC_TOKEN_COMMIT:					/* SQL-2003-R */
			case QC_TOKEN_COMPACT:
			case QC_TOKEN_COMPLETION:
			case QC_TOKEN_COMPRESSED:
			case QC_TOKEN_CONCURRENT:
			case QC_TOKEN_CONDITION:				 /* SQL-2003-R			case  SQL-2008-R */
			case QC_TOKEN_CONNECTION:
			case QC_TOKEN_CONSISTENT:
			case QC_TOKEN_CONSTRAINT:					/* SQL-2003-R */
			case QC_TOKEN_CONSTRAINT_CATALOG:		/* SQL-2003-N */
			case QC_TOKEN_CONSTRAINT_NAME:		   /* SQL-2003-N */
			case QC_TOKEN_CONSTRAINT_SCHEMA:		 /* SQL-2003-N */
			case QC_TOKEN_CONTAINS:				  /* SQL-2003-N */
			case QC_TOKEN_CONTEXT:
			case QC_TOKEN_CONTINUE:				  /* SQL-2003-R */
			case QC_TOKEN_CONTRIBUTORS:
			case QC_TOKEN_CONVERT:				   /* SQL-2003-N */
			case QC_TOKEN_COUNT:					 /* SQL-2003-N */
			case QC_TOKEN_CPU:
			case QC_TOKEN_CREATE:						/* SQL-2003-R */
			case QC_TOKEN_CROSS:						 /* SQL-2003-R */
			case QC_TOKEN_CUBE:					  /* SQL-2003-R */
			case QC_TOKEN_CURDATE:					   /* MYSQL-FUNC */
			case QC_TOKEN_CURRENT_USER:				  /* SQL-2003-R */
			case QC_TOKEN_CURSOR:					/* SQL-2003-R */
			case QC_TOKEN_CURSOR_NAME:			   /* SQL-2003-N */
			case QC_TOKEN_CURTIME:					   /* MYSQL-FUNC */
			case QC_TOKEN_DATABASE:
			case QC_TOKEN_DATABASES:
			case QC_TOKEN_DATAFILE:
			case QC_TOKEN_DATA:					  /* SQL-2003-N */
			case QC_TOKEN_DATETIME:
			case QC_TOKEN_DATE_ADD_INTERVAL:			 /* MYSQL-FUNC */
			case QC_TOKEN_DATE_SUB_INTERVAL:			 /* MYSQL-FUNC */
			case QC_TOKEN_DATE:					  /* SQL-2003-R */
			case QC_TOKEN_DAY_HOUR:
			case QC_TOKEN_DAY_MICROSECOND:
			case QC_TOKEN_DAY_MINUTE:
			case QC_TOKEN_DAY_SECOND:
			case QC_TOKEN_DAY:					   /* SQL-2003-R */
			case QC_TOKEN_DEALLOCATE:				/* SQL-2003-R */
			case QC_TOKEN_DECIMAL_NUM:
			case QC_TOKEN_DECIMAL:				   /* SQL-2003-R */
			case QC_TOKEN_DECLARE:				   /* SQL-2003-R */
			case QC_TOKEN_DEFAULT:					   /* SQL-2003-R */
			case QC_TOKEN_DEFINER:
			case QC_TOKEN_DELAYED:
			case QC_TOKEN_DELAY_KEY_WRITE:
			case QC_TOKEN_DELETE:					/* SQL-2003-R */
			case QC_TOKEN_DESC:						  /* SQL-2003-N */
			case QC_TOKEN_DESCRIBE:					  /* SQL-2003-R */
			case QC_TOKEN_DES_KEY_FILE:
			case QC_TOKEN_DETERMINISTIC:			 /* SQL-2003-R */
			case QC_TOKEN_DIRECTORY:
			case QC_TOKEN_DISABLE:
			case QC_TOKEN_DISCARD:
			case QC_TOKEN_DISK:
			case QC_TOKEN_DISTINCT:					  /* SQL-2003-R */
			case QC_TOKEN_DOUBLE:					/* SQL-2003-R */
			case QC_TOKEN_DO:
			case QC_TOKEN_DROP:						  /* SQL-2003-R */
			case QC_TOKEN_DUAL:
			case QC_TOKEN_DUMPFILE:
			case QC_TOKEN_DUPLICATE:
			case QC_TOKEN_DYNAMIC:				   /* SQL-2003-R */
			case QC_TOKEN_EACH:					  /* SQL-2003-R */
			case QC_TOKEN_ELSE:						  /* SQL-2003-R */
			case QC_TOKEN_ELSEIF:
			case QC_TOKEN_ENABLE:
			case QC_TOKEN_ENCLOSED:
			case QC_TOKEN_END:						   /* SQL-2003-R */
			case QC_TOKEN_ENDS:
			case QC_TOKEN_END_OF_INPUT:				  /* INTERNAL */
			case QC_TOKEN_ENGINES:
			case QC_TOKEN_ENGINE:
			case QC_TOKEN_ENUM:
			case QC_TOKEN_EQUAL:					 /* OPERATOR */
			case QC_TOKEN_ERRORS:
			case QC_TOKEN_ESCAPED:
			case QC_TOKEN_ESCAPE:					/* SQL-2003-R */
			case QC_TOKEN_EVENTS:
			case QC_TOKEN_EVENT:
			case QC_TOKEN_EVERY:					 /* SQL-2003-N */
			case QC_TOKEN_EXECUTE:				   /* SQL-2003-R */
			case QC_TOKEN_EXISTS:						/* SQL-2003-R */
			case QC_TOKEN_EXIT:
			case QC_TOKEN_EXPANSION:
			case QC_TOKEN_EXTENDED:
			case QC_TOKEN_EXTENT_SIZE:
			case QC_TOKEN_EXTRACT:				   /* SQL-2003-N */
			case QC_TOKEN_FALSE:					 /* SQL-2003-R */
			case QC_TOKEN_FAST:
			case QC_TOKEN_FAULTS:
			case QC_TOKEN_FETCH:					 /* SQL-2003-R */
			case QC_TOKEN_FILE:
			case QC_TOKEN_FIRST:					 /* SQL-2003-N */
			case QC_TOKEN_FIXED:
			case QC_TOKEN_FLOAT_NUM:
			case QC_TOKEN_FLOAT:					 /* SQL-2003-R */
			case QC_TOKEN_FLUSH:
			case QC_TOKEN_FORCE:
			case QC_TOKEN_FOREIGN:					   /* SQL-2003-R */
			case QC_TOKEN_FOR:					   /* SQL-2003-R */
			case QC_TOKEN_FOUND:					 /* SQL-2003-R */
			case QC_TOKEN_FRAC_SECOND:
				goto append;
			case QC_TOKEN_FROM:
				expect_table_name = TRUE;
				jump_to_skip_init = TRUE;
				goto append;
			case QC_TOKEN_FULL:						  /* SQL-2003-R */
			case QC_TOKEN_FULLTEXT:
			case QC_TOKEN_FUNCTION:				  /* SQL-2003-R */
			case QC_TOKEN_GEOMETRYCOLLECTION:
			case QC_TOKEN_GEOMETRY:
			case QC_TOKEN_GET_FORMAT:					/* MYSQL-FUNC */
			case QC_TOKEN_GLOBAL:					/* SQL-2003-R */
			case QC_TOKEN_GRANT:						 /* SQL-2003-R */
			case QC_TOKEN_GRANTS:
			case QC_TOKEN_GROUP:					 /* SQL-2003-R */
			case QC_TOKEN_GROUP_CONCAT:
			case QC_TOKEN_HANDLER:
			case QC_TOKEN_HASH:
			case QC_TOKEN_HAVING:						/* SQL-2003-R */
			case QC_TOKEN_HELP:
			case QC_TOKEN_HEX_NUM:
			case QC_TOKEN_HIGH_PRIORITY:
			case QC_TOKEN_HOST:
			case QC_TOKEN_HOSTS:
			case QC_TOKEN_HOUR_MICROSECOND:
			case QC_TOKEN_HOUR_MINUTE:
			case QC_TOKEN_HOUR_SECOND:
			case QC_TOKEN_HOUR:					  /* SQL-2003-R */
			case QC_TOKEN_IDENTIFIED:
			case QC_TOKEN_IDENT_QUOTED:
			case QC_TOKEN_IF:
			case QC_TOKEN_IGNORE:
			case QC_TOKEN_IGNORE_SERVER_IDS:
			case QC_TOKEN_IMPORT:
			case QC_TOKEN_INDEXES:
			case QC_TOKEN_INDEX:
			case QC_TOKEN_INFILE:
			case QC_TOKEN_INITIAL_SIZE:
			case QC_TOKEN_INNER:					 /* SQL-2003-R */
			case QC_TOKEN_INOUT:					 /* SQL-2003-R */
			case QC_TOKEN_INSENSITIVE:			   /* SQL-2003-R */
			case QC_TOKEN_INSERT:						/* SQL-2003-R */
			case QC_TOKEN_INSERT_METHOD:
			case QC_TOKEN_INSTALL:
			case QC_TOKEN_INTERVAL:				  /* SQL-2003-R */
			case QC_TOKEN_INTO:						  /* SQL-2003-R */
			case QC_TOKEN_INT:					   /* SQL-2003-R */
			case QC_TOKEN_INVOKER:
			case QC_TOKEN_IN:						/* SQL-2003-R */
			case QC_TOKEN_IO:
			case QC_TOKEN_IPC:
			case QC_TOKEN_IS:							/* SQL-2003-R */
			case QC_TOKEN_ISOLATION:					 /* SQL-2003-R */
			case QC_TOKEN_ISSUER:
			case QC_TOKEN_ITERATE:
			case QC_TOKEN_JOIN:					  /* SQL-2003-R */
			case QC_TOKEN_KEYS:
			case QC_TOKEN_KEY_BLOCK_SIZE:
			case QC_TOKEN_KEY:					   /* SQL-2003-N */
			case QC_TOKEN_KILL:
			case QC_TOKEN_LANGUAGE:				  /* SQL-2003-R */
			case QC_TOKEN_LAST:					  /* SQL-2003-N */
			case QC_TOKEN_LEADING:					   /* SQL-2003-R */
			case QC_TOKEN_LEAVES:
			case QC_TOKEN_LEAVE:
			case QC_TOKEN_LEFT:						  /* SQL-2003-R */
			case QC_TOKEN_LESS:
			case QC_TOKEN_LEVEL:
			case QC_TOKEN_LEX_HOSTNAME:
			case QC_TOKEN_LIKE:						  /* SQL-2003-R */
			case QC_TOKEN_LIMIT:
			case QC_TOKEN_LINEAR:
			case QC_TOKEN_LINES:
			case QC_TOKEN_LINESTRING:
			case QC_TOKEN_LIST:
			case QC_TOKEN_LOAD:
			case QC_TOKEN_LOCAL:					 /* SQL-2003-R */
			case QC_TOKEN_LOCATOR:				   /* SQL-2003-N */
			case QC_TOKEN_LOCKS:
			case QC_TOKEN_LOCK:
			case QC_TOKEN_LOGFILE:
			case QC_TOKEN_LOGS:
			case QC_TOKEN_LONGBLOB:
			case QC_TOKEN_LONGTEXT:
			case QC_TOKEN_LONG_NUM:
			case QC_TOKEN_LONG:
			case QC_TOKEN_LOOP:
			case QC_TOKEN_LOW_PRIORITY:
			case QC_TOKEN_MASTER_CONNECT_RETRY:
			case QC_TOKEN_MASTER_HOST:
			case QC_TOKEN_MASTER_LOG_FILE:
			case QC_TOKEN_MASTER_LOG_POS:
			case QC_TOKEN_MASTER_PASSWORD:
			case QC_TOKEN_MASTER_PORT:
			case QC_TOKEN_MASTER_SERVER_ID:
			case QC_TOKEN_MASTER_SSL_CAPATH:
			case QC_TOKEN_MASTER_SSL_CA:
			case QC_TOKEN_MASTER_SSL_CERT:
			case QC_TOKEN_MASTER_SSL_CIPHER:
			case QC_TOKEN_MASTER_SSL_KEY:
			case QC_TOKEN_MASTER_SSL:
			case QC_TOKEN_MASTER_SSL_VERIFY_SERVER_CERT:
			case QC_TOKEN_MASTER:
			case QC_TOKEN_MASTER_USER:
			case QC_TOKEN_MASTER_HEARTBEAT_PERIOD:
			case QC_TOKEN_MATCH:						 /* SQL-2003-R */
			case QC_TOKEN_MAX_CONNECTIONS_PER_HOUR:
			case QC_TOKEN_MAX_QUERIES_PER_HOUR:
			case QC_TOKEN_MAX_ROWS:
			case QC_TOKEN_MAX_SIZE:
			case QC_TOKEN_MAX:					   /* SQL-2003-N */
			case QC_TOKEN_MAX_UPDATES_PER_HOUR:
			case QC_TOKEN_MAX_USER_CONNECTIONS:
			case QC_TOKEN_MAX_VALUE:				 /* SQL-2003-N */
			case QC_TOKEN_MEDIUMBLOB:
			case QC_TOKEN_MEDIUMINT:
			case QC_TOKEN_MEDIUMTEXT:
			case QC_TOKEN_MEDIUM:
			case QC_TOKEN_MEMORY:
			case QC_TOKEN_MERGE:					 /* SQL-2003-R */
			case QC_TOKEN_MESSAGE_TEXT:			  /* SQL-2003-N */
			case QC_TOKEN_MICROSECOND:			   /* MYSQL-FUNC */
			case QC_TOKEN_MIGRATE:
			case QC_TOKEN_MINUTE_MICROSECOND:
			case QC_TOKEN_MINUTE_SECOND:
			case QC_TOKEN_MINUTE:					/* SQL-2003-R */
			case QC_TOKEN_MIN_ROWS:
			case QC_TOKEN_MIN:					   /* SQL-2003-N */
			case QC_TOKEN_MODE:
			case QC_TOKEN_MODIFIES:				  /* SQL-2003-R */
			case QC_TOKEN_MODIFY:
			case QC_TOKEN_MONTH:					 /* SQL-2003-R */
			case QC_TOKEN_MULTILINESTRING:
			case QC_TOKEN_MULTIPOINT:
			case QC_TOKEN_MULTIPOLYGON:
			case QC_TOKEN_MUTEX:
			case QC_TOKEN_MYSQL_ERRNO:
			case QC_TOKEN_NAMES:					 /* SQL-2003-N */
			case QC_TOKEN_NAME:					  /* SQL-2003-N */
			case QC_TOKEN_NATIONAL:				  /* SQL-2003-R */
			case QC_TOKEN_NATURAL:					   /* SQL-2003-R */
			case QC_TOKEN_NCHAR_STRING:
			case QC_TOKEN_NCHAR:					 /* SQL-2003-R */
			case QC_TOKEN_NDBCLUSTER:
			case QC_TOKEN_NE:							/* OPERATOR */
			case QC_TOKEN_NEG:
			case QC_TOKEN_NEW:					   /* SQL-2003-R */
			case QC_TOKEN_NEXT:					  /* SQL-2003-N */
			case QC_TOKEN_NODEGROUP:
			case QC_TOKEN_NONE:					  /* SQL-2003-R */
			case QC_TOKEN_NOT2:
			case QC_TOKEN_NOT:					   /* SQL-2003-R */
			case QC_TOKEN_NOW:
			case QC_TOKEN_NO:						/* SQL-2003-R */
			case QC_TOKEN_NO_WAIT:
			case QC_TOKEN_NO_WRITE_TO_BINLOG:
			case QC_TOKEN_NULL:					  /* SQL-2003-R */
			case QC_TOKEN_NUMERIC:				   /* SQL-2003-R */
			case QC_TOKEN_NVARCHAR:
			case QC_TOKEN_OFFSET:
			case QC_TOKEN_OLD_PASSWORD:
			case QC_TOKEN_ON:							/* SQL-2003-R */
			case QC_TOKEN_ONE_SHOT:
			case QC_TOKEN_ONE:
			case QC_TOKEN_OPEN:					  /* SQL-2003-R */
			case QC_TOKEN_OPTIMIZE:
			case QC_TOKEN_OPTIONS:
			case QC_TOKEN_OPTION:						/* SQL-2003-N */
			case QC_TOKEN_OPTIONALLY:
			case QC_TOKEN_OR2:
			case QC_TOKEN_ORDER:					 /* SQL-2003-R */
			case QC_TOKEN_OR_OR:					 /* OPERATOR */
			case QC_TOKEN_OR:						/* SQL-2003-R */
			case QC_TOKEN_OUTER:
			case QC_TOKEN_OUTFILE:
			case QC_TOKEN_OUT:					   /* SQL-2003-R */
			case QC_TOKEN_OWNER:
			case QC_TOKEN_PACK_KEYS:
			case QC_TOKEN_PAGE:
			case QC_TOKEN_PARAM_MARKER:
			case QC_TOKEN_PARSER:
			case QC_TOKEN_PARTIAL:					   /* SQL-2003-N */
			case QC_TOKEN_PARTITIONING:
			case QC_TOKEN_PARTITIONS:
			case QC_TOKEN_PARTITION:				 /* SQL-2003-R */
			case QC_TOKEN_PASSWORD:
			case QC_TOKEN_PHASE:
			case QC_TOKEN_PLUGINS:
			case QC_TOKEN_PLUGIN:
			case QC_TOKEN_POINT:
			case QC_TOKEN_POLYGON:
			case QC_TOKEN_PORT:
			case QC_TOKEN_POSITION:				  /* SQL-2003-N */
			case QC_TOKEN_PRECISION:					 /* SQL-2003-R */
			case QC_TOKEN_PREPARE:				   /* SQL-2003-R */
			case QC_TOKEN_PRESERVE:
			case QC_TOKEN_PREV:
			case QC_TOKEN_PRIMARY:				   /* SQL-2003-R */
			case QC_TOKEN_PRIVILEGES:					/* SQL-2003-N */
			case QC_TOKEN_PROCEDURE:					 /* SQL-2003-R */
			case QC_TOKEN_PROCESS:
			case QC_TOKEN_PROCESSLIST:
			case QC_TOKEN_PROFILE:
			case QC_TOKEN_PROFILES:
			case QC_TOKEN_PURGE:
			case QC_TOKEN_QUARTER:
			case QC_TOKEN_QUERY:
			case QC_TOKEN_QUICK:
			case QC_TOKEN_RANGE:					 /* SQL-2003-R */
			case QC_TOKEN_READS:					 /* SQL-2003-R */
			case QC_TOKEN_READ_ONLY:
			case QC_TOKEN_READ:					  /* SQL-2003-N */
			case QC_TOKEN_READ_WRITE:
			case QC_TOKEN_REAL:						  /* SQL-2003-R */
			case QC_TOKEN_REBUILD:
			case QC_TOKEN_RECOVER:
			case QC_TOKEN_REDOFILE:
			case QC_TOKEN_REDO_BUFFER_SIZE:
			case QC_TOKEN_REDUNDANT:
			case QC_TOKEN_REFERENCES:					/* SQL-2003-R */
			case QC_TOKEN_REGEXP:
			case QC_TOKEN_RELAYLOG:
			case QC_TOKEN_RELAY_LOG_FILE:
			case QC_TOKEN_RELAY_LOG_POS:
			case QC_TOKEN_RELAY_THREAD:
			case QC_TOKEN_RELEASE:				   /* SQL-2003-R */
			case QC_TOKEN_RELOAD:
			case QC_TOKEN_REMOVE:
			case QC_TOKEN_RENAME:
			case QC_TOKEN_REORGANIZE:
			case QC_TOKEN_REPAIR:
			case QC_TOKEN_REPEATABLE:				/* SQL-2003-N */
			case QC_TOKEN_REPEAT:					/* MYSQL-FUNC */
			case QC_TOKEN_REPLACE:					   /* MYSQL-FUNC */
			case QC_TOKEN_REPLICATION:
			case QC_TOKEN_REQUIRE:
			case QC_TOKEN_RESET:
			case QC_TOKEN_RESIGNAL:				  /* SQL-2003-R */
			case QC_TOKEN_RESOURCES:
			case QC_TOKEN_RESTORE:
			case QC_TOKEN_RESTRICT:
			case QC_TOKEN_RESUME:
			case QC_TOKEN_RETURNS:				   /* SQL-2003-R */
			case QC_TOKEN_RETURN:					/* SQL-2003-R */
			case QC_TOKEN_REVOKE:						/* SQL-2003-R */
			case QC_TOKEN_RIGHT:						 /* SQL-2003-R */
			case QC_TOKEN_ROLLBACK:				  /* SQL-2003-R */
			case QC_TOKEN_ROLLUP:					/* SQL-2003-R */
			case QC_TOKEN_ROUTINE:				   /* SQL-2003-N */
			case QC_TOKEN_ROWS:					  /* SQL-2003-R */
			case QC_TOKEN_ROW_FORMAT:
			case QC_TOKEN_ROW:					   /* SQL-2003-R */
			case QC_TOKEN_RTREE:
			case QC_TOKEN_SAVEPOINT:				 /* SQL-2003-R */
			case QC_TOKEN_SCHEDULE:
			case QC_TOKEN_SCHEMA_NAME:			   /* SQL-2003-N */
			case QC_TOKEN_SECOND_MICROSECOND:
			case QC_TOKEN_SECOND:					/* SQL-2003-R */
			case QC_TOKEN_SECURITY:				  /* SQL-2003-N */
			case QC_TOKEN_SELECT:					/* SQL-2003-R */
			case QC_TOKEN_SENSITIVE:				 /* FUTURE-USE */
			case QC_TOKEN_SEPARATOR:
			case QC_TOKEN_SERIALIZABLE:			  /* SQL-2003-N */
			case QC_TOKEN_SERIAL:
			case QC_TOKEN_SESSION:				   /* SQL-2003-N */
			case QC_TOKEN_SERVER:
			case QC_TOKEN_SERVER_OPTIONS:
			case QC_TOKEN_SET:						   /* SQL-2003-R */
			case QC_TOKEN_SET_VAR:
			case QC_TOKEN_SHARE:
			case QC_TOKEN_SHIFT_LEFT:					/* OPERATOR */
			case QC_TOKEN_SHIFT_RIGHT:				   /* OPERATOR */
			case QC_TOKEN_SHOW:
			case QC_TOKEN_SHUTDOWN:
			case QC_TOKEN_SIGNAL:					/* SQL-2003-R */
			case QC_TOKEN_SIGNED:
			case QC_TOKEN_SIMPLE:					/* SQL-2003-N */
			case QC_TOKEN_SLAVE:
			case QC_TOKEN_SMALLINT:					  /* SQL-2003-R */
			case QC_TOKEN_SNAPSHOT:
			case QC_TOKEN_SOCKET:
			case QC_TOKEN_SONAME:
			case QC_TOKEN_SOUNDS:
			case QC_TOKEN_SOURCE:
			case QC_TOKEN_SPATIAL:
			case QC_TOKEN_SPECIFIC:				  /* SQL-2003-R */
			case QC_TOKEN_SQLEXCEPTION:			  /* SQL-2003-R */
			case QC_TOKEN_SQLSTATE:				  /* SQL-2003-R */
			case QC_TOKEN_SQLWARNING:				/* SQL-2003-R */
			case QC_TOKEN_SQL_BIG_RESULT:
			case QC_TOKEN_SQL_BUFFER_RESULT:
			case QC_TOKEN_SQL_CACHE:
			case QC_TOKEN_SQL_CALC_FOUND_ROWS:
			case QC_TOKEN_SQL_NO_CACHE:
			case QC_TOKEN_SQL_SMALL_RESULT:
			case QC_TOKEN_SQL:					   /* SQL-2003-R */
			case QC_TOKEN_SQL_THREAD:
			case QC_TOKEN_SSL:
			case QC_TOKEN_STARTING:
			case QC_TOKEN_STARTS:
			case QC_TOKEN_START:					 /* SQL-2003-R */
			case QC_TOKEN_STATUS:
			case QC_TOKEN_STDDEV_SAMP:			   /* SQL-2003-N */
			case QC_TOKEN_STD:
			case QC_TOKEN_STOP:
			case QC_TOKEN_STORAGE:
			case QC_TOKEN_STRAIGHT_JOIN:
			case QC_TOKEN_SUBCLASS_ORIGIN:		   /* SQL-2003-N */
			case QC_TOKEN_SUBDATE:
			case QC_TOKEN_SUBJECT:
			case QC_TOKEN_SUBPARTITIONS:
			case QC_TOKEN_SUBPARTITION:
			case QC_TOKEN_SUBSTRING:					 /* SQL-2003-N */
			case QC_TOKEN_SUM:					   /* SQL-2003-N */
			case QC_TOKEN_SUPER:
			case QC_TOKEN_SUSPEND:
			case QC_TOKEN_SWAPS:
			case QC_TOKEN_SWITCHES:
			case QC_TOKEN_SYSDATE:
			case QC_TOKEN_TABLES:
			case QC_TOKEN_TABLESPACE:
			case QC_TOKEN_TABLE_REF_PRIORITY:
			case QC_TOKEN_TABLE:					 /* SQL-2003-R */
			case QC_TOKEN_TABLE_CHECKSUM:
			case QC_TOKEN_TABLE_NAME:				/* SQL-2003-N */
			case QC_TOKEN_TEMPORARY:					 /* SQL-2003-N */
			case QC_TOKEN_TEMPTABLE:
			case QC_TOKEN_TERMINATED:
			case QC_TOKEN_TEXT_STRING:
			case QC_TOKEN_TEXT:
			case QC_TOKEN_THAN:
			case QC_TOKEN_THEN:					  /* SQL-2003-R */
			case QC_TOKEN_TIMESTAMP:					 /* SQL-2003-R */
			case QC_TOKEN_TIMESTAMP_ADD:
			case QC_TOKEN_TIMESTAMP_DIFF:
			case QC_TOKEN_TIME:					  /* SQL-2003-R */
			case QC_TOKEN_TINYBLOB:
			case QC_TOKEN_TINYINT:
			case QC_TOKEN_TINYTEXT:
			case QC_TOKEN_TO:						/* SQL-2003-R */
			case QC_TOKEN_TRAILING:					  /* SQL-2003-R */
			case QC_TOKEN_TRANSACTION:
			case QC_TOKEN_TRIGGERS:
			case QC_TOKEN_TRIGGER:				   /* SQL-2003-R */
			case QC_TOKEN_TRIM:						  /* SQL-2003-N */
			case QC_TOKEN_TRUE:					  /* SQL-2003-R */
			case QC_TOKEN_TRUNCATE:
			case QC_TOKEN_TYPES:
			case QC_TOKEN_TYPE:					  /* SQL-2003-N */
			case QC_TOKEN_UDF_RETURNS:
			case QC_TOKEN_ULONGLONG_NUM:
			case QC_TOKEN_UNCOMMITTED:			   /* SQL-2003-N */
			case QC_TOKEN_UNDEFINED:
			case QC_TOKEN_UNDERSCORE_CHARSET:
			case QC_TOKEN_UNDOFILE:
			case QC_TOKEN_UNDO_BUFFER_SIZE:
			case QC_TOKEN_UNDO:					  /* FUTURE-USE */
			case QC_TOKEN_UNICODE:
			case QC_TOKEN_UNINSTALL:
			case QC_TOKEN_UNION:					 /* SQL-2003-R */
			case QC_TOKEN_UNIQUE:
			case QC_TOKEN_UNLOCK:
			case QC_TOKEN_UNSIGNED:
			case QC_TOKEN_UNTIL:
			case QC_TOKEN_UPDATE:					/* SQL-2003-R */
			case QC_TOKEN_UPGRADE:
			case QC_TOKEN_USAGE:						 /* SQL-2003-N */
			case QC_TOKEN_USER:						  /* SQL-2003-R */
			case QC_TOKEN_USE_FRM:
			case QC_TOKEN_USE:
			case QC_TOKEN_USING:						 /* SQL-2003-R */
			case QC_TOKEN_UTC_DATE:
			case QC_TOKEN_UTC_TIMESTAMP:
			case QC_TOKEN_UTC_TIME:
			case QC_TOKEN_VALUES:						/* SQL-2003-R */
			case QC_TOKEN_VALUE:					 /* SQL-2003-R */
			case QC_TOKEN_VARBINARY:
			case QC_TOKEN_VARCHAR:					   /* SQL-2003-R */
			case QC_TOKEN_VARIABLES:
			case QC_TOKEN_VARIANCE:
			case QC_TOKEN_VARYING:					   /* SQL-2003-R */
			case QC_TOKEN_VAR_SAMP:
			case QC_TOKEN_VIEW:					  /* SQL-2003-N */
			case QC_TOKEN_WAIT:
			case QC_TOKEN_WARNINGS:
			case QC_TOKEN_WEEK:
			case QC_TOKEN_WHEN:					  /* SQL-2003-R */
			case QC_TOKEN_WHERE:						 /* SQL-2003-R */
			case QC_TOKEN_WHILE:
			case QC_TOKEN_WITH:						  /* SQL-2003-R */
			case QC_TOKEN_WITH_CUBE:				 /* INTERNAL */
			case QC_TOKEN_WITH_ROLLUP:			   /* INTERNAL */
			case QC_TOKEN_WORK:					  /* SQL-2003-N */
			case QC_TOKEN_WRAPPER:
			case QC_TOKEN_WRITE:					 /* SQL-2003-N */
			case QC_TOKEN_X509:
			case QC_TOKEN_XA:
			case QC_TOKEN_XML:
			case QC_TOKEN_YEAR_MONTH:
			case QC_TOKEN_YEAR:					  /* SQL-2003-R */
			case QC_TOKEN_ZEROFILL:
append:
				smart_str_appendc(normalized_query, ' ');
				smart_str_appendl_ex(normalized_query, Z_STRVAL(token.value), Z_STRLEN(token.value), 0);
				smart_str_appendc(normalized_query, ' ');
				break;
			case QC_TOKEN_GLOBAL_VAR:
				smart_str_appendl_ex(normalized_query, "@@", sizeof("@@") - 1, 0);
				break;
			case QC_TOKEN_SESSION_VAR:
				smart_str_appendc(normalized_query, '@');
				break;
			case QC_TOKEN_STAR:
				smart_str_appendl_ex(normalized_query, "*", sizeof("*") - 1, 0);
				break;
			case QC_TOKEN_EQ:
				smart_str_appendl_ex(normalized_query, "=", sizeof("=") - 1, 0);
				break;
			case QC_TOKEN_DOT:
				smart_str_appendl_ex(normalized_query, ".", sizeof(".") - 1, 0);
				if (TRUE == expect_dot_part_of_table) {
					/* move table to db */
					smart_str * tmp = first_db;
					first_db = first_table;
					first_table = tmp;
					db_found = TRUE;
					jump_to_skip_init = TRUE;					
				}
				break;
			case QC_TOKEN_XOR:
				smart_str_appendl_ex(normalized_query, " ^ ", sizeof(" ^ ") - 1, 0);
				break;
			case QC_TOKEN_PLUS:
				smart_str_appendl_ex(normalized_query, " + ", sizeof(" + ") - 1, 0);
				break;
			case QC_TOKEN_MINUS:
				smart_str_appendl_ex(normalized_query, " - ", sizeof(" - ") - 1, 0);
				break;
			case QC_TOKEN_MOD:
				smart_str_appendl_ex(normalized_query, " % ", sizeof(" % ") - 1, 0);
				break;
			case QC_TOKEN_DIV:
				smart_str_appendl_ex(normalized_query, " / ", sizeof(" / ") - 1, 0);
				break;
			case QC_TOKEN_COMMA:
				smart_str_appendl_ex(normalized_query, ",", sizeof(",") - 1, 0);
				break;
			case QC_TOKEN_BRACKET_OPEN:
				smart_str_appendl_ex(normalized_query, "(", sizeof("(") - 1, 0);
				break;
			case QC_TOKEN_BRACKET_CLOSE:
				smart_str_appendl_ex(normalized_query, ") ", sizeof(") ") - 1, 0);
				break;
			case QC_TOKEN_LT:
				smart_str_appendl_ex(normalized_query, " < ", sizeof(" < ") - 1, 0);
				break;
			case QC_TOKEN_LE:
				smart_str_appendl_ex(normalized_query, " <= ", sizeof(" <= ") - 1, 0);
				break;
			case QC_TOKEN_GT:
				smart_str_appendl_ex(normalized_query, " > ", sizeof(" > ") - 1, 0);
				break;
			case QC_TOKEN_GE:
				smart_str_appendl_ex(normalized_query, " >= ", sizeof(" >= ") - 1, 0);
				break;
			case QC_TOKEN_STRING:
				smart_str_appendl_ex(normalized_query, "? ", sizeof("? ") - 1, 0);
#if 0
				smart_str_appendl_ex(normalized_query, " \" ", sizeof(" \" ") - 1, 0);
				smart_str_appendl_ex(normalized_query, Z_STRVAL(token.value), Z_STRLEN(token.value), 0);
				smart_str_appendl_ex(normalized_query, " \" ", sizeof(" \" ") - 1, 0);
#endif
				break;
			case QC_TOKEN_NUM:
				smart_str_appendl_ex(normalized_query, "? ", sizeof("? ") - 1, 0);
#if 0
				convert_to_string(&token.value);
				smart_str_appendl_ex(normalized_query, Z_STRVAL(token.value), Z_STRLEN(token.value), 0);
				smart_str_appendc(normalized_query, ' ');
#endif
				break;
			case QC_TOKEN_IDENT:
//				smart_str_appendl_ex(normalized_query, "?", sizeof("?") - 1, 0);
#if 1
				smart_str_appendl_ex(normalized_query, Z_STRVAL(token.value), Z_STRLEN(token.value), 0);
#endif
				if (TRUE == expect_table_name) {
					smart_str_appendl_ex(first_table, Z_STRVAL(token.value), Z_STRLEN(token.value), 0);
					smart_str_appendc(first_table, '\0');
					if (FALSE == db_found) {
						expect_dot_part_of_table = TRUE;
						jump_to_skip_init = TRUE;
					}
				}
				break;
			case QC_TOKEN_COMMENT:
				break;
			case QC_TOKEN_SEMICOLON:
				smart_str_appendc(normalized_query, ';');
				break;
			case QC_TOKEN_CLIENT_FLAG:
			case QC_TOKEN_UNKNOWN:
				break;
			case QC_TOKEN_NO_MORE:
				/* doesn't happen */
				break;
		}
		if (Z_TYPE(token.value) != IS_NULL) {
			zval_dtor(&token.value);
		}
		if (TRUE == jump_to_skip_init) {
			goto skip_init;
		}
	} while (1);
	smart_str_appendc(normalized_query, '\0');
	DBG_INF_FMT("normalized_query=%*s", normalized_query->len - 1, normalized_query->c? normalized_query->c:"");
	if (first_db) {
		DBG_INF_FMT("first_db=%s", first_db->c);
		smart_str_free_ex(first_db, 0);
		efree(first_db);
	}
	if (first_table) {
		DBG_INF_FMT("first_table=%s", first_table->c);
		smart_str_free_ex(first_table, 0);
		efree(first_table);
	}
	
	DBG_RETURN(normalized_query);
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
