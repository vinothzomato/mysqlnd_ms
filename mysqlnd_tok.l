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

%x COMMENT_MODE
%s BETWEEN_MODE

%%
%{
	zval * token_value = yyextra;
%}

ACCESSIBLE						{ return QC_TOKEN_ACCESSIBLE; }
ACTION							{ return QC_TOKEN_ACTION; }
ADD								{ return QC_TOKEN_ADD; }
ADDDATE							{ return QC_TOKEN_ADDDATE; }
AFTER							{ return QC_TOKEN_AFTER; }
AGAINST							{ return QC_TOKEN_AGAINST; }
AGGREGATE						{ return QC_TOKEN_AGGREGATE; }
ALGORITHM						{ return QC_TOKEN_ALGORITHM; }
ALL								{ return QC_TOKEN_ALL; }
ALTER							{ return QC_TOKEN_ALTER; }
ANALYZE							{ return QC_TOKEN_ANALYZE; }
AND_AND							{ return QC_TOKEN_AND_AND; }
<BETWEEN_MODE>AND				{ BEGIN INITIAL; return QC_TOKEN_BETWEEN_AND; }
AND								{ return QC_TOKEN_AND; }
ANY								{ return QC_TOKEN_ANY; }
AS								{ return QC_TOKEN_AS; }
ASC								{ return QC_TOKEN_ASC; }
ASCII							{ return QC_TOKEN_ASCII; }
ASENSITIVE						{ return QC_TOKEN_ASENSITIVE; }
AT								{ return QC_TOKEN_AT; }
AUTHORS							{ return QC_TOKEN_AUTHORS; }
AUTOEXTEND_SIZE					{ return QC_TOKEN_AUTOEXTEND_SIZE; }
AUTO_INC						{ return QC_TOKEN_AUTO_INC; }
AVG_ROW_LENGTH					{ return QC_TOKEN_AVG_ROW_LENGTH; }
AVG								{ return QC_TOKEN_AVG; }
BACKUP							{ return QC_TOKEN_BACKUP; }
BEFORE							{ return QC_TOKEN_BEFORE; }
BEGIN							{ return QC_TOKEN_BEGIN; }
BETWEEN							{ BEGIN BETWEEN_MODE; return QC_TOKEN_BETWEEN; }
BIGINT							{ return QC_TOKEN_BIGINT; }
BINARY							{ return QC_TOKEN_BINARY; }
BINLOG							{ return QC_TOKEN_BINLOG; }
BIN_NUM							{ return QC_TOKEN_BIN_NUM; }
BIT_AND							{ return QC_TOKEN_BIT_AND; }
BIT_OR							{ return QC_TOKEN_BIT_OR; }
BIT								{ return QC_TOKEN_BIT; }
BIT_XOR							{ return QC_TOKEN_BIT_XOR; }
BLOB							{ return QC_TOKEN_BLOB; }
BLOCK							{ return QC_TOKEN_BLOCK; }
BOOLEAN							{ return QC_TOKEN_BOOLEAN; }
BOOL							{ return QC_TOKEN_BOOL; }
BOTH							{ return QC_TOKEN_BOTH; }
BTREE							{ return QC_TOKEN_BTREE; }
BY								{ return QC_TOKEN_BY; }
BYTE							{ return QC_TOKEN_BYTE; }
CACHE							{ return QC_TOKEN_CACHE; }
CALL							{ return QC_TOKEN_CALL; }
CASCADE							{ return QC_TOKEN_CASCADE; }
CASCADED						{ return QC_TOKEN_CASCADED; }
CASE							{ return QC_TOKEN_CASE; }
CAST							{ return QC_TOKEN_CAST; }
CATALOG_NAME					{ return QC_TOKEN_CATALOG_NAME; }
CHAIN							{ return QC_TOKEN_CHAIN; }
CHANGE							{ return QC_TOKEN_CHANGE; }
CHANGED							{ return QC_TOKEN_CHANGED; }
CHARSET							{ return QC_TOKEN_CHARSET; }
CHAR							{ return QC_TOKEN_CHAR; }
CHECKSUM						{ return QC_TOKEN_CHECKSUM; }
CHECK							{ return QC_TOKEN_CHECK; }
CIPHER							{ return QC_TOKEN_CIPHER; }
CLASS_ORIGIN					{ return QC_TOKEN_CLASS_ORIGIN; }
CLIENT							{ return QC_TOKEN_CLIENT; }
CLOSE							{ return QC_TOKEN_CLOSE; }
COALESCE						{ return QC_TOKEN_COALESCE; }
CODE							{ return QC_TOKEN_CODE; }
COLLATE							{ return QC_TOKEN_COLLATE; }
COLLATION						{ return QC_TOKEN_COLLATION; }
COLUMNS							{ return QC_TOKEN_COLUMNS; }
COLUMN							{ return QC_TOKEN_COLUMN; }
COLUMN_NAME						{ return QC_TOKEN_COLUMN_NAME; }
COMMENT							{ return QC_TOKEN_COMMENT; }
COMMITTED						{ return QC_TOKEN_COMMITTED; }
COMMIT							{ return QC_TOKEN_COMMIT; }
COMPACT							{ return QC_TOKEN_COMPACT; }
COMPLETION						{ return QC_TOKEN_COMPLETION; }
COMPRESSED						{ return QC_TOKEN_COMPRESSED; }
CONCURRENT						{ return QC_TOKEN_CONCURRENT; }
CONDITION						{ return QC_TOKEN_CONDITION; }
CONNECTION						{ return QC_TOKEN_CONNECTION; }
CONSISTENT						{ return QC_TOKEN_CONSISTENT; }
CONSTRAINT						{ return QC_TOKEN_CONSTRAINT; }
CONSTRAINT_CATALOG				{ return QC_TOKEN_CONSTRAINT_CATALOG; }
CONSTRAINT_NAME					{ return QC_TOKEN_CONSTRAINT_NAME; }
CONSTRAINT_SCHEMA				{ return QC_TOKEN_CONSTRAINT_SCHEMA; }
CONTAINS						{ return QC_TOKEN_CONTAINS; }
CONTEXT							{ return QC_TOKEN_CONTEXT; }
CONTINUE						{ return QC_TOKEN_CONTINUE; }
CONTRIBUTORS					{ return QC_TOKEN_CONTRIBUTORS; }
CONVERT							{ return QC_TOKEN_CONVERT; }
COUNT							{ return QC_TOKEN_COUNT; }
CPU								{ return QC_TOKEN_CPU; }
CREATE							{ return QC_TOKEN_CREATE; }
CROSS							{ return QC_TOKEN_CROSS; }
CUBE							{ return QC_TOKEN_CUBE; }
CURDATE							{ return QC_TOKEN_CURDATE; }
CURRENT_USER					{ return QC_TOKEN_CURRENT_USER; }
CURSOR							{ return QC_TOKEN_CURSOR; }
CURSOR_NAME						{ return QC_TOKEN_CURSOR_NAME; }
CURTIME							{ return QC_TOKEN_CURTIME; }
DATABASE						{ return QC_TOKEN_DATABASE; }
DATABASES						{ return QC_TOKEN_DATABASES; }
DATAFILE						{ return QC_TOKEN_DATAFILE; }
DATA							{ return QC_TOKEN_DATA; }
DATETIME						{ return QC_TOKEN_DATETIME; }
DATE_ADD_INTERVAL				{ return QC_TOKEN_DATE_ADD_INTERVAL; }
DATE_SUB_INTERVAL				{ return QC_TOKEN_DATE_SUB_INTERVAL; }
DATE							{ return QC_TOKEN_DATE; }
DAY_HOUR						{ return QC_TOKEN_DAY_HOUR; }
DAY_MICROSECOND					{ return QC_TOKEN_DAY_MICROSECOND; }
DAY_MINUTE						{ return QC_TOKEN_DAY_MINUTE; }
DAY_SECOND						{ return QC_TOKEN_DAY_SECOND; }
DAY								{ return QC_TOKEN_DAY; }
DEALLOCATE						{ return QC_TOKEN_DEALLOCATE; }
DECIMAL_NUM						{ return QC_TOKEN_DECIMAL_NUM; }
DECIMAL							{ return QC_TOKEN_DECIMAL; }
DECLARE							{ return QC_TOKEN_DECLARE; }
DEFAULT							{ return QC_TOKEN_DEFAULT; }
DEFINER							{ return QC_TOKEN_DEFINER; }
DELAYED							{ return QC_TOKEN_DELAYED; }
DELAY_KEY_WRITE					{ return QC_TOKEN_DELAY_KEY_WRITE; }
DELETE							{ return QC_TOKEN_DELETE; }
DESC							{ return QC_TOKEN_DESC; }
DESCRIBE						{ return QC_TOKEN_DESCRIBE; }
DES_KEY_FILE					{ return QC_TOKEN_DES_KEY_FILE; }
DETERMINISTIC					{ return QC_TOKEN_DETERMINISTIC; }
DIRECTORY						{ return QC_TOKEN_DIRECTORY; }
DISABLE							{ return QC_TOKEN_DISABLE; }
DISCARD							{ return QC_TOKEN_DISCARD; }
DISK							{ return QC_TOKEN_DISK; }
DISTINCT						{ return QC_TOKEN_DISTINCT; }
DIV								{ return QC_TOKEN_DIV; }
DOUBLE							{ return QC_TOKEN_DOUBLE; }
DO								{ return QC_TOKEN_DO; }
DROP							{ return QC_TOKEN_DROP; }
DUAL							{ return QC_TOKEN_DUAL; }
DUMPFILE						{ return QC_TOKEN_DUMPFILE; }
DUPLICATE						{ return QC_TOKEN_DUPLICATE; }
DYNAMIC							{ return QC_TOKEN_DYNAMIC; }
EACH							{ return QC_TOKEN_EACH; }
ELSE							{ return QC_TOKEN_ELSE; }
ELSEIF							{ return QC_TOKEN_ELSEIF; }
ENABLE							{ return QC_TOKEN_ENABLE; }
ENCLOSED						{ return QC_TOKEN_ENCLOSED; }
END								{ return QC_TOKEN_END; }
ENDS							{ return QC_TOKEN_ENDS; }
END_OF_INPUT					{ return QC_TOKEN_END_OF_INPUT; }
ENGINES							{ return QC_TOKEN_ENGINES; }
ENGINE							{ return QC_TOKEN_ENGINE; }
ENUM							{ return QC_TOKEN_ENUM; }
EQ								{ return QC_TOKEN_EQ; }
EQUAL							{ return QC_TOKEN_EQUAL; }
ERRORS							{ return QC_TOKEN_ERRORS; }
ESCAPED							{ return QC_TOKEN_ESCAPED; }
ESCAPE							{ return QC_TOKEN_ESCAPE; }
EVENTS							{ return QC_TOKEN_EVENTS; }
EVENT							{ return QC_TOKEN_EVENT; }
EVERY							{ return QC_TOKEN_EVERY; }
EXECUTE							{ return QC_TOKEN_EXECUTE; }
EXISTS							{ return QC_TOKEN_EXISTS; }
EXIT							{ return QC_TOKEN_EXIT; }
EXPANSION						{ return QC_TOKEN_EXPANSION; }
EXTENDED						{ return QC_TOKEN_EXTENDED; }
EXTENT_SIZE						{ return QC_TOKEN_EXTENT_SIZE; }
EXTRACT							{ return QC_TOKEN_EXTRACT; }
FALSE							{ return QC_TOKEN_FALSE; }
FAST							{ return QC_TOKEN_FAST; }
FAULTS							{ return QC_TOKEN_FAULTS; }
FETCH							{ return QC_TOKEN_FETCH; }
FILE							{ return QC_TOKEN_FILE; }
FIRST							{ return QC_TOKEN_FIRST; }
FIXED							{ return QC_TOKEN_FIXED; }
FLOAT_NUM						{ return QC_TOKEN_FLOAT_NUM; }
FLOAT							{ return QC_TOKEN_FLOAT; }
FLUSH							{ return QC_TOKEN_FLUSH; }
FORCE							{ return QC_TOKEN_FORCE; }
FOREIGN							{ return QC_TOKEN_FOREIGN; }
FOR								{ return QC_TOKEN_FOR; }
FOUND							{ return QC_TOKEN_FOUND; }
FRAC_SECOND						{ return QC_TOKEN_FRAC_SECOND; }
FROM							{ return QC_TOKEN_FROM; }
FULL							{ return QC_TOKEN_FULL; }
FULLTEXT						{ return QC_TOKEN_FULLTEXT; }
FUNCTION						{ return QC_TOKEN_FUNCTION; }
GE								{ return QC_TOKEN_GE; }
GEOMETRYCOLLECTION				{ return QC_TOKEN_GEOMETRYCOLLECTION; }
GEOMETRY						{ return QC_TOKEN_GEOMETRY; }
GET_FORMAT						{ return QC_TOKEN_GET_FORMAT; }
GLOBAL							{ return QC_TOKEN_GLOBAL; }
GRANT							{ return QC_TOKEN_GRANT; }
GRANTS							{ return QC_TOKEN_GRANTS; }
GROUP							{ return QC_TOKEN_GROUP; }
GROUP_CONCAT					{ return QC_TOKEN_GROUP_CONCAT; }
GT								{ return QC_TOKEN_GT; }
HANDLER							{ return QC_TOKEN_HANDLER; }
HASH							{ return QC_TOKEN_HASH; }
HAVING							{ return QC_TOKEN_HAVING; }
HELP							{ return QC_TOKEN_HELP; }
HEX_NUM							{ return QC_TOKEN_HEX_NUM; }
HIGH_PRIORITY					{ return QC_TOKEN_HIGH_PRIORITY; }
HOST							{ return QC_TOKEN_HOST; }
HOSTS							{ return QC_TOKEN_HOSTS; }
HOUR_MICROSECOND				{ return QC_TOKEN_HOUR_MICROSECOND; }
HOUR_MINUTE						{ return QC_TOKEN_HOUR_MINUTE; }
HOUR_SECOND						{ return QC_TOKEN_HOUR_SECOND; }
HOUR							{ return QC_TOKEN_HOUR; }
IDENT							{ return QC_TOKEN_IDENT; }
IDENTIFIED						{ return QC_TOKEN_IDENTIFIED; }
IDENT_QUOTED					{ return QC_TOKEN_IDENT_QUOTED; }
IF								{ return QC_TOKEN_IF; }
IGNORE							{ return QC_TOKEN_IGNORE; }
IGNORE_SERVER_IDS				{ return QC_TOKEN_IGNORE_SERVER_IDS; }
IMPORT							{ return QC_TOKEN_IMPORT; }
INDEXES							{ return QC_TOKEN_INDEXES; }
INDEX							{ return QC_TOKEN_INDEX; }
INFILE							{ return QC_TOKEN_INFILE; }
INITIAL_SIZE					{ return QC_TOKEN_INITIAL_SIZE; }
INNER							{ return QC_TOKEN_INNER; }
INOUT							{ return QC_TOKEN_INOUT; }
INSENSITIVE						{ return QC_TOKEN_INSENSITIVE; }
INSERT							{ return QC_TOKEN_INSERT; }
INSERT_METHOD					{ return QC_TOKEN_INSERT_METHOD; }
INSTALL							{ return QC_TOKEN_INSTALL; }
INTERVAL						{ return QC_TOKEN_INTERVAL; }
INTO							{ return QC_TOKEN_INTO; }
INT								{ return QC_TOKEN_INT; }
INVOKER							{ return QC_TOKEN_INVOKER; }
IN								{ return QC_TOKEN_IN; }
IO								{ return QC_TOKEN_IO; }
IPC								{ return QC_TOKEN_IPC; }
IS								{ return QC_TOKEN_IS; }
ISOLATION						{ return QC_TOKEN_ISOLATION; }
ISSUER							{ return QC_TOKEN_ISSUER; }
ITERATE							{ return QC_TOKEN_ITERATE; }
JOIN							{ return QC_TOKEN_JOIN; }
KEYS							{ return QC_TOKEN_KEYS; }
KEY_BLOCK_SIZE					{ return QC_TOKEN_KEY_BLOCK_SIZE; }
KEY								{ return QC_TOKEN_KEY; }
KILL							{ return QC_TOKEN_KILL; }
LANGUAGE						{ return QC_TOKEN_LANGUAGE; }
LAST							{ return QC_TOKEN_LAST; }
LE								{ return QC_TOKEN_LE; }
LEADING							{ return QC_TOKEN_LEADING; }
LEAVES							{ return QC_TOKEN_LEAVES; }
LEAVE							{ return QC_TOKEN_LEAVE; }
LEFT							{ return QC_TOKEN_LEFT; }
LESS							{ return QC_TOKEN_LESS; }
LEVEL							{ return QC_TOKEN_LEVEL; }
LEX_HOSTNAME					{ return QC_TOKEN_LEX_HOSTNAME; }
LIKE							{ return QC_TOKEN_LIKE; }
LIMIT							{ return QC_TOKEN_LIMIT; }
LINEAR							{ return QC_TOKEN_LINEAR; }
LINES							{ return QC_TOKEN_LINES; }
LINESTRING						{ return QC_TOKEN_LINESTRING; }
LIST							{ return QC_TOKEN_LIST; }
LOAD							{ return QC_TOKEN_LOAD; }
LOCAL							{ return QC_TOKEN_LOCAL; }
LOCATOR							{ return QC_TOKEN_LOCATOR; }
LOCKS							{ return QC_TOKEN_LOCKS; }
LOCK							{ return QC_TOKEN_LOCK; }
LOGFILE							{ return QC_TOKEN_LOGFILE; }
LOGS							{ return QC_TOKEN_LOGS; }
LONGBLOB						{ return QC_TOKEN_LONGBLOB; }
LONGTEXT						{ return QC_TOKEN_LONGTEXT; }
LONG_NUM						{ return QC_TOKEN_LONG_NUM; }
LONG							{ return QC_TOKEN_LONG; }
LOOP							{ return QC_TOKEN_LOOP; }
LOW_PRIORITY					{ return QC_TOKEN_LOW_PRIORITY; }
LT								{ return QC_TOKEN_LT; }
MASTER_CONNECT_RETRY			{ return QC_TOKEN_MASTER_CONNECT_RETRY; }
MASTER_HOST						{ return QC_TOKEN_MASTER_HOST; }
MASTER_LOG_FILE					{ return QC_TOKEN_MASTER_LOG_FILE; }
MASTER_LOG_POS					{ return QC_TOKEN_MASTER_LOG_POS; }
MASTER_PASSWORD					{ return QC_TOKEN_MASTER_PASSWORD; }
MASTER_PORT						{ return QC_TOKEN_MASTER_PORT; }
MASTER_SERVER_ID				{ return QC_TOKEN_MASTER_SERVER_ID; }
MASTER_SSL_CAPATH				{ return QC_TOKEN_MASTER_SSL_CAPATH; }
MASTER_SSL_CA					{ return QC_TOKEN_MASTER_SSL_CA; }
MASTER_SSL_CERT					{ return QC_TOKEN_MASTER_SSL_CERT; }
MASTER_SSL_CIPHER				{ return QC_TOKEN_MASTER_SSL_CIPHER; }
MASTER_SSL_KEY					{ return QC_TOKEN_MASTER_SSL_KEY; }
MASTER_SSL						{ return QC_TOKEN_MASTER_SSL; }
MASTER_SSL_VERIFY_SERVER_CERT	{ return QC_TOKEN_MASTER_SSL_VERIFY_SERVER_CERT; }
MASTER							{ return QC_TOKEN_MASTER; }
MASTER_USER						{ return QC_TOKEN_MASTER_USER; }
MASTER_HEARTBEAT_PERIOD			{ return QC_TOKEN_MASTER_HEARTBEAT_PERIOD; }
MATCH							{ return QC_TOKEN_MATCH; }
MAX_CONNECTIONS_PER_HOUR		{ return QC_TOKEN_MAX_CONNECTIONS_PER_HOUR; }
MAX_QUERIES_PER_HOUR			{ return QC_TOKEN_MAX_QUERIES_PER_HOUR; }
MAX_ROWS						{ return QC_TOKEN_MAX_ROWS; }
MAX_SIZE						{ return QC_TOKEN_MAX_SIZE; }
MAX								{ return QC_TOKEN_MAX; }
MAX_UPDATES_PER_HOUR			{ return QC_TOKEN_MAX_UPDATES_PER_HOUR; }
MAX_USER_CONNECTIONS			{ return QC_TOKEN_MAX_USER_CONNECTIONS; }
MAX_VALUE						{ return QC_TOKEN_MAX_VALUE; }
MEDIUMBLOB						{ return QC_TOKEN_MEDIUMBLOB; }
MEDIUMINT						{ return QC_TOKEN_MEDIUMINT; }
MEDIUMTEXT						{ return QC_TOKEN_MEDIUMTEXT; }
MEDIUM							{ return QC_TOKEN_MEDIUM; }
MEMORY							{ return QC_TOKEN_MEMORY; }
MERGE							{ return QC_TOKEN_MERGE; }
MESSAGE_TEXT					{ return QC_TOKEN_MESSAGE_TEXT; }
MICROSECOND						{ return QC_TOKEN_MICROSECOND; }
MIGRATE							{ return QC_TOKEN_MIGRATE; }
MINUTE_MICROSECOND				{ return QC_TOKEN_MINUTE_MICROSECOND; }
MINUTE_SECOND					{ return QC_TOKEN_MINUTE_SECOND; }
MINUTE							{ return QC_TOKEN_MINUTE; }
MIN_ROWS						{ return QC_TOKEN_MIN_ROWS; }
MIN								{ return QC_TOKEN_MIN; }
MODE							{ return QC_TOKEN_MODE; }
MODIFIES						{ return QC_TOKEN_MODIFIES; }
MODIFY							{ return QC_TOKEN_MODIFY; }
MOD								{ return QC_TOKEN_MOD; }
MONTH							{ return QC_TOKEN_MONTH; }
MULTILINESTRING					{ return QC_TOKEN_MULTILINESTRING; }
MULTIPOINT						{ return QC_TOKEN_MULTIPOINT; }
MULTIPOLYGON					{ return QC_TOKEN_MULTIPOLYGON; }
MUTEX							{ return QC_TOKEN_MUTEX; }
MYSQL_ERRNO						{ return QC_TOKEN_MYSQL_ERRNO; }
NAMES							{ return QC_TOKEN_NAMES; }
NAME							{ return QC_TOKEN_NAME; }
NATIONAL						{ return QC_TOKEN_NATIONAL; }
NATURAL							{ return QC_TOKEN_NATURAL; }
NCHAR_STRING					{ return QC_TOKEN_NCHAR_STRING; }
NCHAR							{ return QC_TOKEN_NCHAR; }
NDBCLUSTER						{ return QC_TOKEN_NDBCLUSTER; }
NE								{ return QC_TOKEN_NE; }
NEG								{ return QC_TOKEN_NEG; }
NEW								{ return QC_TOKEN_NEW; }
NEXT							{ return QC_TOKEN_NEXT; }
NODEGROUP						{ return QC_TOKEN_NODEGROUP; }
NONE							{ return QC_TOKEN_NONE; }
NOT2							{ return QC_TOKEN_NOT2; }
NOT								{ return QC_TOKEN_NOT; }
NOW								{ return QC_TOKEN_NOW; }
NO								{ return QC_TOKEN_NO; }
NO_WAIT							{ return QC_TOKEN_NO_WAIT; }
NO_WRITE_TO_BINLOG				{ return QC_TOKEN_NO_WRITE_TO_BINLOG; }
NULL							{ return QC_TOKEN_NULL; }
NUM								{ return QC_TOKEN_NUM; }
NUMERIC							{ return QC_TOKEN_NUMERIC; }
NVARCHAR						{ return QC_TOKEN_NVARCHAR; }
OFFSET							{ return QC_TOKEN_OFFSET; }
OLD_PASSWORD					{ return QC_TOKEN_OLD_PASSWORD; }
ON								{ return QC_TOKEN_ON; }
ONE_SHOT						{ return QC_TOKEN_ONE_SHOT; }
ONE								{ return QC_TOKEN_ONE; }
OPEN							{ return QC_TOKEN_OPEN; }
OPTIMIZE						{ return QC_TOKEN_OPTIMIZE; }
OPTIONS							{ return QC_TOKEN_OPTIONS; }
OPTION							{ return QC_TOKEN_OPTION; }
OPTIONALLY						{ return QC_TOKEN_OPTIONALLY; }
OR2								{ return QC_TOKEN_OR2; }
ORDER							{ return QC_TOKEN_ORDER; }
OR_OR							{ return QC_TOKEN_OR_OR; }
OR								{ return QC_TOKEN_OR; }
OUTER							{ return QC_TOKEN_OUTER; }
OUTFILE							{ return QC_TOKEN_OUTFILE; }
OUT								{ return QC_TOKEN_OUT; }
OWNER							{ return QC_TOKEN_OWNER; }
PACK_KEYS						{ return QC_TOKEN_PACK_KEYS; }
PAGE							{ return QC_TOKEN_PAGE; }
PARAM_MARKER					{ return QC_TOKEN_PARAM_MARKER; }
PARSER							{ return QC_TOKEN_PARSER; }
PARTIAL							{ return QC_TOKEN_PARTIAL; }
PARTITIONING					{ return QC_TOKEN_PARTITIONING; }
PARTITIONS						{ return QC_TOKEN_PARTITIONS; }
PARTITION						{ return QC_TOKEN_PARTITION; }
PASSWORD						{ return QC_TOKEN_PASSWORD; }
PHASE							{ return QC_TOKEN_PHASE; }
PLUGINS							{ return QC_TOKEN_PLUGINS; }
PLUGIN							{ return QC_TOKEN_PLUGIN; }
POINT							{ return QC_TOKEN_POINT; }
POLYGON							{ return QC_TOKEN_POLYGON; }
PORT							{ return QC_TOKEN_PORT; }
POSITION						{ return QC_TOKEN_POSITION; }
PRECISION						{ return QC_TOKEN_PRECISION; }
PREPARE							{ return QC_TOKEN_PREPARE; }
PRESERVE						{ return QC_TOKEN_PRESERVE; }
PREV							{ return QC_TOKEN_PREV; }
PRIMARY							{ return QC_TOKEN_PRIMARY; }
PRIVILEGES						{ return QC_TOKEN_PRIVILEGES; }
PROCEDURE						{ return QC_TOKEN_PROCEDURE; }
PROCESS							{ return QC_TOKEN_PROCESS; }
PROCESSLIST						{ return QC_TOKEN_PROCESSLIST; }
PROFILE							{ return QC_TOKEN_PROFILE; }
PROFILES						{ return QC_TOKEN_PROFILES; }
PURGE							{ return QC_TOKEN_PURGE; }
QUARTER							{ return QC_TOKEN_QUARTER; }
QUERY							{ return QC_TOKEN_QUERY; }
QUICK							{ return QC_TOKEN_QUICK; }
RANGE							{ return QC_TOKEN_RANGE; }
READS							{ return QC_TOKEN_READS; }
READ_ONLY						{ return QC_TOKEN_READ_ONLY; }
READ							{ return QC_TOKEN_READ; }
READ_WRITE						{ return QC_TOKEN_READ_WRITE; }
REAL							{ return QC_TOKEN_REAL; }
REBUILD							{ return QC_TOKEN_REBUILD; }
RECOVER							{ return QC_TOKEN_RECOVER; }
REDOFILE						{ return QC_TOKEN_REDOFILE; }
REDO_BUFFER_SIZE				{ return QC_TOKEN_REDO_BUFFER_SIZE; }
REDUNDANT						{ return QC_TOKEN_REDUNDANT; }
REFERENCES						{ return QC_TOKEN_REFERENCES; }
REGEXP							{ return QC_TOKEN_REGEXP; }
RELAYLOG						{ return QC_TOKEN_RELAYLOG; }
RELAY_LOG_FILE					{ return QC_TOKEN_RELAY_LOG_FILE; }
RELAY_LOG_POS					{ return QC_TOKEN_RELAY_LOG_POS; }
RELAY_THREAD					{ return QC_TOKEN_RELAY_THREAD; }
RELEASE							{ return QC_TOKEN_RELEASE; }
RELOAD							{ return QC_TOKEN_RELOAD; }
REMOVE							{ return QC_TOKEN_REMOVE; }
RENAME							{ return QC_TOKEN_RENAME; }
REORGANIZE						{ return QC_TOKEN_REORGANIZE; }
REPAIR							{ return QC_TOKEN_REPAIR; }
REPEATABLE						{ return QC_TOKEN_REPEATABLE; }
REPEAT							{ return QC_TOKEN_REPEAT; }
REPLACE							{ return QC_TOKEN_REPLACE; }
REPLICATION						{ return QC_TOKEN_REPLICATION; }
REQUIRE							{ return QC_TOKEN_REQUIRE; }
RESET							{ return QC_TOKEN_RESET; }
RESIGNAL						{ return QC_TOKEN_RESIGNAL; }
RESOURCES						{ return QC_TOKEN_RESOURCES; }
RESTORE							{ return QC_TOKEN_RESTORE; }
RESTRICT						{ return QC_TOKEN_RESTRICT; }
RESUME							{ return QC_TOKEN_RESUME; }
RETURNS							{ return QC_TOKEN_RETURNS; }
RETURN							{ return QC_TOKEN_RETURN; }
REVOKE							{ return QC_TOKEN_REVOKE; }
RIGHT							{ return QC_TOKEN_RIGHT; }
ROLLBACK						{ return QC_TOKEN_ROLLBACK; }
ROLLUP							{ return QC_TOKEN_ROLLUP; }
ROUTINE							{ return QC_TOKEN_ROUTINE; }
ROWS							{ return QC_TOKEN_ROWS; }
ROW_FORMAT						{ return QC_TOKEN_ROW_FORMAT; }
ROW								{ return QC_TOKEN_ROW; }
RTREE							{ return QC_TOKEN_RTREE; }
SAVEPOINT						{ return QC_TOKEN_SAVEPOINT; }
SCHEDULE						{ return QC_TOKEN_SCHEDULE; }
SCHEMA_NAME						{ return QC_TOKEN_SCHEMA_NAME; }
SECOND_MICROSECOND				{ return QC_TOKEN_SECOND_MICROSECOND; }
SECOND							{ return QC_TOKEN_SECOND; }
SECURITY						{ return QC_TOKEN_SECURITY; }
SELECT							{ return QC_TOKEN_SELECT; }
SENSITIVE						{ return QC_TOKEN_SENSITIVE; }
SEPARATOR						{ return QC_TOKEN_SEPARATOR; }
SERIALIZABLE					{ return QC_TOKEN_SERIALIZABLE; }
SERIAL							{ return QC_TOKEN_SERIAL; }
SESSION							{ return QC_TOKEN_SESSION; }
SERVER							{ return QC_TOKEN_SERVER; }
SERVER_OPTIONS					{ return QC_TOKEN_SERVER_OPTIONS; }
SET								{ return QC_TOKEN_SET; }
SET_VAR							{ return QC_TOKEN_SET_VAR; }
SHARE							{ return QC_TOKEN_SHARE; }
SHIFT_LEFT						{ return QC_TOKEN_SHIFT_LEFT; }
SHIFT_RIGHT						{ return QC_TOKEN_SHIFT_RIGHT; }
SHOW							{ return QC_TOKEN_SHOW; }
SHUTDOWN						{ return QC_TOKEN_SHUTDOWN; }
SIGNAL							{ return QC_TOKEN_SIGNAL; }
SIGNED							{ return QC_TOKEN_SIGNED; }
SIMPLE							{ return QC_TOKEN_SIMPLE; }
SLAVE							{ return QC_TOKEN_SLAVE; }
SMALLINT						{ return QC_TOKEN_SMALLINT; }
SNAPSHOT						{ return QC_TOKEN_SNAPSHOT; }
SOCKET							{ return QC_TOKEN_SOCKET; }
SONAME							{ return QC_TOKEN_SONAME; }
SOUNDS							{ return QC_TOKEN_SOUNDS; }
SOURCE							{ return QC_TOKEN_SOURCE; }
SPATIAL							{ return QC_TOKEN_SPATIAL; }
SPECIFIC						{ return QC_TOKEN_SPECIFIC; }
SQLEXCEPTION					{ return QC_TOKEN_SQLEXCEPTION; }
SQLSTATE						{ return QC_TOKEN_SQLSTATE; }
SQLWARNING						{ return QC_TOKEN_SQLWARNING; }
SQL_BIG_RESULT					{ return QC_TOKEN_SQL_BIG_RESULT; }
SQL_BUFFER_RESULT				{ return QC_TOKEN_SQL_BUFFER_RESULT; }
SQL_CACHE						{ return QC_TOKEN_SQL_CACHE; }
SQL_CALC_FOUND_ROWS				{ return QC_TOKEN_SQL_CALC_FOUND_ROWS; }
SQL_NO_CACHE					{ return QC_TOKEN_SQL_NO_CACHE; }
SQL_SMALL_RESULT				{ return QC_TOKEN_SQL_SMALL_RESULT; }
SQL								{ return QC_TOKEN_SQL; }
SQL_THREAD						{ return QC_TOKEN_SQL_THREAD; }
SSL								{ return QC_TOKEN_SSL; }
STARTING						{ return QC_TOKEN_STARTING; }
STARTS							{ return QC_TOKEN_STARTS; }
START							{ return QC_TOKEN_START; }
STATUS							{ return QC_TOKEN_STATUS; }
STDDEV_SAMP						{ return QC_TOKEN_STDDEV_SAMP; }
STD								{ return QC_TOKEN_STD; }
STOP							{ return QC_TOKEN_STOP; }
STORAGE							{ return QC_TOKEN_STORAGE; }
STRAIGHT_JOIN					{ return QC_TOKEN_STRAIGHT_JOIN; }
STRING							{ return QC_TOKEN_STRING; }
SUBCLASS_ORIGIN					{ return QC_TOKEN_SUBCLASS_ORIGIN; }
SUBDATE							{ return QC_TOKEN_SUBDATE; }
SUBJECT							{ return QC_TOKEN_SUBJECT; }
SUBPARTITIONS					{ return QC_TOKEN_SUBPARTITIONS; }
SUBPARTITION					{ return QC_TOKEN_SUBPARTITION; }
SUBSTRING						{ return QC_TOKEN_SUBSTRING; }
SUM								{ return QC_TOKEN_SUM; }
SUPER							{ return QC_TOKEN_SUPER; }
SUSPEND							{ return QC_TOKEN_SUSPEND; }
SWAPS							{ return QC_TOKEN_SWAPS; }
SWITCHES						{ return QC_TOKEN_SWITCHES; }
SYSDATE							{ return QC_TOKEN_SYSDATE; }
TABLES							{ return QC_TOKEN_TABLES; }
TABLESPACE						{ return QC_TOKEN_TABLESPACE; }
TABLE_REF_PRIORITY				{ return QC_TOKEN_TABLE_REF_PRIORITY; }
TABLE							{ return QC_TOKEN_TABLE; }
TABLE_CHECKSUM					{ return QC_TOKEN_TABLE_CHECKSUM; }
TABLE_NAME						{ return QC_TOKEN_TABLE_NAME; }
TEMPORARY						{ return QC_TOKEN_TEMPORARY; }
TEMPTABLE						{ return QC_TOKEN_TEMPTABLE; }
TERMINATED						{ return QC_TOKEN_TERMINATED; }
TEXT_STRING						{ return QC_TOKEN_TEXT_STRING; }
TEXT							{ return QC_TOKEN_TEXT; }
THAN							{ return QC_TOKEN_THAN; }
THEN							{ return QC_TOKEN_THEN; }
TIMESTAMP						{ return QC_TOKEN_TIMESTAMP; }
TIMESTAMP_ADD					{ return QC_TOKEN_TIMESTAMP_ADD; }
TIMESTAMP_DIFF					{ return QC_TOKEN_TIMESTAMP_DIFF; }
TIME							{ return QC_TOKEN_TIME; }
TINYBLOB						{ return QC_TOKEN_TINYBLOB; }
TINYINT							{ return QC_TOKEN_TINYINT; }
TINYTEXT						{ return QC_TOKEN_TINYTEXT; }
TO								{ return QC_TOKEN_TO; }
TRAILING						{ return QC_TOKEN_TRAILING; }
TRANSACTION						{ return QC_TOKEN_TRANSACTION; }
TRIGGERS						{ return QC_TOKEN_TRIGGERS; }
TRIGGER							{ return QC_TOKEN_TRIGGER; }
TRIM							{ return QC_TOKEN_TRIM; }
TRUE							{ return QC_TOKEN_TRUE; }
TRUNCATE						{ return QC_TOKEN_TRUNCATE; }
TYPES							{ return QC_TOKEN_TYPES; }
TYPE							{ return QC_TOKEN_TYPE; }
UDF_RETURNS						{ return QC_TOKEN_UDF_RETURNS; }
ULONGLONG_NUM					{ return QC_TOKEN_ULONGLONG_NUM; }
UNCOMMITTED						{ return QC_TOKEN_UNCOMMITTED; }
UNDEFINED						{ return QC_TOKEN_UNDEFINED; }
UNDERSCORE_CHARSET				{ return QC_TOKEN_UNDERSCORE_CHARSET; }
UNDOFILE						{ return QC_TOKEN_UNDOFILE; }
UNDO_BUFFER_SIZE				{ return QC_TOKEN_UNDO_BUFFER_SIZE; }
UNDO							{ return QC_TOKEN_UNDO; }
UNICODE							{ return QC_TOKEN_UNICODE; }
UNINSTALL						{ return QC_TOKEN_UNINSTALL; }
UNION							{ return QC_TOKEN_UNION; }
UNIQUE							{ return QC_TOKEN_UNIQUE; }
UNKNOWN							{ return QC_TOKEN_UNKNOWN; }
UNLOCK							{ return QC_TOKEN_UNLOCK; }
UNSIGNED						{ return QC_TOKEN_UNSIGNED; }
UNTIL							{ return QC_TOKEN_UNTIL; }
UPDATE							{ return QC_TOKEN_UPDATE; }
UPGRADE							{ return QC_TOKEN_UPGRADE; }
USAGE							{ return QC_TOKEN_USAGE; }
USER							{ return QC_TOKEN_USER; }
USE_FRM							{ return QC_TOKEN_USE_FRM; }
USE								{ return QC_TOKEN_USE; }
USING							{ return QC_TOKEN_USING; }
UTC_DATE						{ return QC_TOKEN_UTC_DATE; }
UTC_TIMESTAMP					{ return QC_TOKEN_UTC_TIMESTAMP; }
UTC_TIME						{ return QC_TOKEN_UTC_TIME; }
VALUES							{ return QC_TOKEN_VALUES; }
VALUE							{ return QC_TOKEN_VALUE; }
VARBINARY						{ return QC_TOKEN_VARBINARY; }
VARCHAR							{ return QC_TOKEN_VARCHAR; }
VARIABLES						{ return QC_TOKEN_VARIABLES; }
VARIANCE						{ return QC_TOKEN_VARIANCE; }
VARYING							{ return QC_TOKEN_VARYING; }
VAR_SAMP						{ return QC_TOKEN_VAR_SAMP; }
VIEW							{ return QC_TOKEN_VIEW; }
WAIT							{ return QC_TOKEN_WAIT; }
WARNINGS						{ return QC_TOKEN_WARNINGS; }
WEEK							{ return QC_TOKEN_WEEK; }
WHEN							{ return QC_TOKEN_WHEN; }
WHERE							{ return QC_TOKEN_WHERE; }
WHILE							{ return QC_TOKEN_WHILE; }
WITH							{ return QC_TOKEN_WITH; }
WITH_CUBE						{ return QC_TOKEN_WITH_CUBE; }
WITH_ROLLUP						{ return QC_TOKEN_WITH_ROLLUP; }
WORK							{ return QC_TOKEN_WORK; }
WRAPPER							{ return QC_TOKEN_WRAPPER; }
WRITE							{ return QC_TOKEN_WRITE; }
X509							{ return QC_TOKEN_X509; }
XA								{ return QC_TOKEN_XA; }
XML								{ return QC_TOKEN_XML; }
XOR								{ return QC_TOKEN_XOR; }
YEAR_MONTH						{ return QC_TOKEN_YEAR_MONTH; }
YEAR							{ return QC_TOKEN_YEAR; }
ZEROFILL						{ return QC_TOKEN_ZEROFILL; }
CLIENT_FLAG						{ return QC_TOKEN_CLIENT_FLAG; }
GLOBAL_VAR						{ return QC_TOKEN_GLOBAL_VAR; }
SESSION_VAR						{ return QC_TOKEN_SESSION_VAR; }
BRACKET_OPEN					{ return QC_TOKEN_BRACKET_OPEN; }
BRACKET_CLOSE					{ return QC_TOKEN_BRACKET_CLOSE; }
PLUS							{ return QC_TOKEN_PLUS; }
MINUS							{ return QC_TOKEN_MINUS; }
STAR							{ return QC_TOKEN_STAR; }
COMMA							{ return QC_TOKEN_COMMA; }
DOT								{ return QC_TOKEN_DOT; }
SEMICOLON						{ return QC_TOKEN_SEMICOLON; }
NO_MORE							{ return QC_TOKEN_NO_MORE; }

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
\|								{ return QC_TOKEN_BIT_OR; }
\&								{ return QC_TOKEN_BIT_AND; }
\~								{ return QC_TOKEN_TILDE; }

"&&"							{ return QC_TOKEN_AND; }
"||"							{ return QC_TOKEN_OR; }
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
	if ((ret.token = yylex(*(yyscan_t *)scanner->scanner))) {
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

