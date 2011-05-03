%{
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

/*
Compile with : flex -8 -o mysqlnd_tok.c --reentrant --prefix mysqlnd_tok_ mysqlnd_tok.flex
*/

#include "mysqlnd_tok_def.h"
#include <string.h>
#include "php.h"
#include "php_ini.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_priv.h"
#include "mysqlnd_ms.h"

int old_yystate;

#define yyerror zend_printf
#define YY_NO_INPUT

/* In Unicode . is [\0-\x7F] | [\xC2-\xDF][\x80-\xBF] | \xE0[\xA0-\xBF][\x80-\xBF] | [\xE1-\xEF][\x80-\xBF][\x80-\xBF] */

%}

%option reentrant noyywrap nounput
%option extra-type="zval *"
%option bison-bridge

%x COMMENT_MODE
%s BETWEEN_MODE

%%
%{
	zval * token_value = yyextra;
%}

(?i:ACCESSIBLE)						{ return QC_TOKEN_ACCESSIBLE; }
(?i:ACTION)							{ return QC_TOKEN_ACTION; }
(?i:ADD)							{ return QC_TOKEN_ADD; }
(?i:ADDDATE)						{ return QC_TOKEN_ADDDATE; }
(?i:AFTER)							{ return QC_TOKEN_AFTER; }
(?i:AGAINST)						{ return QC_TOKEN_AGAINST; }
(?i:AGGREGATE)						{ return QC_TOKEN_AGGREGATE; }
(?i:ALGORITHM)						{ return QC_TOKEN_ALGORITHM; }
(?i:ALL)							{ return QC_TOKEN_ALL; }
(?i:ALTER)							{ return QC_TOKEN_ALTER; }
(?i:ANALYZE)						{ return QC_TOKEN_ANALYZE; }
<BETWEEN_MODE>AND					{ BEGIN INITIAL; return QC_TOKEN_BETWEEN_AND; }
(?i:AND)							{ return QC_TOKEN_AND; }
(?i:ANY)							{ return QC_TOKEN_ANY; }
(?i:AS)								{ return QC_TOKEN_AS; }
(?i:ASC)							{ return QC_TOKEN_ASC; }
(?i:ASCII)							{ return QC_TOKEN_ASCII; }
(?i:ASENSITIVE)						{ return QC_TOKEN_ASENSITIVE; }
(?i:AT)								{ return QC_TOKEN_AT; }
(?i:AUTHORS)						{ return QC_TOKEN_AUTHORS; }
(?i:AUTOEXTEND_SIZE)				{ return QC_TOKEN_AUTOEXTEND_SIZE; }
(?i:AUTO_INC)						{ return QC_TOKEN_AUTO_INC; }
(?i:AVG_ROW_LENGTH)					{ return QC_TOKEN_AVG_ROW_LENGTH; }
(?i:AVG)							{ return QC_TOKEN_AVG; }
(?i:BACKUP)							{ return QC_TOKEN_BACKUP; }
(?i:BEFORE)							{ return QC_TOKEN_BEFORE; }
(?i:BEGIN)							{ return QC_TOKEN_BEGIN; }
(?i:BETWEEN)						{ BEGIN BETWEEN_MODE; return QC_TOKEN_BETWEEN; }
(?i:BIGINT)							{ return QC_TOKEN_BIGINT; }
(?i:BINARY)							{ return QC_TOKEN_BINARY; }
(?i:BINLOG)							{ return QC_TOKEN_BINLOG; }
(?i:BIT)							{ return QC_TOKEN_BIT; }
(?i:BLOB)							{ return QC_TOKEN_BLOB; }
(?i:BLOCK)							{ return QC_TOKEN_BLOCK; }
(?i:BOOLEAN)						{ return QC_TOKEN_BOOLEAN; }
(?i:BOOL)							{ return QC_TOKEN_BOOL; }
(?i:BOTH)							{ return QC_TOKEN_BOTH; }
(?i:BTREE)							{ return QC_TOKEN_BTREE; }
(?i:BY)								{ return QC_TOKEN_BY; }
(?i:BYTE)							{ return QC_TOKEN_BYTE; }
(?i:CACHE)							{ return QC_TOKEN_CACHE; }
(?i:CALL)							{ return QC_TOKEN_CALL; }
(?i:CASCADE)						{ return QC_TOKEN_CASCADE; }
(?i:CASCADED)						{ return QC_TOKEN_CASCADED; }
(?i:CASE)							{ return QC_TOKEN_CASE; }
(?i:CAST)							{ return QC_TOKEN_CAST; }
(?i:CATALOG_NAME)					{ return QC_TOKEN_CATALOG_NAME; }
(?i:CHAIN)							{ return QC_TOKEN_CHAIN; }
(?i:CHANGE)							{ return QC_TOKEN_CHANGE; }
(?i:CHANGED)						{ return QC_TOKEN_CHANGED; }
(?i:CHARSET)						{ return QC_TOKEN_CHARSET; }
(?i:CHAR)							{ return QC_TOKEN_CHAR; }
(?i:CHECKSUM)						{ return QC_TOKEN_CHECKSUM; }
(?i:CHECK)							{ return QC_TOKEN_CHECK; }
(?i:CIPHER)							{ return QC_TOKEN_CIPHER; }
(?i:CLASS_ORIGIN)					{ return QC_TOKEN_CLASS_ORIGIN; }
(?i:CLIENT)							{ return QC_TOKEN_CLIENT; }
(?i:CLOSE)							{ return QC_TOKEN_CLOSE; }
(?i:COALESCE)						{ return QC_TOKEN_COALESCE; }
(?i:CODE)							{ return QC_TOKEN_CODE; }
(?i:COLLATE)						{ return QC_TOKEN_COLLATE; }
(?i:COLLATION)						{ return QC_TOKEN_COLLATION; }
(?i:COLUMNS)						{ return QC_TOKEN_COLUMNS; }
(?i:COLUMN)							{ return QC_TOKEN_COLUMN; }
(?i:COLUMN_NAME)					{ return QC_TOKEN_COLUMN_NAME; }
(?i:COMMENT)						{ return QC_TOKEN_COMMENT; }
(?i:COMMITTED)						{ return QC_TOKEN_COMMITTED; }
(?i:COMMIT)							{ return QC_TOKEN_COMMIT; }
(?i:COMPACT)						{ return QC_TOKEN_COMPACT; }
(?i:COMPLETION)						{ return QC_TOKEN_COMPLETION; }
(?i:COMPRESSED)						{ return QC_TOKEN_COMPRESSED; }
(?i:CONCURRENT)						{ return QC_TOKEN_CONCURRENT; }
(?i:CONDITION)						{ return QC_TOKEN_CONDITION; }
(?i:CONNECTION)						{ return QC_TOKEN_CONNECTION; }
(?i:CONSISTENT)						{ return QC_TOKEN_CONSISTENT; }
(?i:CONSTRAINT)						{ return QC_TOKEN_CONSTRAINT; }
(?i:CONSTRAINT_CATALOG)				{ return QC_TOKEN_CONSTRAINT_CATALOG; }
(?i:CONSTRAINT_NAME)				{ return QC_TOKEN_CONSTRAINT_NAME; }
(?i:CONSTRAINT_SCHEMA)				{ return QC_TOKEN_CONSTRAINT_SCHEMA; }
(?i:CONTAINS)						{ return QC_TOKEN_CONTAINS; }
(?i:CONTEXT)						{ return QC_TOKEN_CONTEXT; }
(?i:CONTINUE)						{ return QC_TOKEN_CONTINUE; }
(?i:CONTRIBUTORS)					{ return QC_TOKEN_CONTRIBUTORS; }
(?i:CONVERT)						{ return QC_TOKEN_CONVERT; }
(?i:COUNT)							{ return QC_TOKEN_COUNT; }
(?i:CPU)							{ return QC_TOKEN_CPU; }
(?i:CREATE)							{ return QC_TOKEN_CREATE; }
(?i:CROSS)							{ return QC_TOKEN_CROSS; }
(?i:CUBE)							{ return QC_TOKEN_CUBE; }
(?i:CURDATE)						{ return QC_TOKEN_CURDATE; }
(?i:CURRENT_USER)					{ return QC_TOKEN_CURRENT_USER; }
(?i:CURSOR)							{ return QC_TOKEN_CURSOR; }
(?i:CURSOR_NAME)					{ return QC_TOKEN_CURSOR_NAME; }
(?i:CURTIME)						{ return QC_TOKEN_CURTIME; }
(?i:DATABASE)						{ return QC_TOKEN_DATABASE; }
(?i:DATABASES)						{ return QC_TOKEN_DATABASES; }
(?i:DATAFILE)						{ return QC_TOKEN_DATAFILE; }
(?i:DATA)							{ return QC_TOKEN_DATA; }
(?i:DATETIME)						{ return QC_TOKEN_DATETIME; }
(?i:DATE_ADD_INTERVAL)				{ return QC_TOKEN_DATE_ADD_INTERVAL; }
(?i:DATE_SUB_INTERVAL)				{ return QC_TOKEN_DATE_SUB_INTERVAL; }
(?i:DATE)							{ return QC_TOKEN_DATE; }
(?i:DAY_HOUR)						{ return QC_TOKEN_DAY_HOUR; }
(?i:DAY_MICROSECOND)				{ return QC_TOKEN_DAY_MICROSECOND; }
(?i:DAY_MINUTE)						{ return QC_TOKEN_DAY_MINUTE; }
(?i:DAY_SECOND)						{ return QC_TOKEN_DAY_SECOND; }
(?i:DAY)							{ return QC_TOKEN_DAY; }
(?i:DEALLOCATE)						{ return QC_TOKEN_DEALLOCATE; }
(?i:DECIMAL_NUM)					{ return QC_TOKEN_DECIMAL_NUM; }
(?i:DECIMAL)						{ return QC_TOKEN_DECIMAL; }
(?i:DECLARE)						{ return QC_TOKEN_DECLARE; }
(?i:DEFAULT)						{ return QC_TOKEN_DEFAULT; }
(?i:DEFINER)						{ return QC_TOKEN_DEFINER; }
(?i:DELAYED)						{ return QC_TOKEN_DELAYED; }
(?i:DELAY_KEY_WRITE)				{ return QC_TOKEN_DELAY_KEY_WRITE; }
(?i:DELETE)							{ return QC_TOKEN_DELETE; }
(?i:DESC)							{ return QC_TOKEN_DESC; }
(?i:DESCRIBE)						{ return QC_TOKEN_DESCRIBE; }
(?i:DES_KEY_FILE)					{ return QC_TOKEN_DES_KEY_FILE; }
(?i:DETERMINISTIC)					{ return QC_TOKEN_DETERMINISTIC; }
(?i:DIRECTORY)						{ return QC_TOKEN_DIRECTORY; }
(?i:DISABLE)						{ return QC_TOKEN_DISABLE; }
(?i:DISCARD)						{ return QC_TOKEN_DISCARD; }
(?i:DISK)							{ return QC_TOKEN_DISK; }
(?i:DISTINCT)						{ return QC_TOKEN_DISTINCT; }
(?i:DIV)							{ return QC_TOKEN_DIV; }
(?i:DOUBLE)							{ return QC_TOKEN_DOUBLE; }
(?i:DO)								{ return QC_TOKEN_DO; }
(?i:DROP)							{ return QC_TOKEN_DROP; }
(?i:DUAL)							{ return QC_TOKEN_DUAL; }
(?i:DUMPFILE)						{ return QC_TOKEN_DUMPFILE; }
(?i:DUPLICATE)						{ return QC_TOKEN_DUPLICATE; }
(?i:DYNAMIC)						{ return QC_TOKEN_DYNAMIC; }
(?i:EACH)							{ return QC_TOKEN_EACH; }
(?i:ELSE)							{ return QC_TOKEN_ELSE; }
(?i:ELSEIF)							{ return QC_TOKEN_ELSEIF; }
(?i:ENABLE)							{ return QC_TOKEN_ENABLE; }
(?i:ENCLOSED)						{ return QC_TOKEN_ENCLOSED; }
(?i:END)							{ return QC_TOKEN_END; }
(?i:ENDS)							{ return QC_TOKEN_ENDS; }
(?i:ENGINES)						{ return QC_TOKEN_ENGINES; }
(?i:ENGINE)							{ return QC_TOKEN_ENGINE; }
(?i:ENUM)							{ return QC_TOKEN_ENUM; }
(?i:EQUAL)							{ return QC_TOKEN_EQUAL; }
(?i:ERRORS)							{ return QC_TOKEN_ERRORS; }
(?i:ESCAPED)						{ return QC_TOKEN_ESCAPED; }
(?i:ESCAPE)							{ return QC_TOKEN_ESCAPE; }
(?i:EVENTS)							{ return QC_TOKEN_EVENTS; }
(?i:EVENT)							{ return QC_TOKEN_EVENT; }
(?i:EVERY)							{ return QC_TOKEN_EVERY; }
(?i:EXECUTE)						{ return QC_TOKEN_EXECUTE; }
(?i:EXISTS)							{ return QC_TOKEN_EXISTS; }
(?i:EXIT)							{ return QC_TOKEN_EXIT; }
(?i:EXPANSION)						{ return QC_TOKEN_EXPANSION; }
(?i:EXTENDED)						{ return QC_TOKEN_EXTENDED; }
(?i:EXTENT_SIZE)					{ return QC_TOKEN_EXTENT_SIZE; }
(?i:EXTRACT)						{ return QC_TOKEN_EXTRACT; }
(?i:FALSE)							{ return QC_TOKEN_FALSE; }
(?i:FAST)							{ return QC_TOKEN_FAST; }
(?i:FAULTS)							{ return QC_TOKEN_FAULTS; }
(?i:FETCH)							{ return QC_TOKEN_FETCH; }
(?i:FILE)							{ return QC_TOKEN_FILE; }
(?i:FIRST)							{ return QC_TOKEN_FIRST; }
(?i:FIXED)							{ return QC_TOKEN_FIXED; }
(?i:FLOAT_NUM)						{ return QC_TOKEN_FLOAT_NUM; }
(?i:FLOAT)							{ return QC_TOKEN_FLOAT; }
(?i:FLUSH)							{ return QC_TOKEN_FLUSH; }
(?i:FORCE)							{ return QC_TOKEN_FORCE; }
(?i:FOREIGN)						{ return QC_TOKEN_FOREIGN; }
(?i:FOR)							{ return QC_TOKEN_FOR; }
(?i:FOUND)							{ return QC_TOKEN_FOUND; }
(?i:FRAC_SECOND)					{ return QC_TOKEN_FRAC_SECOND; }
(?i:FROM)							{ return QC_TOKEN_FROM; }
(?i:FULL)							{ return QC_TOKEN_FULL; }
(?i:FULLTEXT)						{ return QC_TOKEN_FULLTEXT; }
(?i:FUNCTION)						{ return QC_TOKEN_FUNCTION; }
(?i:GEOMETRYCOLLECTION)				{ return QC_TOKEN_GEOMETRYCOLLECTION; }
(?i:GEOMETRY)						{ return QC_TOKEN_GEOMETRY; }
(?i:GET_FORMAT)						{ return QC_TOKEN_GET_FORMAT; }
(?i:GLOBAL)							{ return QC_TOKEN_GLOBAL; }
(?i:GRANT)							{ return QC_TOKEN_GRANT; }
(?i:GRANTS)							{ return QC_TOKEN_GRANTS; }
(?i:GROUP[:space]+BY)				{ return QC_TOKEN_GROUP; }
(?i:GROUP)							{ return QC_TOKEN_GROUP; }
(?i:GROUP_CONCAT)					{ return QC_TOKEN_GROUP_CONCAT; }
(?i:HANDLER)						{ return QC_TOKEN_HANDLER; }
(?i:HASH)							{ return QC_TOKEN_HASH; }
(?i:HAVING)							{ return QC_TOKEN_HAVING; }
(?i:HELP)							{ return QC_TOKEN_HELP; }
(?i:HEX_NUM)						{ return QC_TOKEN_HEX_NUM; }
(?i:HIGH_PRIORITY)					{ return QC_TOKEN_HIGH_PRIORITY; }
(?i:HOST)							{ return QC_TOKEN_HOST; }
(?i:HOSTS)							{ return QC_TOKEN_HOSTS; }
(?i:HOUR_MICROSECOND)				{ return QC_TOKEN_HOUR_MICROSECOND; }
(?i:HOUR_MINUTE)					{ return QC_TOKEN_HOUR_MINUTE; }
(?i:HOUR_SECOND)					{ return QC_TOKEN_HOUR_SECOND; }
(?i:HOUR)							{ return QC_TOKEN_HOUR; }
(?i:IDENT)							{ return QC_TOKEN_IDENT; }
(?i:IDENTIFIED)						{ return QC_TOKEN_IDENTIFIED; }
(?i:IDENT_QUOTED)					{ return QC_TOKEN_IDENT_QUOTED; }
(?i:IF)								{ return QC_TOKEN_IF; }
(?i:IGNORE)							{ return QC_TOKEN_IGNORE; }
(?i:IGNORE_SERVER_IDS)				{ return QC_TOKEN_IGNORE_SERVER_IDS; }
(?i:IMPORT)							{ return QC_TOKEN_IMPORT; }
(?i:INDEXES)						{ return QC_TOKEN_INDEXES; }
(?i:INDEX)							{ return QC_TOKEN_INDEX; }
(?i:INFILE)							{ return QC_TOKEN_INFILE; }
(?i:INITIAL_SIZE)					{ return QC_TOKEN_INITIAL_SIZE; }
(?i:INNER)							{ return QC_TOKEN_INNER; }
(?i:INOUT)							{ return QC_TOKEN_INOUT; }
(?i:INSENSITIVE)					{ return QC_TOKEN_INSENSITIVE; }
(?i:INSERT)							{ return QC_TOKEN_INSERT; }
(?i:INSERT_METHOD)					{ return QC_TOKEN_INSERT_METHOD; }
(?i:INSTALL)						{ return QC_TOKEN_INSTALL; }
(?i:INTERVAL)						{ return QC_TOKEN_INTERVAL; }
(?i:INTO)							{ return QC_TOKEN_INTO; }
(?i:INT)							{ return QC_TOKEN_INT; }
(?i:INVOKER)						{ return QC_TOKEN_INVOKER; }
(?i:IN)								{ return QC_TOKEN_IN; }
(?i:IO)								{ return QC_TOKEN_IO; }
(?i:IPC)							{ return QC_TOKEN_IPC; }
(?i:IS)								{ return QC_TOKEN_IS; }
(?i:ISOLATION)						{ return QC_TOKEN_ISOLATION; }
(?i:ISSUER)							{ return QC_TOKEN_ISSUER; }
(?i:ITERATE)						{ return QC_TOKEN_ITERATE; }
(?i:JOIN)							{ return QC_TOKEN_JOIN; }
(?i:KEYS)							{ return QC_TOKEN_KEYS; }
(?i:KEY_BLOCK_SIZE)					{ return QC_TOKEN_KEY_BLOCK_SIZE; }
(?i:KEY)							{ return QC_TOKEN_KEY; }
(?i:KILL)							{ return QC_TOKEN_KILL; }
(?i:LANGUAGE)						{ return QC_TOKEN_LANGUAGE; }
(?i:LAST)							{ return QC_TOKEN_LAST; }
(?i:LEADING)						{ return QC_TOKEN_LEADING; }
(?i:LEAVES)							{ return QC_TOKEN_LEAVES; }
(?i:LEAVE)							{ return QC_TOKEN_LEAVE; }
(?i:LEFT)							{ return QC_TOKEN_LEFT; }
(?i:LESS)							{ return QC_TOKEN_LESS; }
(?i:LEVEL)							{ return QC_TOKEN_LEVEL; }
(?i:LEX_HOSTNAME)					{ return QC_TOKEN_LEX_HOSTNAME; }
(?i:LIKE)							{ return QC_TOKEN_LIKE; }
(?i:LIMIT)							{ return QC_TOKEN_LIMIT; }
(?i:LINEAR)							{ return QC_TOKEN_LINEAR; }
(?i:LINES)							{ return QC_TOKEN_LINES; }
(?i:LINESTRING)						{ return QC_TOKEN_LINESTRING; }
(?i:LIST)							{ return QC_TOKEN_LIST; }
(?i:LOAD)							{ return QC_TOKEN_LOAD; }
(?i:LOCAL)							{ return QC_TOKEN_LOCAL; }
(?i:LOCATOR)						{ return QC_TOKEN_LOCATOR; }
(?i:LOCKS)							{ return QC_TOKEN_LOCKS; }
(?i:LOCK)							{ return QC_TOKEN_LOCK; }
(?i:LOGFILE)						{ return QC_TOKEN_LOGFILE; }
(?i:LOGS)							{ return QC_TOKEN_LOGS; }
(?i:LONGBLOB)						{ return QC_TOKEN_LONGBLOB; }
(?i:LONGTEXT)						{ return QC_TOKEN_LONGTEXT; }
(?i:LONG_NUM)						{ return QC_TOKEN_LONG_NUM; }
(?i:LONG)							{ return QC_TOKEN_LONG; }
(?i:LOOP)							{ return QC_TOKEN_LOOP; }
(?i:LOW_PRIORITY)					{ return QC_TOKEN_LOW_PRIORITY; }
(?i:MASTER_CONNECT_RETRY)			{ return QC_TOKEN_MASTER_CONNECT_RETRY; }
(?i:MASTER_HOST)					{ return QC_TOKEN_MASTER_HOST; }
(?i:MASTER_LOG_FILE)				{ return QC_TOKEN_MASTER_LOG_FILE; }
(?i:MASTER_LOG_POS)					{ return QC_TOKEN_MASTER_LOG_POS; }
(?i:MASTER_PASSWORD)				{ return QC_TOKEN_MASTER_PASSWORD; }
(?i:MASTER_PORT)					{ return QC_TOKEN_MASTER_PORT; }
(?i:MASTER_SERVER_ID)				{ return QC_TOKEN_MASTER_SERVER_ID; }
(?i:MASTER_SSL_CAPATH)				{ return QC_TOKEN_MASTER_SSL_CAPATH; }
(?i:MASTER_SSL_CA)					{ return QC_TOKEN_MASTER_SSL_CA; }
(?i:MASTER_SSL_CERT)				{ return QC_TOKEN_MASTER_SSL_CERT; }
(?i:MASTER_SSL_CIPHER)				{ return QC_TOKEN_MASTER_SSL_CIPHER; }
(?i:MASTER_SSL_KEY)					{ return QC_TOKEN_MASTER_SSL_KEY; }
(?i:MASTER_SSL)						{ return QC_TOKEN_MASTER_SSL; }
(?i:MASTER_SSL_VERIFY_SERVER_CERT)	{ return QC_TOKEN_MASTER_SSL_VERIFY_SERVER_CERT; }
(?i:MASTER)							{ return QC_TOKEN_MASTER; }
(?i:MASTER_USER)					{ return QC_TOKEN_MASTER_USER; }
(?i:MASTER_HEARTBEAT_PERIOD)		{ return QC_TOKEN_MASTER_HEARTBEAT_PERIOD; }
(?i:MATCH)							{ return QC_TOKEN_MATCH; }
(?i:MAX_CONNECTIONS_PER_HOUR)		{ return QC_TOKEN_MAX_CONNECTIONS_PER_HOUR; }
(?i:MAX_QUERIES_PER_HOUR)			{ return QC_TOKEN_MAX_QUERIES_PER_HOUR; }
(?i:MAX_ROWS)						{ return QC_TOKEN_MAX_ROWS; }
(?i:MAX_SIZE)						{ return QC_TOKEN_MAX_SIZE; }
(?i:MAX)							{ return QC_TOKEN_MAX; }
(?i:MAX_UPDATES_PER_HOUR)			{ return QC_TOKEN_MAX_UPDATES_PER_HOUR; }
(?i:MAX_USER_CONNECTIONS)			{ return QC_TOKEN_MAX_USER_CONNECTIONS; }
(?i:MAX_VALUE)						{ return QC_TOKEN_MAX_VALUE; }
(?i:MEDIUMBLOB)						{ return QC_TOKEN_MEDIUMBLOB; }
(?i:MEDIUMINT)						{ return QC_TOKEN_MEDIUMINT; }
(?i:MEDIUMTEXT)						{ return QC_TOKEN_MEDIUMTEXT; }
(?i:MEDIUM)							{ return QC_TOKEN_MEDIUM; }
(?i:MEMORY)							{ return QC_TOKEN_MEMORY; }
(?i:MERGE)							{ return QC_TOKEN_MERGE; }
(?i:MESSAGE_TEXT)					{ return QC_TOKEN_MESSAGE_TEXT; }
(?i:MICROSECOND)					{ return QC_TOKEN_MICROSECOND; }
(?i:MIGRATE)						{ return QC_TOKEN_MIGRATE; }
(?i:MINUTE_MICROSECOND)				{ return QC_TOKEN_MINUTE_MICROSECOND; }
(?i:MINUTE_SECOND)					{ return QC_TOKEN_MINUTE_SECOND; }
(?i:MINUTE)							{ return QC_TOKEN_MINUTE; }
(?i:MIN_ROWS)						{ return QC_TOKEN_MIN_ROWS; }
(?i:MIN)							{ return QC_TOKEN_MIN; }
(?i:MODE)							{ return QC_TOKEN_MODE; }
(?i:MODIFIES)						{ return QC_TOKEN_MODIFIES; }
(?i:MODIFY)							{ return QC_TOKEN_MODIFY; }
(?i:MOD)							{ return QC_TOKEN_MOD; }
(?i:MONTH)							{ return QC_TOKEN_MONTH; }
(?i:MULTILINESTRING)				{ return QC_TOKEN_MULTILINESTRING; }
(?i:MULTIPOINT)						{ return QC_TOKEN_MULTIPOINT; }
(?i:MULTIPOLYGON)					{ return QC_TOKEN_MULTIPOLYGON; }
(?i:MUTEX)							{ return QC_TOKEN_MUTEX; }
(?i:MYSQL_ERRNO)					{ return QC_TOKEN_MYSQL_ERRNO; }
(?i:NAMES)							{ return QC_TOKEN_NAMES; }
(?i:NAME)							{ return QC_TOKEN_NAME; }
(?i:NATIONAL)						{ return QC_TOKEN_NATIONAL; }
(?i:NATURAL)						{ return QC_TOKEN_NATURAL; }
(?i:NCHAR_STRING)					{ return QC_TOKEN_NCHAR_STRING; }
(?i:NCHAR)							{ return QC_TOKEN_NCHAR; }
(?i:NDBCLUSTER)						{ return QC_TOKEN_NDBCLUSTER; }
(?i:NEG)							{ return QC_TOKEN_NEG; }
(?i:NEW)							{ return QC_TOKEN_NEW; }
(?i:NEXT)							{ return QC_TOKEN_NEXT; }
(?i:NODEGROUP)						{ return QC_TOKEN_NODEGROUP; }
(?i:NONE)							{ return QC_TOKEN_NONE; }
(?i:NOT2)							{ return QC_TOKEN_NOT2; }
(?i:NOT)							{ return QC_TOKEN_NOT; }
(?i:NOW)							{ return QC_TOKEN_NOW; }
(?i:NO)								{ return QC_TOKEN_NO; }
(?i:NO_WAIT)						{ return QC_TOKEN_NO_WAIT; }
(?i:NO_WRITE_TO_BINLOG)				{ return QC_TOKEN_NO_WRITE_TO_BINLOG; }
(?i:NULL)							{ return QC_TOKEN_NULL; }
(?i:NUM)							{ return QC_TOKEN_NUM; }
(?i:NUMERIC)						{ return QC_TOKEN_NUMERIC; }
(?i:NVARCHAR)						{ return QC_TOKEN_NVARCHAR; }
(?i:OFFSET)							{ return QC_TOKEN_OFFSET; }
(?i:OLD_PASSWORD)					{ return QC_TOKEN_OLD_PASSWORD; }
(?i:ON)								{ return QC_TOKEN_ON; }
(?i:ONE_SHOT)						{ return QC_TOKEN_ONE_SHOT; }
(?i:ONE)							{ return QC_TOKEN_ONE; }
(?i:OPEN)							{ return QC_TOKEN_OPEN; }
(?i:OPTIMIZE)						{ return QC_TOKEN_OPTIMIZE; }
(?i:OPTIONS)						{ return QC_TOKEN_OPTIONS; }
(?i:OPTION)							{ return QC_TOKEN_OPTION; }
(?i:OPTIONALLY)						{ return QC_TOKEN_OPTIONALLY; }
(?i:ORDER)							{ return QC_TOKEN_ORDER; }
(?i:OR)								{ return QC_TOKEN_OR; }
(?i:OUTER)							{ return QC_TOKEN_OUTER; }
(?i:OUTFILE)						{ return QC_TOKEN_OUTFILE; }
(?i:OUT)							{ return QC_TOKEN_OUT; }
(?i:OWNER)							{ return QC_TOKEN_OWNER; }
(?i:PACK_KEYS)						{ return QC_TOKEN_PACK_KEYS; }
(?i:PAGE)							{ return QC_TOKEN_PAGE; }
(?i:PARAM_MARKER)					{ return QC_TOKEN_PARAM_MARKER; }
(?i:PARSER)							{ return QC_TOKEN_PARSER; }
(?i:PARTIAL)						{ return QC_TOKEN_PARTIAL; }
(?i:PARTITIONING)					{ return QC_TOKEN_PARTITIONING; }
(?i:PARTITIONS)						{ return QC_TOKEN_PARTITIONS; }
(?i:PARTITION)						{ return QC_TOKEN_PARTITION; }
(?i:PASSWORD)						{ return QC_TOKEN_PASSWORD; }
(?i:PHASE)							{ return QC_TOKEN_PHASE; }
(?i:PLUGINS)						{ return QC_TOKEN_PLUGINS; }
(?i:PLUGIN)							{ return QC_TOKEN_PLUGIN; }
(?i:POINT)							{ return QC_TOKEN_POINT; }
(?i:POLYGON)						{ return QC_TOKEN_POLYGON; }
(?i:PORT)							{ return QC_TOKEN_PORT; }
(?i:POSITION)						{ return QC_TOKEN_POSITION; }
(?i:PRECISION)						{ return QC_TOKEN_PRECISION; }
(?i:PREPARE)						{ return QC_TOKEN_PREPARE; }
(?i:PRESERVE)						{ return QC_TOKEN_PRESERVE; }
(?i:PREV)							{ return QC_TOKEN_PREV; }
(?i:PRIMARY)						{ return QC_TOKEN_PRIMARY; }
(?i:PRIVILEGES)						{ return QC_TOKEN_PRIVILEGES; }
(?i:PROCEDURE)						{ return QC_TOKEN_PROCEDURE; }
(?i:PROCESS)						{ return QC_TOKEN_PROCESS; }
(?i:PROCESSLIST)					{ return QC_TOKEN_PROCESSLIST; }
(?i:PROFILE)						{ return QC_TOKEN_PROFILE; }
(?i:PROFILES)						{ return QC_TOKEN_PROFILES; }
(?i:PURGE)							{ return QC_TOKEN_PURGE; }
(?i:QUARTER)						{ return QC_TOKEN_QUARTER; }
(?i:QUERY)							{ return QC_TOKEN_QUERY; }
(?i:QUICK)							{ return QC_TOKEN_QUICK; }
(?i:RANGE)							{ return QC_TOKEN_RANGE; }
(?i:READS)							{ return QC_TOKEN_READS; }
(?i:READ_ONLY)						{ return QC_TOKEN_READ_ONLY; }
(?i:READ)							{ return QC_TOKEN_READ; }
(?i:READ_WRITE)						{ return QC_TOKEN_READ_WRITE; }
(?i:REAL)							{ return QC_TOKEN_REAL; }
(?i:REBUILD)						{ return QC_TOKEN_REBUILD; }
(?i:RECOVER)						{ return QC_TOKEN_RECOVER; }
(?i:REDOFILE)						{ return QC_TOKEN_REDOFILE; }
(?i:REDO_BUFFER_SIZE)				{ return QC_TOKEN_REDO_BUFFER_SIZE; }
(?i:REDUNDANT)						{ return QC_TOKEN_REDUNDANT; }
(?i:REFERENCES)						{ return QC_TOKEN_REFERENCES; }
(?i:REGEXP)							{ return QC_TOKEN_REGEXP; }
(?i:RELAYLOG)						{ return QC_TOKEN_RELAYLOG; }
(?i:RELAY_LOG_FILE)					{ return QC_TOKEN_RELAY_LOG_FILE; }
(?i:RELAY_LOG_POS)					{ return QC_TOKEN_RELAY_LOG_POS; }
(?i:RELAY_THREAD)					{ return QC_TOKEN_RELAY_THREAD; }
(?i:RELEASE)						{ return QC_TOKEN_RELEASE; }
(?i:RELOAD)							{ return QC_TOKEN_RELOAD; }
(?i:REMOVE)							{ return QC_TOKEN_REMOVE; }
(?i:RENAME)							{ return QC_TOKEN_RENAME; }
(?i:REORGANIZE)						{ return QC_TOKEN_REORGANIZE; }
(?i:REPAIR)							{ return QC_TOKEN_REPAIR; }
(?i:REPEATABLE)						{ return QC_TOKEN_REPEATABLE; }
(?i:REPEAT)							{ return QC_TOKEN_REPEAT; }
(?i:REPLACE)						{ return QC_TOKEN_REPLACE; }
(?i:REPLICATION)					{ return QC_TOKEN_REPLICATION; }
(?i:REQUIRE)						{ return QC_TOKEN_REQUIRE; }
(?i:RESET)							{ return QC_TOKEN_RESET; }
(?i:RESIGNAL)						{ return QC_TOKEN_RESIGNAL; }
(?i:RESOURCES)						{ return QC_TOKEN_RESOURCES; }
(?i:RESTORE)						{ return QC_TOKEN_RESTORE; }
(?i:RESTRICT)						{ return QC_TOKEN_RESTRICT; }
(?i:RESUME)							{ return QC_TOKEN_RESUME; }
(?i:RETURNS)						{ return QC_TOKEN_RETURNS; }
(?i:RETURN)							{ return QC_TOKEN_RETURN; }
(?i:REVOKE)							{ return QC_TOKEN_REVOKE; }
(?i:RIGHT)							{ return QC_TOKEN_RIGHT; }
(?i:ROLLBACK)						{ return QC_TOKEN_ROLLBACK; }
(?i:ROLLUP)							{ return QC_TOKEN_ROLLUP; }
(?i:ROUTINE)						{ return QC_TOKEN_ROUTINE; }
(?i:ROWS)							{ return QC_TOKEN_ROWS; }
(?i:ROW_FORMAT)						{ return QC_TOKEN_ROW_FORMAT; }
(?i:ROW)							{ return QC_TOKEN_ROW; }
(?i:RTREE)							{ return QC_TOKEN_RTREE; }
(?i:SAVEPOINT)						{ return QC_TOKEN_SAVEPOINT; }
(?i:SCHEDULE)						{ return QC_TOKEN_SCHEDULE; }
(?i:SCHEMA_NAME)					{ return QC_TOKEN_SCHEMA_NAME; }
(?i:SECOND_MICROSECOND)				{ return QC_TOKEN_SECOND_MICROSECOND; }
(?i:SECOND)							{ return QC_TOKEN_SECOND; }
(?i:SECURITY)						{ return QC_TOKEN_SECURITY; }
(?i:SELECT)							{ return QC_TOKEN_SELECT; }
(?i:SENSITIVE)						{ return QC_TOKEN_SENSITIVE; }
(?i:SEPARATOR)						{ return QC_TOKEN_SEPARATOR; }
(?i:SERIALIZABLE)					{ return QC_TOKEN_SERIALIZABLE; }
(?i:SERIAL)							{ return QC_TOKEN_SERIAL; }
(?i:SESSION)						{ return QC_TOKEN_SESSION; }
(?i:SERVER)							{ return QC_TOKEN_SERVER; }
(?i:SERVER_OPTIONS)					{ return QC_TOKEN_SERVER_OPTIONS; }
(?i:SET)							{ return QC_TOKEN_SET; }
(?i:SHARE)							{ return QC_TOKEN_SHARE; }
(?i:SHIFT_LEFT)						{ return QC_TOKEN_SHIFT_LEFT; }
(?i:SHIFT_RIGHT)					{ return QC_TOKEN_SHIFT_RIGHT; }
(?i:SHOW)							{ return QC_TOKEN_SHOW; }
(?i:SHUTDOWN)						{ return QC_TOKEN_SHUTDOWN; }
(?i:SIGNAL)							{ return QC_TOKEN_SIGNAL; }
(?i:SIGNED)							{ return QC_TOKEN_SIGNED; }
(?i:SIMPLE)							{ return QC_TOKEN_SIMPLE; }
(?i:SLAVE)							{ return QC_TOKEN_SLAVE; }
(?i:SMALLINT)						{ return QC_TOKEN_SMALLINT; }
(?i:SNAPSHOT)						{ return QC_TOKEN_SNAPSHOT; }
(?i:SOCKET)							{ return QC_TOKEN_SOCKET; }
(?i:SONAME)							{ return QC_TOKEN_SONAME; }
(?i:SOUNDS)							{ return QC_TOKEN_SOUNDS; }
(?i:SOURCE)							{ return QC_TOKEN_SOURCE; }
(?i:SPATIAL)						{ return QC_TOKEN_SPATIAL; }
(?i:SPECIFIC)						{ return QC_TOKEN_SPECIFIC; }
(?i:SQLEXCEPTION)					{ return QC_TOKEN_SQLEXCEPTION; }
(?i:SQLSTATE)						{ return QC_TOKEN_SQLSTATE; }
(?i:SQLWARNING)						{ return QC_TOKEN_SQLWARNING; }
(?i:SQL_BIG_RESULT)					{ return QC_TOKEN_SQL_BIG_RESULT; }
(?i:SQL_BUFFER_RESULT)				{ return QC_TOKEN_SQL_BUFFER_RESULT; }
(?i:SQL_CACHE)						{ return QC_TOKEN_SQL_CACHE; }
(?i:SQL_CALC_FOUND_ROWS)			{ return QC_TOKEN_SQL_CALC_FOUND_ROWS; }
(?i:SQL_NO_CACHE)					{ return QC_TOKEN_SQL_NO_CACHE; }
(?i:SQL_SMALL_RESULT)				{ return QC_TOKEN_SQL_SMALL_RESULT; }
(?i:SQL)							{ return QC_TOKEN_SQL; }
(?i:SQL_THREAD)						{ return QC_TOKEN_SQL_THREAD; }
(?i:SSL)							{ return QC_TOKEN_SSL; }
(?i:STARTING)						{ return QC_TOKEN_STARTING; }
(?i:STARTS)							{ return QC_TOKEN_STARTS; }
(?i:START)							{ return QC_TOKEN_START; }
(?i:STATUS)							{ return QC_TOKEN_STATUS; }
(?i:STDDEV_SAMP)					{ return QC_TOKEN_STDDEV_SAMP; }
(?i:STD)							{ return QC_TOKEN_STD; }
(?i:STOP)							{ return QC_TOKEN_STOP; }
(?i:STORAGE)						{ return QC_TOKEN_STORAGE; }
(?i:STRAIGHT[:space]+JOIN)			{ return QC_TOKEN_STRAIGHT_JOIN; }
(?i:STRING)							{ return QC_TOKEN_STRING; }
(?i:SUBCLASS_ORIGIN)				{ return QC_TOKEN_SUBCLASS_ORIGIN; }
(?i:SUBDATE)						{ return QC_TOKEN_SUBDATE; }
(?i:SUBJECT)						{ return QC_TOKEN_SUBJECT; }
(?i:SUBPARTITIONS)					{ return QC_TOKEN_SUBPARTITIONS; }
(?i:SUBPARTITION)					{ return QC_TOKEN_SUBPARTITION; }
(?i:SUBSTRING)						{ return QC_TOKEN_SUBSTRING; }
(?i:SUM)							{ return QC_TOKEN_SUM; }
(?i:SUPER)							{ return QC_TOKEN_SUPER; }
(?i:SUSPEND)						{ return QC_TOKEN_SUSPEND; }
(?i:SWAPS)							{ return QC_TOKEN_SWAPS; }
(?i:SWITCHES)						{ return QC_TOKEN_SWITCHES; }
(?i:SYSDATE)						{ return QC_TOKEN_SYSDATE; }
(?i:TABLES)							{ return QC_TOKEN_TABLES; }
(?i:TABLESPACE)						{ return QC_TOKEN_TABLESPACE; }
(?i:TABLE_REF_PRIORITY)				{ return QC_TOKEN_TABLE_REF_PRIORITY; }
(?i:TABLE)							{ return QC_TOKEN_TABLE; }
(?i:TABLE_CHECKSUM)					{ return QC_TOKEN_TABLE_CHECKSUM; }
(?i:TABLE_NAME)						{ return QC_TOKEN_TABLE_NAME; }
(?i:TEMPORARY)						{ return QC_TOKEN_TEMPORARY; }
(?i:TEMPTABLE)						{ return QC_TOKEN_TEMPTABLE; }
(?i:TERMINATED)						{ return QC_TOKEN_TERMINATED; }
(?i:TEXT_STRING)					{ return QC_TOKEN_TEXT_STRING; }
(?i:TEXT)							{ return QC_TOKEN_TEXT; }
(?i:THAN)							{ return QC_TOKEN_THAN; }
(?i:THEN)							{ return QC_TOKEN_THEN; }
(?i:TIMESTAMP)						{ return QC_TOKEN_TIMESTAMP; }
(?i:TIMESTAMP_ADD)					{ return QC_TOKEN_TIMESTAMP_ADD; }
(?i:TIMESTAMP_DIFF)					{ return QC_TOKEN_TIMESTAMP_DIFF; }
(?i:TIME)							{ return QC_TOKEN_TIME; }
(?i:TINYBLOB)						{ return QC_TOKEN_TINYBLOB; }
(?i:TINYINT)						{ return QC_TOKEN_TINYINT; }
(?i:TINYTEXT)						{ return QC_TOKEN_TINYTEXT; }
(?i:TO)								{ return QC_TOKEN_TO; }
(?i:TRAILING)						{ return QC_TOKEN_TRAILING; }
(?i:TRANSACTION)					{ return QC_TOKEN_TRANSACTION; }
(?i:TRIGGERS)						{ return QC_TOKEN_TRIGGERS; }
(?i:TRIGGER)						{ return QC_TOKEN_TRIGGER; }
(?i:TRIM)							{ return QC_TOKEN_TRIM; }
(?i:TRUE)							{ return QC_TOKEN_TRUE; }
(?i:TRUNCATE)						{ return QC_TOKEN_TRUNCATE; }
(?i:TYPES)							{ return QC_TOKEN_TYPES; }
(?i:TYPE)							{ return QC_TOKEN_TYPE; }
(?i:UDF_RETURNS)					{ return QC_TOKEN_UDF_RETURNS; }
(?i:ULONGLONG_NUM)					{ return QC_TOKEN_ULONGLONG_NUM; }
(?i:UNCOMMITTED)					{ return QC_TOKEN_UNCOMMITTED; }
(?i:UNDEFINED)						{ return QC_TOKEN_UNDEFINED; }
(?i:UNDERSCORE_CHARSET)				{ return QC_TOKEN_UNDERSCORE_CHARSET; }
(?i:UNDOFILE)						{ return QC_TOKEN_UNDOFILE; }
(?i:UNDO_BUFFER_SIZE)				{ return QC_TOKEN_UNDO_BUFFER_SIZE; }
(?i:UNDO)							{ return QC_TOKEN_UNDO; }
(?i:UNICODE)						{ return QC_TOKEN_UNICODE; }
(?i:UNINSTALL)						{ return QC_TOKEN_UNINSTALL; }
(?i:UNION)							{ return QC_TOKEN_UNION; }
(?i:UNIQUE)							{ return QC_TOKEN_UNIQUE; }
(?i:UNKNOWN)						{ return QC_TOKEN_UNKNOWN; }
(?i:UNLOCK)							{ return QC_TOKEN_UNLOCK; }
(?i:UNSIGNED)						{ return QC_TOKEN_UNSIGNED; }
(?i:UNTIL)							{ return QC_TOKEN_UNTIL; }
(?i:UPDATE)							{ return QC_TOKEN_UPDATE; }
(?i:UPGRADE)						{ return QC_TOKEN_UPGRADE; }
(?i:USAGE)							{ return QC_TOKEN_USAGE; }
(?i:USER)							{ return QC_TOKEN_USER; }
(?i:USE_FRM)						{ return QC_TOKEN_USE_FRM; }
(?i:USE)							{ return QC_TOKEN_USE; }
(?i:USING)							{ return QC_TOKEN_USING; }
(?i:UTC_DATE)						{ return QC_TOKEN_UTC_DATE; }
(?i:UTC_TIMESTAMP)					{ return QC_TOKEN_UTC_TIMESTAMP; }
(?i:UTC_TIME)						{ return QC_TOKEN_UTC_TIME; }
(?i:VALUES)							{ return QC_TOKEN_VALUES; }
(?i:VALUE)							{ return QC_TOKEN_VALUE; }
(?i:VARBINARY)						{ return QC_TOKEN_VARBINARY; }
(?i:VARCHAR)						{ return QC_TOKEN_VARCHAR; }
(?i:VARIABLES)						{ return QC_TOKEN_VARIABLES; }
(?i:VARIANCE)						{ return QC_TOKEN_VARIANCE; }
(?i:VARYING)						{ return QC_TOKEN_VARYING; }
(?i:VAR_SAMP)						{ return QC_TOKEN_VAR_SAMP; }
(?i:VIEW)							{ return QC_TOKEN_VIEW; }
(?i:WAIT)							{ return QC_TOKEN_WAIT; }
(?i:WARNINGS)						{ return QC_TOKEN_WARNINGS; }
(?i:WEEK)							{ return QC_TOKEN_WEEK; }
(?i:WHEN)							{ return QC_TOKEN_WHEN; }
(?i:WHERE)							{ return QC_TOKEN_WHERE; }
(?i:WHILE)							{ return QC_TOKEN_WHILE; }
(?i:WITH)							{ return QC_TOKEN_WITH; }
(?i:WITH[:spacee]+CUBE)				{ return QC_TOKEN_WITH_CUBE; }
(?i:WITH[:spacee]+ROLLUP)			{ return QC_TOKEN_WITH_ROLLUP; }
(?i:WORK)							{ return QC_TOKEN_WORK; }
(?i:WRAPPER)						{ return QC_TOKEN_WRAPPER; }
(?i:WRITE)							{ return QC_TOKEN_WRITE; }
(?i:X509)							{ return QC_TOKEN_X509; }
(?i:XA)								{ return QC_TOKEN_XA; }
(?i:XML)							{ return QC_TOKEN_XML; }
(?i:XOR)							{ return QC_TOKEN_XOR; }
(?i:YEAR_MONTH)						{ return QC_TOKEN_YEAR_MONTH; }
(?i:YEAR)							{ return QC_TOKEN_YEAR; }
(?i:ZEROFILL)						{ return QC_TOKEN_ZEROFILL; }
(?i:CLIENT_FLAG)					{ return QC_TOKEN_CLIENT_FLAG; }

	/* Integers and Floats */
-?[0-9]+						{ ZVAL_LONG(token_value, atoi(yytext)); return QC_TOKEN_INTNUM; }

-?[0-9]+"."[0-9]* |
-?"."[0-9]+ |
-?[0-9]+E[-+]?[0-9]+ |
-?[0-9]+"."[0-9]*E[-+]?[0-9]+ |
-?"."[0-9]+E[-+]?[0-9]+			{ ZVAL_DOUBLE(token_value, atof(yytext)); return QC_TOKEN_FLOATNUM; }

	/* Normal strings */
'(\\.|''|[^'\n])*' |
\"(\\.|\"\"|[^"\n])*\"			{ ZVAL_STRINGL(token_value, yytext, yyleng, 1); return QC_TOKEN_STRING; }
'(\\.|[^'\n])*$					{ yyerror("Unterminated string %s", yytext); }
\"(\\.|[^"\n])*$				{ yyerror("Unterminated string %s", yytext); }

	/* Hex and Bit strings */
X'[0-9A-F]+' |
0X[0-9A-F]+						{ ZVAL_STRINGL(token_value, yytext, yyleng, 1); return QC_TOKEN_STRING; }
0B[01]+ |
B'[01]+'						{ ZVAL_STRINGL(token_value, yytext, yyleng, 1); return QC_TOKEN_STRING; }

	/* Operators */
\+								{ return QC_TOKEN_PLUS; }
\-								{ return QC_TOKEN_MINUS; }
\,								{ return QC_TOKEN_COMMA; }
\;								{ return QC_TOKEN_SEMICOLON; }
\(								{ return QC_TOKEN_BRACKET_OPEN; }
\)								{ return QC_TOKEN_BRACKET_CLOSE; }
\*								{ return QC_TOKEN_STAR; }
\.								{ return QC_TOKEN_DOT; }
!								{ return QC_TOKEN_NOT; }
\^								{ return QC_TOKEN_XOR; }
\%								{ return QC_TOKEN_MOD; }
\/								{ return QC_TOKEN_DIV; }
\~								{ return QC_TOKEN_TILDE; }
"@@"							{ return QC_TOKEN_GLOBAL_VAR; }
\@								{ return QC_TOKEN_SESSION_VAR; }
"&&"							{ return QC_TOKEN_AND; }
\&								{ return QC_TOKEN_BIT_AND; }
"||"							{ return QC_TOKEN_OR; }
\|								{ return QC_TOKEN_BIT_OR; }
"="								{ return QC_TOKEN_EQ; }
"<=>"							{ return QC_TOKEN_NE_TRIPLE; }
">="							{ return QC_TOKEN_GE; }
">"								{ return QC_TOKEN_GT; }
"<="							{ return QC_TOKEN_LE; }
"<"								{ return QC_TOKEN_LT; }
"!=" | "<>"						{ return QC_TOKEN_NE; }
"<<"							{ return QC_TOKEN_SHIFT_LEFT; }
">>"							{ return QC_TOKEN_SHIFT_RIGHT; }
":="							{ return QC_TOKEN_ASSIGN_TO_VAR; }



	/* normal identifier */
([A-Za-z$_]|[\xC2-\xDF][\x80-\xBF]|\xE0[\xA0-\xBF][\x80-\xBF]|[\xE1-\xEF][\x80-\xBF][\x80-\xBF])([0-9]|[A-Za-z$_]|[\xC2-\xDF][\x80-\xBF]|\xE0[\xA0-\xBF][\x80-\xBF]|[\xE1-\xEF][\x80-\xBF][\x80-\xBF])* { ZVAL_STRINGL(token_value, yytext, yyleng, 1); return QC_TOKEN_IDENTIFIER; }

	/* quoted identifier */
`[^`/\\.\n]+`					{ ZVAL_STRINGL(token_value, yytext + 1, yyleng - 2, 1); return QC_TOKEN_IDENTIFIER; }

	/* Comments */
#.* ;							{ ZVAL_STRINGL(token_value, yytext + 1, yyleng - 1, 1); return QC_TOKEN_COMMENT; }
"--"[ \t].*						{ ZVAL_STRINGL(token_value, yytext + 2, yyleng - 2, 1); return QC_TOKEN_COMMENT; }
"/*" 							{ old_yystate = YY_START; BEGIN COMMENT_MODE; ZVAL_NULL(token_value); }
<COMMENT_MODE>"*/" 				{
									BEGIN old_yystate;
									return QC_TOKEN_COMMENT;
								}
<COMMENT_MODE>.|\n 				{	
									char * tmp_copy;
									long tmp_len;
#if 0
									printf("yytext=[%s]\n", yytext);
#endif
									convert_to_string(token_value);
									tmp_len = Z_STRLEN_P(token_value);
									tmp_copy = emalloc(Z_STRLEN_P(token_value) + 2);
									memcpy(tmp_copy, Z_STRVAL_P(token_value), Z_STRLEN_P(token_value));
									tmp_copy[Z_STRLEN_P(token_value)] = yytext[0];
									tmp_copy[Z_STRLEN_P(token_value) + 1] = '\0';
									zval_dtor(token_value);
									ZVAL_STRINGL(token_value, tmp_copy, tmp_len + 1, 0);
								}

	/* the rest */
[ \t\n] /* whitespace */
. 								{ yyerror("report to the developer '%c'\n", *yytext); }
%%


/* {{{ mysqlnd_tok_create_scanner */
PHPAPI struct st_mysqlnd_tok_scanner *
mysqlnd_tok_create_scanner(const char * const query, const size_t query_len TSRMLS_DC)
{
	struct st_mysqlnd_tok_scanner * ret = mnd_ecalloc(1, sizeof(struct st_mysqlnd_tok_scanner));

	DBG_ENTER("mysqlnd_tok_create_scanner");

	ret->scanner = mnd_ecalloc(1, sizeof(yyscan_t));
	MAKE_STD_ZVAL(ret->token_value);
	ZVAL_NULL(ret->token_value);

	if (yylex_init_extra(ret->token_value, (yyscan_t *) ret->scanner) || !query) {
		DBG_ERR_FMT("yylex_init_extra failed");
		mysqlnd_tok_free_scanner(ret TSRMLS_CC);
		ret = NULL;
	}

	/* scan_string/scan_bytes expect `yyscan_t`, not `yyscan_t*` */
	yy_scan_bytes(query, query_len, *((yyscan_t *)ret->scanner));

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_tok_free_scanner */
PHPAPI void
mysqlnd_tok_free_scanner(struct st_mysqlnd_tok_scanner * scanner TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_tok_free_scanner");
	if (scanner) {
		yylex_destroy(*(yyscan_t *) scanner->scanner);
		mnd_efree(scanner->scanner);
	
		zval_ptr_dtor(&scanner->token_value);
		scanner->token_value = NULL;

		mnd_efree(scanner);
	}

	DBG_VOID_RETURN;
}
/* }}} */


PHPAPI struct st_qc_token_and_value
mysqlnd_tok_get_token(struct st_mysqlnd_tok_scanner * scanner TSRMLS_DC)
{
	struct st_qc_token_and_value ret = {0};
	DBG_ENTER("mysqlnd_tok_get_token");
	
	/* yylex expects `yyscan_t`, not `yyscan_t*` */
	if ((ret.token = yylex(NULL, *(yyscan_t *)scanner->scanner))) {
		DBG_INF_FMT("token=%d", ret.token);
		switch (Z_TYPE_P(scanner->token_value)) {
			case IS_STRING:
				DBG_INF_FMT("strval=%s", Z_STRVAL_P(scanner->token_value));
				break;
			case IS_LONG:
				DBG_INF_FMT("lval=%ld", Z_LVAL_P(scanner->token_value));
				break;
			case IS_DOUBLE:
				DBG_INF_FMT("dval=%f", Z_DVAL_P(scanner->token_value));
				break;
		}
		/* transfer ownership */
		ret.value = *scanner->token_value;
		ZVAL_NULL(scanner->token_value);
		Z_LVAL_P(scanner->token_value) = 0;
	}

	DBG_RETURN(ret);
}

