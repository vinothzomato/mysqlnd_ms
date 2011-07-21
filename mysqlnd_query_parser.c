
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         mysqlnd_qp_parse
#define yylex           mysqlnd_qp_lex
#define yyerror         mysqlnd_qp_error
#define yylval          mysqlnd_qp_lval
#define yychar          mysqlnd_qp_char
#define yydebug         mysqlnd_qp_debug
#define yynerrs         mysqlnd_qp_nerrs


/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "mysqlnd_query_parser.grammar"

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

#include "php.h"
#include "mysqlnd_ms.h"
#include "zend_llist.h"
/* Compile with : bison -o mysqlnd_query_parser.c -d mysqlnd_query_parser.grammar --name-prefix=mysqlnd_qp_ */

#define yyerror mysqlnd_qp_error
extern int mysqlnd_qp_error(const char * format, ...);

#if defined(PHP_DEBUG) && !defined(YYDEBUG)
#define YYDEBUG 1
#else
#define YYDEBUG 0
#endif

#define YYPARSE_PARAM my_parser TSRMLS_DC
#define PINFO (((struct st_mysqlnd_query_parser *) my_parser)->parse_info)
#define YYLEX_PARAM *(yyscan_t *)(((struct st_mysqlnd_query_parser *) my_parser)->scanner->scanner) TSRMLS_CC



/* Line 189 of yacc.c  */
#line 122 "mysqlnd_query_parser.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     QC_TOKEN_ACCESSIBLE = 258,
     QC_TOKEN_ACTION = 259,
     QC_TOKEN_ADD = 260,
     QC_TOKEN_ADDDATE = 261,
     QC_TOKEN_AFTER = 262,
     QC_TOKEN_AGAINST = 263,
     QC_TOKEN_AGGREGATE = 264,
     QC_TOKEN_ALGORITHM = 265,
     QC_TOKEN_ALL = 266,
     QC_TOKEN_ALTER = 267,
     QC_TOKEN_ANALYZE = 268,
     QC_TOKEN_AND_AND = 269,
     QC_TOKEN_AND = 270,
     QC_TOKEN_BETWEEN_AND = 271,
     QC_TOKEN_ANY = 272,
     QC_TOKEN_AS = 273,
     QC_TOKEN_ASC = 274,
     QC_TOKEN_ASCII = 275,
     QC_TOKEN_ASENSITIVE = 276,
     QC_TOKEN_AT = 277,
     QC_TOKEN_AUTHORS = 278,
     QC_TOKEN_AUTOEXTEND_SIZE = 279,
     QC_TOKEN_AUTO_INC = 280,
     QC_TOKEN_AVG_ROW_LENGTH = 281,
     QC_TOKEN_AVG = 282,
     QC_TOKEN_BACKUP = 283,
     QC_TOKEN_BEFORE = 284,
     QC_TOKEN_BEGIN = 285,
     QC_TOKEN_BETWEEN = 286,
     QC_TOKEN_BIGINT = 287,
     QC_TOKEN_BINARY = 288,
     QC_TOKEN_BINLOG = 289,
     QC_TOKEN_BIN_NUM = 290,
     QC_TOKEN_BIT_AND = 291,
     QC_TOKEN_BIT_OR = 292,
     QC_TOKEN_BIT = 293,
     QC_TOKEN_BIT_XOR = 294,
     QC_TOKEN_BLOB = 295,
     QC_TOKEN_BLOCK = 296,
     QC_TOKEN_BOOLEAN = 297,
     QC_TOKEN_BOOL = 298,
     QC_TOKEN_BOTH = 299,
     QC_TOKEN_BTREE = 300,
     QC_TOKEN_BY = 301,
     QC_TOKEN_BYTE = 302,
     QC_TOKEN_CACHE = 303,
     QC_TOKEN_CALL = 304,
     QC_TOKEN_CASCADE = 305,
     QC_TOKEN_CASCADED = 306,
     QC_TOKEN_CASE = 307,
     QC_TOKEN_CAST = 308,
     QC_TOKEN_CATALOG_NAME = 309,
     QC_TOKEN_CHAIN = 310,
     QC_TOKEN_CHANGE = 311,
     QC_TOKEN_CHANGED = 312,
     QC_TOKEN_CHARSET = 313,
     QC_TOKEN_CHAR = 314,
     QC_TOKEN_CHECKSUM = 315,
     QC_TOKEN_CHECK = 316,
     QC_TOKEN_CIPHER = 317,
     QC_TOKEN_CLASS_ORIGIN = 318,
     QC_TOKEN_CLIENT = 319,
     QC_TOKEN_CLOSE = 320,
     QC_TOKEN_COALESCE = 321,
     QC_TOKEN_CODE = 322,
     QC_TOKEN_COLLATE = 323,
     QC_TOKEN_COLLATION = 324,
     QC_TOKEN_COLUMNS = 325,
     QC_TOKEN_COLUMN = 326,
     QC_TOKEN_COLUMN_NAME = 327,
     QC_TOKEN_COMMENT = 328,
     QC_TOKEN_COMMITTED = 329,
     QC_TOKEN_COMMIT = 330,
     QC_TOKEN_COMPACT = 331,
     QC_TOKEN_COMPLETION = 332,
     QC_TOKEN_COMPRESSED = 333,
     QC_TOKEN_CONCURRENT = 334,
     QC_TOKEN_CONDITION = 335,
     QC_TOKEN_CONNECTION = 336,
     QC_TOKEN_CONSISTENT = 337,
     QC_TOKEN_CONSTRAINT = 338,
     QC_TOKEN_CONSTRAINT_CATALOG = 339,
     QC_TOKEN_CONSTRAINT_NAME = 340,
     QC_TOKEN_CONSTRAINT_SCHEMA = 341,
     QC_TOKEN_CONTAINS = 342,
     QC_TOKEN_CONTEXT = 343,
     QC_TOKEN_CONTINUE = 344,
     QC_TOKEN_CONTRIBUTORS = 345,
     QC_TOKEN_CONVERT = 346,
     QC_TOKEN_COUNT = 347,
     QC_TOKEN_CPU = 348,
     QC_TOKEN_CREATE = 349,
     QC_TOKEN_CROSS = 350,
     QC_TOKEN_CUBE = 351,
     QC_TOKEN_CURDATE = 352,
     QC_TOKEN_CURRENT_USER = 353,
     QC_TOKEN_CURSOR = 354,
     QC_TOKEN_CURSOR_NAME = 355,
     QC_TOKEN_CURTIME = 356,
     QC_TOKEN_DATABASE = 357,
     QC_TOKEN_DATABASES = 358,
     QC_TOKEN_DATAFILE = 359,
     QC_TOKEN_DATA = 360,
     QC_TOKEN_DATETIME = 361,
     QC_TOKEN_DATE_ADD_INTERVAL = 362,
     QC_TOKEN_DATE_SUB_INTERVAL = 363,
     QC_TOKEN_DATE = 364,
     QC_TOKEN_DAY_HOUR = 365,
     QC_TOKEN_DAY_MICROSECOND = 366,
     QC_TOKEN_DAY_MINUTE = 367,
     QC_TOKEN_DAY_SECOND = 368,
     QC_TOKEN_DAY = 369,
     QC_TOKEN_DEALLOCATE = 370,
     QC_TOKEN_DECIMAL_NUM = 371,
     QC_TOKEN_DECIMAL = 372,
     QC_TOKEN_DECLARE = 373,
     QC_TOKEN_DEFAULT = 374,
     QC_TOKEN_DEFINER = 375,
     QC_TOKEN_DELAYED = 376,
     QC_TOKEN_DELAY_KEY_WRITE = 377,
     QC_TOKEN_DELETE = 378,
     QC_TOKEN_DESC = 379,
     QC_TOKEN_DESCRIBE = 380,
     QC_TOKEN_DES_KEY_FILE = 381,
     QC_TOKEN_DETERMINISTIC = 382,
     QC_TOKEN_DIRECTORY = 383,
     QC_TOKEN_DISABLE = 384,
     QC_TOKEN_DISCARD = 385,
     QC_TOKEN_DISK = 386,
     QC_TOKEN_DISTINCT = 387,
     QC_TOKEN_DIV = 388,
     QC_TOKEN_DOUBLE = 389,
     QC_TOKEN_DO = 390,
     QC_TOKEN_DROP = 391,
     QC_TOKEN_DUAL = 392,
     QC_TOKEN_DUMPFILE = 393,
     QC_TOKEN_DUPLICATE = 394,
     QC_TOKEN_DYNAMIC = 395,
     QC_TOKEN_EACH = 396,
     QC_TOKEN_ELSE = 397,
     QC_TOKEN_ELSEIF = 398,
     QC_TOKEN_ENABLE = 399,
     QC_TOKEN_ENCLOSED = 400,
     QC_TOKEN_END = 401,
     QC_TOKEN_ENDS = 402,
     QC_TOKEN_END_OF_INPUT = 403,
     QC_TOKEN_ENGINES = 404,
     QC_TOKEN_ENGINE = 405,
     QC_TOKEN_ENUM = 406,
     QC_TOKEN_EQ = 407,
     QC_TOKEN_EQUAL = 408,
     QC_TOKEN_ERRORS = 409,
     QC_TOKEN_ESCAPED = 410,
     QC_TOKEN_ESCAPE = 411,
     QC_TOKEN_EVENTS = 412,
     QC_TOKEN_EVENT = 413,
     QC_TOKEN_EVERY = 414,
     QC_TOKEN_EXECUTE = 415,
     QC_TOKEN_EXISTS = 416,
     QC_TOKEN_EXIT = 417,
     QC_TOKEN_EXPANSION = 418,
     QC_TOKEN_EXTENDED = 419,
     QC_TOKEN_EXTENT_SIZE = 420,
     QC_TOKEN_EXTRACT = 421,
     QC_TOKEN_FALSE = 422,
     QC_TOKEN_FAST = 423,
     QC_TOKEN_FAULTS = 424,
     QC_TOKEN_FETCH = 425,
     QC_TOKEN_FILE = 426,
     QC_TOKEN_FIRST = 427,
     QC_TOKEN_FIXED = 428,
     QC_TOKEN_FLOAT_NUM = 429,
     QC_TOKEN_FLOAT = 430,
     QC_TOKEN_FLUSH = 431,
     QC_TOKEN_FORCE = 432,
     QC_TOKEN_FOREIGN = 433,
     QC_TOKEN_FOR = 434,
     QC_TOKEN_FOUND = 435,
     QC_TOKEN_FRAC_SECOND = 436,
     QC_TOKEN_FROM = 437,
     QC_TOKEN_FULL = 438,
     QC_TOKEN_FULLTEXT = 439,
     QC_TOKEN_FUNCTION = 440,
     QC_TOKEN_GE = 441,
     QC_TOKEN_GEOMETRYCOLLECTION = 442,
     QC_TOKEN_GEOMETRY = 443,
     QC_TOKEN_GET_FORMAT = 444,
     QC_TOKEN_GLOBAL = 445,
     QC_TOKEN_GRANT = 446,
     QC_TOKEN_GRANTS = 447,
     QC_TOKEN_GROUP = 448,
     QC_TOKEN_GROUP_CONCAT = 449,
     QC_TOKEN_GT = 450,
     QC_TOKEN_HANDLER = 451,
     QC_TOKEN_HASH = 452,
     QC_TOKEN_HAVING = 453,
     QC_TOKEN_HELP = 454,
     QC_TOKEN_HEX_NUM = 455,
     QC_TOKEN_HIGH_PRIORITY = 456,
     QC_TOKEN_HOST = 457,
     QC_TOKEN_HOSTS = 458,
     QC_TOKEN_HOUR_MICROSECOND = 459,
     QC_TOKEN_HOUR_MINUTE = 460,
     QC_TOKEN_HOUR_SECOND = 461,
     QC_TOKEN_HOUR = 462,
     QC_TOKEN_IDENT = 463,
     QC_TOKEN_IDENTIFIED = 464,
     QC_TOKEN_IDENT_QUOTED = 465,
     QC_TOKEN_IF = 466,
     QC_TOKEN_IGNORE = 467,
     QC_TOKEN_IGNORE_SERVER_IDS = 468,
     QC_TOKEN_IMPORT = 469,
     QC_TOKEN_INDEXES = 470,
     QC_TOKEN_INDEX = 471,
     QC_TOKEN_INFILE = 472,
     QC_TOKEN_INITIAL_SIZE = 473,
     QC_TOKEN_INNER = 474,
     QC_TOKEN_INOUT = 475,
     QC_TOKEN_INSENSITIVE = 476,
     QC_TOKEN_INSERT = 477,
     QC_TOKEN_INSERT_METHOD = 478,
     QC_TOKEN_INSTALL = 479,
     QC_TOKEN_INTERVAL = 480,
     QC_TOKEN_INTO = 481,
     QC_TOKEN_INT = 482,
     QC_TOKEN_INVOKER = 483,
     QC_TOKEN_IN = 484,
     QC_TOKEN_IO = 485,
     QC_TOKEN_IPC = 486,
     QC_TOKEN_IS = 487,
     QC_TOKEN_ISOLATION = 488,
     QC_TOKEN_ISSUER = 489,
     QC_TOKEN_ITERATE = 490,
     QC_TOKEN_JOIN = 491,
     QC_TOKEN_KEYS = 492,
     QC_TOKEN_KEY_BLOCK_SIZE = 493,
     QC_TOKEN_KEY = 494,
     QC_TOKEN_KILL = 495,
     QC_TOKEN_LANGUAGE = 496,
     QC_TOKEN_LAST = 497,
     QC_TOKEN_LE = 498,
     QC_TOKEN_LEADING = 499,
     QC_TOKEN_LEAVES = 500,
     QC_TOKEN_LEAVE = 501,
     QC_TOKEN_LEFT = 502,
     QC_TOKEN_LESS = 503,
     QC_TOKEN_LEVEL = 504,
     QC_TOKEN_LEX_HOSTNAME = 505,
     QC_TOKEN_LIKE = 506,
     QC_TOKEN_LIMIT = 507,
     QC_TOKEN_LINEAR = 508,
     QC_TOKEN_LINES = 509,
     QC_TOKEN_LINESTRING = 510,
     QC_TOKEN_LIST = 511,
     QC_TOKEN_LOAD = 512,
     QC_TOKEN_LOCAL = 513,
     QC_TOKEN_LOCATOR = 514,
     QC_TOKEN_LOCKS = 515,
     QC_TOKEN_LOCK = 516,
     QC_TOKEN_LOGFILE = 517,
     QC_TOKEN_LOGS = 518,
     QC_TOKEN_LONGBLOB = 519,
     QC_TOKEN_LONGTEXT = 520,
     QC_TOKEN_LONG_NUM = 521,
     QC_TOKEN_LONG = 522,
     QC_TOKEN_LOOP = 523,
     QC_TOKEN_LOW_PRIORITY = 524,
     QC_TOKEN_LT = 525,
     QC_TOKEN_MASTER_CONNECT_RETRY = 526,
     QC_TOKEN_MASTER_HOST = 527,
     QC_TOKEN_MASTER_LOG_FILE = 528,
     QC_TOKEN_MASTER_LOG_POS = 529,
     QC_TOKEN_MASTER_PASSWORD = 530,
     QC_TOKEN_MASTER_PORT = 531,
     QC_TOKEN_MASTER_SERVER_ID = 532,
     QC_TOKEN_MASTER_SSL_CAPATH = 533,
     QC_TOKEN_MASTER_SSL_CA = 534,
     QC_TOKEN_MASTER_SSL_CERT = 535,
     QC_TOKEN_MASTER_SSL_CIPHER = 536,
     QC_TOKEN_MASTER_SSL_KEY = 537,
     QC_TOKEN_MASTER_SSL = 538,
     QC_TOKEN_MASTER_SSL_VERIFY_SERVER_CERT = 539,
     QC_TOKEN_MASTER = 540,
     QC_TOKEN_MASTER_USER = 541,
     QC_TOKEN_MASTER_HEARTBEAT_PERIOD = 542,
     QC_TOKEN_MATCH = 543,
     QC_TOKEN_MAX_CONNECTIONS_PER_HOUR = 544,
     QC_TOKEN_MAX_QUERIES_PER_HOUR = 545,
     QC_TOKEN_MAX_ROWS = 546,
     QC_TOKEN_MAX_SIZE = 547,
     QC_TOKEN_MAX = 548,
     QC_TOKEN_MAX_UPDATES_PER_HOUR = 549,
     QC_TOKEN_MAX_USER_CONNECTIONS = 550,
     QC_TOKEN_MAX_VALUE = 551,
     QC_TOKEN_MEDIUMBLOB = 552,
     QC_TOKEN_MEDIUMINT = 553,
     QC_TOKEN_MEDIUMTEXT = 554,
     QC_TOKEN_MEDIUM = 555,
     QC_TOKEN_MEMORY = 556,
     QC_TOKEN_MERGE = 557,
     QC_TOKEN_MESSAGE_TEXT = 558,
     QC_TOKEN_MICROSECOND = 559,
     QC_TOKEN_MIGRATE = 560,
     QC_TOKEN_MINUTE_MICROSECOND = 561,
     QC_TOKEN_MINUTE_SECOND = 562,
     QC_TOKEN_MINUTE = 563,
     QC_TOKEN_MIN_ROWS = 564,
     QC_TOKEN_MIN = 565,
     QC_TOKEN_MODE = 566,
     QC_TOKEN_MODIFIES = 567,
     QC_TOKEN_MODIFY = 568,
     QC_TOKEN_MOD = 569,
     QC_TOKEN_MONTH = 570,
     QC_TOKEN_MULTILINESTRING = 571,
     QC_TOKEN_MULTIPOINT = 572,
     QC_TOKEN_MULTIPOLYGON = 573,
     QC_TOKEN_MUTEX = 574,
     QC_TOKEN_MYSQL_ERRNO = 575,
     QC_TOKEN_NAMES = 576,
     QC_TOKEN_NAME = 577,
     QC_TOKEN_NATIONAL = 578,
     QC_TOKEN_NATURAL = 579,
     QC_TOKEN_NCHAR_STRING = 580,
     QC_TOKEN_NCHAR = 581,
     QC_TOKEN_NDBCLUSTER = 582,
     QC_TOKEN_NE = 583,
     QC_TOKEN_NE_TRIPLE = 584,
     QC_TOKEN_NEG = 585,
     QC_TOKEN_NEW = 586,
     QC_TOKEN_NEXT = 587,
     QC_TOKEN_NODEGROUP = 588,
     QC_TOKEN_NONE = 589,
     QC_TOKEN_NOT2 = 590,
     QC_TOKEN_NOT = 591,
     QC_TOKEN_NOW = 592,
     QC_TOKEN_NO = 593,
     QC_TOKEN_NO_WAIT = 594,
     QC_TOKEN_NO_WRITE_TO_BINLOG = 595,
     QC_TOKEN_NULL = 596,
     QC_TOKEN_NUM = 597,
     QC_TOKEN_NUMERIC = 598,
     QC_TOKEN_NVARCHAR = 599,
     QC_TOKEN_OFFSET = 600,
     QC_TOKEN_OLD_PASSWORD = 601,
     QC_TOKEN_ON = 602,
     QC_TOKEN_ONE_SHOT = 603,
     QC_TOKEN_ONE = 604,
     QC_TOKEN_OPEN = 605,
     QC_TOKEN_OPTIMIZE = 606,
     QC_TOKEN_OPTIONS = 607,
     QC_TOKEN_OPTION = 608,
     QC_TOKEN_OPTIONALLY = 609,
     QC_TOKEN_OR2 = 610,
     QC_TOKEN_ORDER = 611,
     QC_TOKEN_OR_OR = 612,
     QC_TOKEN_OR = 613,
     QC_TOKEN_OUTER = 614,
     QC_TOKEN_OUTFILE = 615,
     QC_TOKEN_OUT = 616,
     QC_TOKEN_OWNER = 617,
     QC_TOKEN_PACK_KEYS = 618,
     QC_TOKEN_PAGE = 619,
     QC_TOKEN_PARAM_MARKER = 620,
     QC_TOKEN_PARSER = 621,
     QC_TOKEN_PARTIAL = 622,
     QC_TOKEN_PARTITIONING = 623,
     QC_TOKEN_PARTITIONS = 624,
     QC_TOKEN_PARTITION = 625,
     QC_TOKEN_PASSWORD = 626,
     QC_TOKEN_PHASE = 627,
     QC_TOKEN_PLUGINS = 628,
     QC_TOKEN_PLUGIN = 629,
     QC_TOKEN_POINT = 630,
     QC_TOKEN_POLYGON = 631,
     QC_TOKEN_PORT = 632,
     QC_TOKEN_POSITION = 633,
     QC_TOKEN_PRECISION = 634,
     QC_TOKEN_PREPARE = 635,
     QC_TOKEN_PRESERVE = 636,
     QC_TOKEN_PREV = 637,
     QC_TOKEN_PRIMARY = 638,
     QC_TOKEN_PRIVILEGES = 639,
     QC_TOKEN_PROCEDURE = 640,
     QC_TOKEN_PROCESS = 641,
     QC_TOKEN_PROCESSLIST = 642,
     QC_TOKEN_PROFILE = 643,
     QC_TOKEN_PROFILES = 644,
     QC_TOKEN_PURGE = 645,
     QC_TOKEN_QUARTER = 646,
     QC_TOKEN_QUERY = 647,
     QC_TOKEN_QUICK = 648,
     QC_TOKEN_RANGE = 649,
     QC_TOKEN_READS = 650,
     QC_TOKEN_READ_ONLY = 651,
     QC_TOKEN_READ = 652,
     QC_TOKEN_READ_WRITE = 653,
     QC_TOKEN_REAL = 654,
     QC_TOKEN_REBUILD = 655,
     QC_TOKEN_RECOVER = 656,
     QC_TOKEN_REDOFILE = 657,
     QC_TOKEN_REDO_BUFFER_SIZE = 658,
     QC_TOKEN_REDUNDANT = 659,
     QC_TOKEN_REFERENCES = 660,
     QC_TOKEN_REGEXP = 661,
     QC_TOKEN_RELAYLOG = 662,
     QC_TOKEN_RELAY_LOG_FILE = 663,
     QC_TOKEN_RELAY_LOG_POS = 664,
     QC_TOKEN_RELAY_THREAD = 665,
     QC_TOKEN_RELEASE = 666,
     QC_TOKEN_RELOAD = 667,
     QC_TOKEN_REMOVE = 668,
     QC_TOKEN_RENAME = 669,
     QC_TOKEN_REORGANIZE = 670,
     QC_TOKEN_REPAIR = 671,
     QC_TOKEN_REPEATABLE = 672,
     QC_TOKEN_REPEAT = 673,
     QC_TOKEN_REPLACE = 674,
     QC_TOKEN_REPLICATION = 675,
     QC_TOKEN_REQUIRE = 676,
     QC_TOKEN_RESET = 677,
     QC_TOKEN_RESIGNAL = 678,
     QC_TOKEN_RESOURCES = 679,
     QC_TOKEN_RESTORE = 680,
     QC_TOKEN_RESTRICT = 681,
     QC_TOKEN_RESUME = 682,
     QC_TOKEN_RETURNS = 683,
     QC_TOKEN_RETURN = 684,
     QC_TOKEN_REVOKE = 685,
     QC_TOKEN_RIGHT = 686,
     QC_TOKEN_ROLLBACK = 687,
     QC_TOKEN_ROLLUP = 688,
     QC_TOKEN_ROUTINE = 689,
     QC_TOKEN_ROWS = 690,
     QC_TOKEN_ROW_FORMAT = 691,
     QC_TOKEN_ROW = 692,
     QC_TOKEN_RTREE = 693,
     QC_TOKEN_SAVEPOINT = 694,
     QC_TOKEN_SCHEDULE = 695,
     QC_TOKEN_SCHEMA_NAME = 696,
     QC_TOKEN_SECOND_MICROSECOND = 697,
     QC_TOKEN_SECOND = 698,
     QC_TOKEN_SECURITY = 699,
     QC_TOKEN_SELECT = 700,
     QC_TOKEN_SENSITIVE = 701,
     QC_TOKEN_SEPARATOR = 702,
     QC_TOKEN_SERIALIZABLE = 703,
     QC_TOKEN_SERIAL = 704,
     QC_TOKEN_SESSION = 705,
     QC_TOKEN_SERVER = 706,
     QC_TOKEN_SERVER_OPTIONS = 707,
     QC_TOKEN_SET = 708,
     QC_TOKEN_SET_VAR = 709,
     QC_TOKEN_SHARE = 710,
     QC_TOKEN_SHIFT_LEFT = 711,
     QC_TOKEN_SHIFT_RIGHT = 712,
     QC_TOKEN_SHOW = 713,
     QC_TOKEN_SHUTDOWN = 714,
     QC_TOKEN_SIGNAL = 715,
     QC_TOKEN_SIGNED = 716,
     QC_TOKEN_SIMPLE = 717,
     QC_TOKEN_SLAVE = 718,
     QC_TOKEN_SMALLINT = 719,
     QC_TOKEN_SNAPSHOT = 720,
     QC_TOKEN_SOCKET = 721,
     QC_TOKEN_SONAME = 722,
     QC_TOKEN_SOUNDS = 723,
     QC_TOKEN_SOURCE = 724,
     QC_TOKEN_SPATIAL = 725,
     QC_TOKEN_SPECIFIC = 726,
     QC_TOKEN_SQLEXCEPTION = 727,
     QC_TOKEN_SQLSTATE = 728,
     QC_TOKEN_SQLWARNING = 729,
     QC_TOKEN_SQL_BIG_RESULT = 730,
     QC_TOKEN_SQL_BUFFER_RESULT = 731,
     QC_TOKEN_SQL_CACHE = 732,
     QC_TOKEN_SQL_CALC_FOUND_ROWS = 733,
     QC_TOKEN_SQL_NO_CACHE = 734,
     QC_TOKEN_SQL_SMALL_RESULT = 735,
     QC_TOKEN_SQL = 736,
     QC_TOKEN_SQL_THREAD = 737,
     QC_TOKEN_SSL = 738,
     QC_TOKEN_STARTING = 739,
     QC_TOKEN_STARTS = 740,
     QC_TOKEN_START = 741,
     QC_TOKEN_STATUS = 742,
     QC_TOKEN_STDDEV_SAMP = 743,
     QC_TOKEN_STD = 744,
     QC_TOKEN_STOP = 745,
     QC_TOKEN_STORAGE = 746,
     QC_TOKEN_STRAIGHT_JOIN = 747,
     QC_TOKEN_STRING = 748,
     QC_TOKEN_SUBCLASS_ORIGIN = 749,
     QC_TOKEN_SUBDATE = 750,
     QC_TOKEN_SUBJECT = 751,
     QC_TOKEN_SUBPARTITIONS = 752,
     QC_TOKEN_SUBPARTITION = 753,
     QC_TOKEN_SUBSTRING = 754,
     QC_TOKEN_SUM = 755,
     QC_TOKEN_SUPER = 756,
     QC_TOKEN_SUSPEND = 757,
     QC_TOKEN_SWAPS = 758,
     QC_TOKEN_SWITCHES = 759,
     QC_TOKEN_SYSDATE = 760,
     QC_TOKEN_TABLES = 761,
     QC_TOKEN_TABLESPACE = 762,
     QC_TOKEN_TABLE_REF_PRIORITY = 763,
     QC_TOKEN_TABLE = 764,
     QC_TOKEN_TABLE_CHECKSUM = 765,
     QC_TOKEN_TABLE_NAME = 766,
     QC_TOKEN_TEMPORARY = 767,
     QC_TOKEN_TEMPTABLE = 768,
     QC_TOKEN_TERMINATED = 769,
     QC_TOKEN_TEXT_STRING = 770,
     QC_TOKEN_TEXT = 771,
     QC_TOKEN_THAN = 772,
     QC_TOKEN_THEN = 773,
     QC_TOKEN_TIMESTAMP = 774,
     QC_TOKEN_TIMESTAMP_ADD = 775,
     QC_TOKEN_TIMESTAMP_DIFF = 776,
     QC_TOKEN_TIME = 777,
     QC_TOKEN_TINYBLOB = 778,
     QC_TOKEN_TINYINT = 779,
     QC_TOKEN_TINYTEXT = 780,
     QC_TOKEN_TO = 781,
     QC_TOKEN_TRAILING = 782,
     QC_TOKEN_TRANSACTION = 783,
     QC_TOKEN_TRIGGERS = 784,
     QC_TOKEN_TRIGGER = 785,
     QC_TOKEN_TRIM = 786,
     QC_TOKEN_TRUE = 787,
     QC_TOKEN_TRUNCATE = 788,
     QC_TOKEN_TYPES = 789,
     QC_TOKEN_TYPE = 790,
     QC_TOKEN_UDF_RETURNS = 791,
     QC_TOKEN_ULONGLONG_NUM = 792,
     QC_TOKEN_UNCOMMITTED = 793,
     QC_TOKEN_UNDEFINED = 794,
     QC_TOKEN_UNDERSCORE_CHARSET = 795,
     QC_TOKEN_UNDOFILE = 796,
     QC_TOKEN_UNDO_BUFFER_SIZE = 797,
     QC_TOKEN_UNDO = 798,
     QC_TOKEN_UNICODE = 799,
     QC_TOKEN_UNINSTALL = 800,
     QC_TOKEN_UNION = 801,
     QC_TOKEN_UNIQUE = 802,
     QC_TOKEN_UNKNOWN = 803,
     QC_TOKEN_UNLOCK = 804,
     QC_TOKEN_UNSIGNED = 805,
     QC_TOKEN_UNTIL = 806,
     QC_TOKEN_UPDATE = 807,
     QC_TOKEN_UPGRADE = 808,
     QC_TOKEN_USAGE = 809,
     QC_TOKEN_USER = 810,
     QC_TOKEN_USE_FRM = 811,
     QC_TOKEN_USE = 812,
     QC_TOKEN_USING = 813,
     QC_TOKEN_UTC_DATE = 814,
     QC_TOKEN_UTC_TIMESTAMP = 815,
     QC_TOKEN_UTC_TIME = 816,
     QC_TOKEN_VALUES = 817,
     QC_TOKEN_VALUE = 818,
     QC_TOKEN_VARBINARY = 819,
     QC_TOKEN_VARCHAR = 820,
     QC_TOKEN_VARIABLES = 821,
     QC_TOKEN_VARIANCE = 822,
     QC_TOKEN_VARYING = 823,
     QC_TOKEN_VAR_SAMP = 824,
     QC_TOKEN_VIEW = 825,
     QC_TOKEN_WAIT = 826,
     QC_TOKEN_WARNINGS = 827,
     QC_TOKEN_WEEK = 828,
     QC_TOKEN_WHEN = 829,
     QC_TOKEN_WHERE = 830,
     QC_TOKEN_WHILE = 831,
     QC_TOKEN_WITH = 832,
     QC_TOKEN_WITH_CUBE = 833,
     QC_TOKEN_WITH_ROLLUP = 834,
     QC_TOKEN_WORK = 835,
     QC_TOKEN_WRAPPER = 836,
     QC_TOKEN_WRITE = 837,
     QC_TOKEN_X509 = 838,
     QC_TOKEN_XA = 839,
     QC_TOKEN_XML = 840,
     QC_TOKEN_XOR = 841,
     QC_TOKEN_YEAR_MONTH = 842,
     QC_TOKEN_YEAR = 843,
     QC_TOKEN_ZEROFILL = 844,
     QC_TOKEN_CLIENT_FLAG = 845,
     QC_TOKEN_GLOBAL_VAR = 846,
     QC_TOKEN_SESSION_VAR = 847,
     QC_TOKEN_BRACKET_OPEN = 848,
     QC_TOKEN_BRACKET_CLOSE = 849,
     QC_TOKEN_PLUS = 850,
     QC_TOKEN_MINUS = 851,
     QC_TOKEN_STAR = 852,
     QC_TOKEN_COMMA = 853,
     QC_TOKEN_DOT = 854,
     QC_TOKEN_SEMICOLON = 855,
     QC_TOKEN_NO_MORE = 856,
     QC_TOKEN_IDENTIFIER = 857,
     QC_TOKEN_INTNUM = 858,
     QC_TOKEN_FLOATNUM = 859,
     QC_TOKEN_ASSIGN_TO_VAR = 860,
     QC_TOKEN_TILDE = 861
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 40 "mysqlnd_query_parser.grammar"

  zval zv;
  const char * kn; /* keyword_name */
  smart_str * comment;



/* Line 214 of yacc.c  */
#line 772 "mysqlnd_query_parser.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */

/* Line 264 of yacc.c  */
#line 46 "mysqlnd_query_parser.grammar"

/* so we can override the default declaration */
#define YY_DECL 
#include "mysqlnd_query_lexer.lex.h"
extern int mysqlnd_qp_lex(YYSTYPE * yylval_param, yyscan_t yyscanner TSRMLS_DC);


/* Line 264 of yacc.c  */
#line 792 "mysqlnd_query_parser.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  13
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2951

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  607
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  41
/* YYNRULES -- Number of rules.  */
#define YYNRULES  383
/* YYNRULES -- Number of states.  */
#define YYNSTATES  424

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   861

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint16 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   251,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,   430,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     455,   456,   457,   458,   459,   460,   461,   462,   463,   464,
     465,   466,   467,   468,   469,   470,   471,   472,   473,   474,
     475,   476,   477,   478,   479,   480,   481,   482,   483,   484,
     485,   486,   487,   488,   489,   490,   491,   492,   493,   494,
     495,   496,   497,   498,   499,   500,   501,   502,   503,   504,
     505,   506,   507,   508,   509,   510,   511,   512,   513,   514,
     515,   516,   517,   518,   519,   520,   521,   522,   523,   524,
     525,   526,   527,   528,   529,   530,   531,   532,   533,   534,
     535,   536,   537,   538,   539,   540,   541,   542,   543,   544,
     545,   546,   547,   548,   549,   550,   551,   552,   553,   554,
     555,   556,   557,   558,   559,   560,   561,   562,   563,   564,
     565,   566,   567,   568,   569,   570,   571,   572,   573,   574,
     575,   576,   577,   578,   579,   580,   581,   582,   583,   584,
     585,   586,   587,   588,   589,   590,   591,   592,   593,   594,
     595,   596,   597,   598,   599,   600,   601,   602,   603,   604,
     605,   606
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    15,    17,
      19,    21,    29,    31,    32,    35,    36,    38,    40,    41,
      47,    52,    54,    56,    61,    63,    65,    70,    72,    73,
      79,    82,    83,    85,    87,    89,    96,    98,    99,   105,
     107,   108,   109,   110,   111,   120,   122,   123,   126,   128,
     132,   134,   138,   140,   142,   144,   146,   150,   156,   158,
     160,   163,   165,   166,   169,   171,   173,   177,   180,   182,
     186,   191,   196,   199,   203,   205,   207,   209,   211,   213,
     215,   217,   219,   221,   223,   225,   227,   229,   231,   233,
     235,   237,   239,   241,   243,   245,   247,   249,   251,   253,
     255,   257,   259,   261,   263,   265,   267,   269,   271,   273,
     275,   277,   279,   281,   283,   285,   287,   289,   291,   293,
     295,   297,   299,   301,   303,   305,   307,   309,   311,   313,
     315,   317,   319,   321,   323,   325,   327,   329,   331,   333,
     335,   337,   339,   341,   343,   345,   347,   349,   351,   353,
     355,   357,   359,   361,   363,   365,   367,   369,   371,   373,
     375,   377,   379,   381,   383,   385,   387,   389,   391,   393,
     395,   397,   399,   401,   403,   405,   407,   409,   411,   413,
     415,   417,   419,   421,   423,   425,   427,   429,   431,   433,
     435,   437,   439,   441,   443,   445,   447,   449,   451,   453,
     455,   457,   459,   461,   463,   465,   467,   469,   471,   473,
     475,   477,   479,   481,   483,   485,   487,   489,   491,   493,
     495,   497,   499,   501,   503,   505,   507,   509,   511,   513,
     515,   517,   519,   521,   523,   525,   527,   529,   531,   533,
     535,   537,   539,   541,   543,   545,   547,   549,   551,   553,
     555,   557,   559,   561,   563,   565,   567,   569,   571,   573,
     575,   577,   579,   581,   583,   585,   587,   589,   591,   593,
     595,   597,   599,   601,   603,   605,   607,   609,   611,   613,
     615,   617,   619,   621,   623,   625,   627,   629,   631,   633,
     635,   637,   639,   641,   643,   645,   647,   649,   651,   653,
     655,   657,   659,   661,   663,   665,   667,   669,   671,   673,
     675,   677,   679,   681,   683,   685,   687,   689,   691,   693,
     695,   697,   699,   701,   703,   705,   707,   709,   711,   713,
     715,   717,   719,   721,   723,   725,   727,   729,   731,   733,
     735,   737,   739,   741,   743,   745,   747,   749,   751,   753,
     755,   757,   759,   761,   763,   765,   767,   769,   771,   773,
     775,   777,   779,   781,   783,   785,   787,   789,   791,   793,
     795,   797,   799,   801,   803,   805,   807,   809,   811,   813,
     815,   817,   819,   821
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     608,     0,    -1,   627,    -1,   625,    -1,   623,    -1,   620,
      -1,   618,    -1,   616,    -1,   614,    -1,   613,    -1,   609,
      -1,   631,   136,   610,   615,   611,   640,   612,    -1,   512,
      -1,    -1,   211,   161,    -1,    -1,   426,    -1,    50,    -1,
      -1,   631,    12,   626,   509,   640,    -1,   631,   414,   615,
     640,    -1,   509,    -1,   506,    -1,   631,   419,   617,   640,
      -1,   624,    -1,   121,    -1,   631,   533,   619,   640,    -1,
     509,    -1,    -1,   631,   123,   621,   182,   640,    -1,   622,
     621,    -1,    -1,   393,    -1,   269,    -1,   212,    -1,   631,
     552,   624,   626,   640,   453,    -1,   269,    -1,    -1,   631,
     222,   226,   626,   640,    -1,   212,    -1,    -1,    -1,    -1,
      -1,   631,   445,   628,   632,   629,   641,   630,   643,    -1,
      73,    -1,    -1,   631,   597,    -1,   633,    -1,   634,   598,
     633,    -1,   634,    -1,   631,   635,   638,    -1,   636,    -1,
     603,    -1,   493,    -1,   637,    -1,   637,   599,   637,    -1,
     637,   599,   637,   599,   637,    -1,   602,    -1,   646,    -1,
      18,   637,    -1,   637,    -1,    -1,   640,   638,    -1,   137,
      -1,   637,    -1,   637,   599,   637,    -1,   182,   642,    -1,
     639,    -1,   642,   598,   639,    -1,   642,   247,   236,   639,
      -1,   642,   431,   236,   639,    -1,   575,   644,    -1,   636,
     152,   645,    -1,   603,    -1,   493,    -1,   647,    -1,    20,
      -1,    28,    -1,    30,    -1,    47,    -1,    48,    -1,    58,
      -1,    60,    -1,    65,    -1,    75,    -1,    87,    -1,   115,
      -1,   135,    -1,   146,    -1,   160,    -1,   176,    -1,   196,
      -1,   199,    -1,   202,    -1,   224,    -1,   241,    -1,   338,
      -1,   350,    -1,   352,    -1,   362,    -1,   366,    -1,   370,
      -1,   377,    -1,   380,    -1,   413,    -1,   416,    -1,   422,
      -1,   425,    -1,   432,    -1,   439,    -1,   444,    -1,   451,
      -1,   461,    -1,   466,    -1,   463,    -1,   467,    -1,   486,
      -1,   490,    -1,   533,    -1,   544,    -1,   545,    -1,   581,
      -1,   584,    -1,   553,    -1,     4,    -1,     6,    -1,     7,
      -1,     8,    -1,     9,    -1,    10,    -1,    17,    -1,    22,
      -1,    23,    -1,    25,    -1,    24,    -1,    26,    -1,    27,
      -1,    34,    -1,    38,    -1,    41,    -1,    43,    -1,    42,
      -1,    45,    -1,    51,    -1,    55,    -1,    57,    -1,    62,
      -1,    64,    -1,    66,    -1,    67,    -1,    69,    -1,    70,
      -1,    74,    -1,    76,    -1,    77,    -1,    78,    -1,    79,
      -1,    81,    -1,    82,    -1,    88,    -1,    90,    -1,    93,
      -1,    96,    -1,   105,    -1,   104,    -1,   106,    -1,   109,
      -1,   114,    -1,   120,    -1,   122,    -1,   126,    -1,   128,
      -1,   129,    -1,   130,    -1,   131,    -1,   138,    -1,   139,
      -1,   140,    -1,   147,    -1,   151,    -1,   150,    -1,   149,
      -1,   154,    -1,   156,    -1,   158,    -1,   157,    -1,   159,
      -1,   163,    -1,   164,    -1,   165,    -1,   169,    -1,   168,
      -1,   180,    -1,   144,    -1,   183,    -1,   171,    -1,   172,
      -1,   173,    -1,   181,    -1,   188,    -1,   187,    -1,   189,
      -1,   192,    -1,   190,    -1,   197,    -1,   203,    -1,   207,
      -1,   209,    -1,   228,    -1,   214,    -1,   215,    -1,   218,
      -1,   230,    -1,   231,    -1,   233,    -1,   234,    -1,   223,
      -1,   238,    -1,   242,    -1,   245,    -1,   248,    -1,   249,
      -1,   255,    -1,   256,    -1,   258,    -1,   260,    -1,   262,
      -1,   263,    -1,   291,    -1,   285,    -1,   272,    -1,   276,
      -1,   273,    -1,   274,    -1,   286,    -1,   275,    -1,   277,
      -1,   271,    -1,   283,    -1,   279,    -1,   278,    -1,   280,
      -1,   281,    -1,   282,    -1,   289,    -1,   290,    -1,   292,
      -1,   294,    -1,   295,    -1,   296,    -1,   300,    -1,   301,
      -1,   302,    -1,   304,    -1,   305,    -1,   308,    -1,   309,
      -1,   313,    -1,   311,    -1,   315,    -1,   316,    -1,   317,
      -1,   318,    -1,   319,    -1,   322,    -1,   321,    -1,   323,
      -1,   326,    -1,   327,    -1,   332,    -1,   331,    -1,   339,
      -1,   333,    -1,   334,    -1,   344,    -1,   345,    -1,   346,
      -1,   348,    -1,   349,    -1,   363,    -1,   364,    -1,   367,
      -1,   368,    -1,   369,    -1,   371,    -1,   372,    -1,   374,
      -1,   373,    -1,   375,    -1,   376,    -1,   381,    -1,   382,
      -1,   384,    -1,   386,    -1,   387,    -1,   388,    -1,   389,
      -1,   391,    -1,   392,    -1,   393,    -1,   396,    -1,   400,
      -1,   401,    -1,   403,    -1,   402,    -1,   404,    -1,   408,
      -1,   409,    -1,   410,    -1,   412,    -1,   415,    -1,   417,
      -1,   420,    -1,   424,    -1,   427,    -1,   428,    -1,   433,
      -1,   434,    -1,   435,    -1,   436,    -1,   437,    -1,   438,
      -1,   440,    -1,   443,    -1,   449,    -1,   448,    -1,   450,
      -1,   462,    -1,   455,    -1,   459,    -1,   465,    -1,   468,
      -1,   469,    -1,   477,    -1,   476,    -1,   479,    -1,   482,
      -1,   485,    -1,   487,    -1,   491,    -1,   493,    -1,   495,
      -1,   496,    -1,   498,    -1,   497,    -1,   501,    -1,   502,
      -1,   503,    -1,   504,    -1,   506,    -1,   510,    -1,   507,
      -1,   512,    -1,   513,    -1,   516,    -1,   517,    -1,   528,
      -1,   529,    -1,   519,    -1,   520,    -1,   521,    -1,   522,
      -1,   534,    -1,   535,    -1,   536,    -1,   185,    -1,   538,
      -1,   539,    -1,   542,    -1,   541,    -1,   548,    -1,   551,
      -1,   555,    -1,   556,    -1,   566,    -1,   570,    -1,   563,
      -1,   572,    -1,   571,    -1,   573,    -1,   580,    -1,   583,
      -1,   588,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   665,   665,   666,   667,   668,   669,   670,   671,   672,
     673,   676,   678,   679,   682,   683,   686,   687,   688,   693,
     702,   709,   710,   715,   722,   723,   728,   734,   735,   740,
     747,   748,   751,   752,   753,   758,   765,   766,   772,   779,
     780,   785,   791,   795,   784,   808,   809,   812,   813,   816,
     817,   820,   822,   823,   824,   826,   838,   852,   870,   871,
     875,   876,   877,   880,   898,   908,   918,   935,   938,   939,
     940,   941,   944,   948,   960,   961,   964,   965,   966,   967,
     968,   969,   970,   971,   972,   973,   974,   975,   976,   977,
     978,   979,   980,   981,   981,   982,   983,   984,   985,   986,
     987,   988,   989,   990,   991,   992,   993,   994,   995,   996,
     997,   998,   999,  1000,  1001,  1002,  1003,  1004,  1005,  1006,
    1007,  1008,  1009,  1010,  1011,  1014,  1015,  1016,  1017,  1018,
    1019,  1020,  1021,  1022,  1023,  1024,  1025,  1026,  1027,  1028,
    1029,  1030,  1031,  1032,  1033,  1034,  1035,  1036,  1037,  1038,
    1039,  1040,  1041,  1042,  1043,  1044,  1045,  1046,  1047,  1048,
    1049,  1050,  1051,  1052,  1053,  1054,  1055,  1056,  1057,  1058,
    1059,  1060,  1061,  1062,  1063,  1064,  1065,  1066,  1067,  1068,
    1069,  1070,  1071,  1072,  1073,  1074,  1075,  1076,  1077,  1078,
    1079,  1080,  1081,  1082,  1083,  1084,  1085,  1086,  1087,  1088,
    1089,  1090,  1091,  1092,  1093,  1094,  1095,  1096,  1097,  1098,
    1099,  1100,  1101,  1102,  1103,  1104,  1105,  1106,  1107,  1108,
    1109,  1110,  1111,  1112,  1113,  1114,  1115,  1116,  1117,  1118,
    1119,  1120,  1121,  1122,  1123,  1124,  1125,  1126,  1127,  1128,
    1129,  1130,  1131,  1132,  1133,  1134,  1135,  1136,  1137,  1138,
    1139,  1140,  1141,  1142,  1143,  1144,  1145,  1146,  1147,  1148,
    1149,  1150,  1151,  1152,  1153,  1154,  1155,  1156,  1157,  1158,
    1159,  1160,  1161,  1162,  1163,  1164,  1165,  1166,  1167,  1168,
    1169,  1170,  1171,  1172,  1173,  1174,  1175,  1176,  1177,  1178,
    1179,  1180,  1181,  1182,  1183,  1184,  1185,  1186,  1187,  1188,
    1189,  1190,  1191,  1192,  1193,  1194,  1195,  1196,  1197,  1198,
    1199,  1200,  1201,  1202,  1203,  1204,  1205,  1206,  1207,  1208,
    1209,  1210,  1211,  1212,  1213,  1214,  1215,  1216,  1217,  1218,
    1219,  1220,  1221,  1222,  1223,  1224,  1225,  1226,  1227,  1228,
    1229,  1230,  1231,  1232,  1233,  1234,  1235,  1236,  1237,  1238,
    1239,  1240,  1241,  1242,  1243,  1244,  1245,  1246,  1247,  1248,
    1249,  1250,  1251,  1252,  1253,  1254,  1255,  1256,  1257,  1258,
    1259,  1260,  1261,  1262,  1263,  1264,  1265,  1266,  1267,  1268,
    1269,  1270,  1271,  1272
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "QC_TOKEN_ACCESSIBLE", "QC_TOKEN_ACTION",
  "QC_TOKEN_ADD", "QC_TOKEN_ADDDATE", "QC_TOKEN_AFTER", "QC_TOKEN_AGAINST",
  "QC_TOKEN_AGGREGATE", "QC_TOKEN_ALGORITHM", "QC_TOKEN_ALL",
  "QC_TOKEN_ALTER", "QC_TOKEN_ANALYZE", "QC_TOKEN_AND_AND", "QC_TOKEN_AND",
  "QC_TOKEN_BETWEEN_AND", "QC_TOKEN_ANY", "QC_TOKEN_AS", "QC_TOKEN_ASC",
  "QC_TOKEN_ASCII", "QC_TOKEN_ASENSITIVE", "QC_TOKEN_AT",
  "QC_TOKEN_AUTHORS", "QC_TOKEN_AUTOEXTEND_SIZE", "QC_TOKEN_AUTO_INC",
  "QC_TOKEN_AVG_ROW_LENGTH", "QC_TOKEN_AVG", "QC_TOKEN_BACKUP",
  "QC_TOKEN_BEFORE", "QC_TOKEN_BEGIN", "QC_TOKEN_BETWEEN",
  "QC_TOKEN_BIGINT", "QC_TOKEN_BINARY", "QC_TOKEN_BINLOG",
  "QC_TOKEN_BIN_NUM", "QC_TOKEN_BIT_AND", "QC_TOKEN_BIT_OR",
  "QC_TOKEN_BIT", "QC_TOKEN_BIT_XOR", "QC_TOKEN_BLOB", "QC_TOKEN_BLOCK",
  "QC_TOKEN_BOOLEAN", "QC_TOKEN_BOOL", "QC_TOKEN_BOTH", "QC_TOKEN_BTREE",
  "QC_TOKEN_BY", "QC_TOKEN_BYTE", "QC_TOKEN_CACHE", "QC_TOKEN_CALL",
  "QC_TOKEN_CASCADE", "QC_TOKEN_CASCADED", "QC_TOKEN_CASE",
  "QC_TOKEN_CAST", "QC_TOKEN_CATALOG_NAME", "QC_TOKEN_CHAIN",
  "QC_TOKEN_CHANGE", "QC_TOKEN_CHANGED", "QC_TOKEN_CHARSET",
  "QC_TOKEN_CHAR", "QC_TOKEN_CHECKSUM", "QC_TOKEN_CHECK",
  "QC_TOKEN_CIPHER", "QC_TOKEN_CLASS_ORIGIN", "QC_TOKEN_CLIENT",
  "QC_TOKEN_CLOSE", "QC_TOKEN_COALESCE", "QC_TOKEN_CODE",
  "QC_TOKEN_COLLATE", "QC_TOKEN_COLLATION", "QC_TOKEN_COLUMNS",
  "QC_TOKEN_COLUMN", "QC_TOKEN_COLUMN_NAME", "QC_TOKEN_COMMENT",
  "QC_TOKEN_COMMITTED", "QC_TOKEN_COMMIT", "QC_TOKEN_COMPACT",
  "QC_TOKEN_COMPLETION", "QC_TOKEN_COMPRESSED", "QC_TOKEN_CONCURRENT",
  "QC_TOKEN_CONDITION", "QC_TOKEN_CONNECTION", "QC_TOKEN_CONSISTENT",
  "QC_TOKEN_CONSTRAINT", "QC_TOKEN_CONSTRAINT_CATALOG",
  "QC_TOKEN_CONSTRAINT_NAME", "QC_TOKEN_CONSTRAINT_SCHEMA",
  "QC_TOKEN_CONTAINS", "QC_TOKEN_CONTEXT", "QC_TOKEN_CONTINUE",
  "QC_TOKEN_CONTRIBUTORS", "QC_TOKEN_CONVERT", "QC_TOKEN_COUNT",
  "QC_TOKEN_CPU", "QC_TOKEN_CREATE", "QC_TOKEN_CROSS", "QC_TOKEN_CUBE",
  "QC_TOKEN_CURDATE", "QC_TOKEN_CURRENT_USER", "QC_TOKEN_CURSOR",
  "QC_TOKEN_CURSOR_NAME", "QC_TOKEN_CURTIME", "QC_TOKEN_DATABASE",
  "QC_TOKEN_DATABASES", "QC_TOKEN_DATAFILE", "QC_TOKEN_DATA",
  "QC_TOKEN_DATETIME", "QC_TOKEN_DATE_ADD_INTERVAL",
  "QC_TOKEN_DATE_SUB_INTERVAL", "QC_TOKEN_DATE", "QC_TOKEN_DAY_HOUR",
  "QC_TOKEN_DAY_MICROSECOND", "QC_TOKEN_DAY_MINUTE", "QC_TOKEN_DAY_SECOND",
  "QC_TOKEN_DAY", "QC_TOKEN_DEALLOCATE", "QC_TOKEN_DECIMAL_NUM",
  "QC_TOKEN_DECIMAL", "QC_TOKEN_DECLARE", "QC_TOKEN_DEFAULT",
  "QC_TOKEN_DEFINER", "QC_TOKEN_DELAYED", "QC_TOKEN_DELAY_KEY_WRITE",
  "QC_TOKEN_DELETE", "QC_TOKEN_DESC", "QC_TOKEN_DESCRIBE",
  "QC_TOKEN_DES_KEY_FILE", "QC_TOKEN_DETERMINISTIC", "QC_TOKEN_DIRECTORY",
  "QC_TOKEN_DISABLE", "QC_TOKEN_DISCARD", "QC_TOKEN_DISK",
  "QC_TOKEN_DISTINCT", "QC_TOKEN_DIV", "QC_TOKEN_DOUBLE", "QC_TOKEN_DO",
  "QC_TOKEN_DROP", "QC_TOKEN_DUAL", "QC_TOKEN_DUMPFILE",
  "QC_TOKEN_DUPLICATE", "QC_TOKEN_DYNAMIC", "QC_TOKEN_EACH",
  "QC_TOKEN_ELSE", "QC_TOKEN_ELSEIF", "QC_TOKEN_ENABLE",
  "QC_TOKEN_ENCLOSED", "QC_TOKEN_END", "QC_TOKEN_ENDS",
  "QC_TOKEN_END_OF_INPUT", "QC_TOKEN_ENGINES", "QC_TOKEN_ENGINE",
  "QC_TOKEN_ENUM", "QC_TOKEN_EQ", "QC_TOKEN_EQUAL", "QC_TOKEN_ERRORS",
  "QC_TOKEN_ESCAPED", "QC_TOKEN_ESCAPE", "QC_TOKEN_EVENTS",
  "QC_TOKEN_EVENT", "QC_TOKEN_EVERY", "QC_TOKEN_EXECUTE",
  "QC_TOKEN_EXISTS", "QC_TOKEN_EXIT", "QC_TOKEN_EXPANSION",
  "QC_TOKEN_EXTENDED", "QC_TOKEN_EXTENT_SIZE", "QC_TOKEN_EXTRACT",
  "QC_TOKEN_FALSE", "QC_TOKEN_FAST", "QC_TOKEN_FAULTS", "QC_TOKEN_FETCH",
  "QC_TOKEN_FILE", "QC_TOKEN_FIRST", "QC_TOKEN_FIXED",
  "QC_TOKEN_FLOAT_NUM", "QC_TOKEN_FLOAT", "QC_TOKEN_FLUSH",
  "QC_TOKEN_FORCE", "QC_TOKEN_FOREIGN", "QC_TOKEN_FOR", "QC_TOKEN_FOUND",
  "QC_TOKEN_FRAC_SECOND", "QC_TOKEN_FROM", "QC_TOKEN_FULL",
  "QC_TOKEN_FULLTEXT", "QC_TOKEN_FUNCTION", "QC_TOKEN_GE",
  "QC_TOKEN_GEOMETRYCOLLECTION", "QC_TOKEN_GEOMETRY",
  "QC_TOKEN_GET_FORMAT", "QC_TOKEN_GLOBAL", "QC_TOKEN_GRANT",
  "QC_TOKEN_GRANTS", "QC_TOKEN_GROUP", "QC_TOKEN_GROUP_CONCAT",
  "QC_TOKEN_GT", "QC_TOKEN_HANDLER", "QC_TOKEN_HASH", "QC_TOKEN_HAVING",
  "QC_TOKEN_HELP", "QC_TOKEN_HEX_NUM", "QC_TOKEN_HIGH_PRIORITY",
  "QC_TOKEN_HOST", "QC_TOKEN_HOSTS", "QC_TOKEN_HOUR_MICROSECOND",
  "QC_TOKEN_HOUR_MINUTE", "QC_TOKEN_HOUR_SECOND", "QC_TOKEN_HOUR",
  "QC_TOKEN_IDENT", "QC_TOKEN_IDENTIFIED", "QC_TOKEN_IDENT_QUOTED",
  "QC_TOKEN_IF", "QC_TOKEN_IGNORE", "QC_TOKEN_IGNORE_SERVER_IDS",
  "QC_TOKEN_IMPORT", "QC_TOKEN_INDEXES", "QC_TOKEN_INDEX",
  "QC_TOKEN_INFILE", "QC_TOKEN_INITIAL_SIZE", "QC_TOKEN_INNER",
  "QC_TOKEN_INOUT", "QC_TOKEN_INSENSITIVE", "QC_TOKEN_INSERT",
  "QC_TOKEN_INSERT_METHOD", "QC_TOKEN_INSTALL", "QC_TOKEN_INTERVAL",
  "QC_TOKEN_INTO", "QC_TOKEN_INT", "QC_TOKEN_INVOKER", "QC_TOKEN_IN",
  "QC_TOKEN_IO", "QC_TOKEN_IPC", "QC_TOKEN_IS", "QC_TOKEN_ISOLATION",
  "QC_TOKEN_ISSUER", "QC_TOKEN_ITERATE", "QC_TOKEN_JOIN", "QC_TOKEN_KEYS",
  "QC_TOKEN_KEY_BLOCK_SIZE", "QC_TOKEN_KEY", "QC_TOKEN_KILL",
  "QC_TOKEN_LANGUAGE", "QC_TOKEN_LAST", "QC_TOKEN_LE", "QC_TOKEN_LEADING",
  "QC_TOKEN_LEAVES", "QC_TOKEN_LEAVE", "QC_TOKEN_LEFT", "QC_TOKEN_LESS",
  "QC_TOKEN_LEVEL", "QC_TOKEN_LEX_HOSTNAME", "QC_TOKEN_LIKE",
  "QC_TOKEN_LIMIT", "QC_TOKEN_LINEAR", "QC_TOKEN_LINES",
  "QC_TOKEN_LINESTRING", "QC_TOKEN_LIST", "QC_TOKEN_LOAD",
  "QC_TOKEN_LOCAL", "QC_TOKEN_LOCATOR", "QC_TOKEN_LOCKS", "QC_TOKEN_LOCK",
  "QC_TOKEN_LOGFILE", "QC_TOKEN_LOGS", "QC_TOKEN_LONGBLOB",
  "QC_TOKEN_LONGTEXT", "QC_TOKEN_LONG_NUM", "QC_TOKEN_LONG",
  "QC_TOKEN_LOOP", "QC_TOKEN_LOW_PRIORITY", "QC_TOKEN_LT",
  "QC_TOKEN_MASTER_CONNECT_RETRY", "QC_TOKEN_MASTER_HOST",
  "QC_TOKEN_MASTER_LOG_FILE", "QC_TOKEN_MASTER_LOG_POS",
  "QC_TOKEN_MASTER_PASSWORD", "QC_TOKEN_MASTER_PORT",
  "QC_TOKEN_MASTER_SERVER_ID", "QC_TOKEN_MASTER_SSL_CAPATH",
  "QC_TOKEN_MASTER_SSL_CA", "QC_TOKEN_MASTER_SSL_CERT",
  "QC_TOKEN_MASTER_SSL_CIPHER", "QC_TOKEN_MASTER_SSL_KEY",
  "QC_TOKEN_MASTER_SSL", "QC_TOKEN_MASTER_SSL_VERIFY_SERVER_CERT",
  "QC_TOKEN_MASTER", "QC_TOKEN_MASTER_USER",
  "QC_TOKEN_MASTER_HEARTBEAT_PERIOD", "QC_TOKEN_MATCH",
  "QC_TOKEN_MAX_CONNECTIONS_PER_HOUR", "QC_TOKEN_MAX_QUERIES_PER_HOUR",
  "QC_TOKEN_MAX_ROWS", "QC_TOKEN_MAX_SIZE", "QC_TOKEN_MAX",
  "QC_TOKEN_MAX_UPDATES_PER_HOUR", "QC_TOKEN_MAX_USER_CONNECTIONS",
  "QC_TOKEN_MAX_VALUE", "QC_TOKEN_MEDIUMBLOB", "QC_TOKEN_MEDIUMINT",
  "QC_TOKEN_MEDIUMTEXT", "QC_TOKEN_MEDIUM", "QC_TOKEN_MEMORY",
  "QC_TOKEN_MERGE", "QC_TOKEN_MESSAGE_TEXT", "QC_TOKEN_MICROSECOND",
  "QC_TOKEN_MIGRATE", "QC_TOKEN_MINUTE_MICROSECOND",
  "QC_TOKEN_MINUTE_SECOND", "QC_TOKEN_MINUTE", "QC_TOKEN_MIN_ROWS",
  "QC_TOKEN_MIN", "QC_TOKEN_MODE", "QC_TOKEN_MODIFIES", "QC_TOKEN_MODIFY",
  "QC_TOKEN_MOD", "QC_TOKEN_MONTH", "QC_TOKEN_MULTILINESTRING",
  "QC_TOKEN_MULTIPOINT", "QC_TOKEN_MULTIPOLYGON", "QC_TOKEN_MUTEX",
  "QC_TOKEN_MYSQL_ERRNO", "QC_TOKEN_NAMES", "QC_TOKEN_NAME",
  "QC_TOKEN_NATIONAL", "QC_TOKEN_NATURAL", "QC_TOKEN_NCHAR_STRING",
  "QC_TOKEN_NCHAR", "QC_TOKEN_NDBCLUSTER", "QC_TOKEN_NE",
  "QC_TOKEN_NE_TRIPLE", "QC_TOKEN_NEG", "QC_TOKEN_NEW", "QC_TOKEN_NEXT",
  "QC_TOKEN_NODEGROUP", "QC_TOKEN_NONE", "QC_TOKEN_NOT2", "QC_TOKEN_NOT",
  "QC_TOKEN_NOW", "QC_TOKEN_NO", "QC_TOKEN_NO_WAIT",
  "QC_TOKEN_NO_WRITE_TO_BINLOG", "QC_TOKEN_NULL", "QC_TOKEN_NUM",
  "QC_TOKEN_NUMERIC", "QC_TOKEN_NVARCHAR", "QC_TOKEN_OFFSET",
  "QC_TOKEN_OLD_PASSWORD", "QC_TOKEN_ON", "QC_TOKEN_ONE_SHOT",
  "QC_TOKEN_ONE", "QC_TOKEN_OPEN", "QC_TOKEN_OPTIMIZE", "QC_TOKEN_OPTIONS",
  "QC_TOKEN_OPTION", "QC_TOKEN_OPTIONALLY", "QC_TOKEN_OR2",
  "QC_TOKEN_ORDER", "QC_TOKEN_OR_OR", "QC_TOKEN_OR", "QC_TOKEN_OUTER",
  "QC_TOKEN_OUTFILE", "QC_TOKEN_OUT", "QC_TOKEN_OWNER",
  "QC_TOKEN_PACK_KEYS", "QC_TOKEN_PAGE", "QC_TOKEN_PARAM_MARKER",
  "QC_TOKEN_PARSER", "QC_TOKEN_PARTIAL", "QC_TOKEN_PARTITIONING",
  "QC_TOKEN_PARTITIONS", "QC_TOKEN_PARTITION", "QC_TOKEN_PASSWORD",
  "QC_TOKEN_PHASE", "QC_TOKEN_PLUGINS", "QC_TOKEN_PLUGIN",
  "QC_TOKEN_POINT", "QC_TOKEN_POLYGON", "QC_TOKEN_PORT",
  "QC_TOKEN_POSITION", "QC_TOKEN_PRECISION", "QC_TOKEN_PREPARE",
  "QC_TOKEN_PRESERVE", "QC_TOKEN_PREV", "QC_TOKEN_PRIMARY",
  "QC_TOKEN_PRIVILEGES", "QC_TOKEN_PROCEDURE", "QC_TOKEN_PROCESS",
  "QC_TOKEN_PROCESSLIST", "QC_TOKEN_PROFILE", "QC_TOKEN_PROFILES",
  "QC_TOKEN_PURGE", "QC_TOKEN_QUARTER", "QC_TOKEN_QUERY", "QC_TOKEN_QUICK",
  "QC_TOKEN_RANGE", "QC_TOKEN_READS", "QC_TOKEN_READ_ONLY",
  "QC_TOKEN_READ", "QC_TOKEN_READ_WRITE", "QC_TOKEN_REAL",
  "QC_TOKEN_REBUILD", "QC_TOKEN_RECOVER", "QC_TOKEN_REDOFILE",
  "QC_TOKEN_REDO_BUFFER_SIZE", "QC_TOKEN_REDUNDANT", "QC_TOKEN_REFERENCES",
  "QC_TOKEN_REGEXP", "QC_TOKEN_RELAYLOG", "QC_TOKEN_RELAY_LOG_FILE",
  "QC_TOKEN_RELAY_LOG_POS", "QC_TOKEN_RELAY_THREAD", "QC_TOKEN_RELEASE",
  "QC_TOKEN_RELOAD", "QC_TOKEN_REMOVE", "QC_TOKEN_RENAME",
  "QC_TOKEN_REORGANIZE", "QC_TOKEN_REPAIR", "QC_TOKEN_REPEATABLE",
  "QC_TOKEN_REPEAT", "QC_TOKEN_REPLACE", "QC_TOKEN_REPLICATION",
  "QC_TOKEN_REQUIRE", "QC_TOKEN_RESET", "QC_TOKEN_RESIGNAL",
  "QC_TOKEN_RESOURCES", "QC_TOKEN_RESTORE", "QC_TOKEN_RESTRICT",
  "QC_TOKEN_RESUME", "QC_TOKEN_RETURNS", "QC_TOKEN_RETURN",
  "QC_TOKEN_REVOKE", "QC_TOKEN_RIGHT", "QC_TOKEN_ROLLBACK",
  "QC_TOKEN_ROLLUP", "QC_TOKEN_ROUTINE", "QC_TOKEN_ROWS",
  "QC_TOKEN_ROW_FORMAT", "QC_TOKEN_ROW", "QC_TOKEN_RTREE",
  "QC_TOKEN_SAVEPOINT", "QC_TOKEN_SCHEDULE", "QC_TOKEN_SCHEMA_NAME",
  "QC_TOKEN_SECOND_MICROSECOND", "QC_TOKEN_SECOND", "QC_TOKEN_SECURITY",
  "QC_TOKEN_SELECT", "QC_TOKEN_SENSITIVE", "QC_TOKEN_SEPARATOR",
  "QC_TOKEN_SERIALIZABLE", "QC_TOKEN_SERIAL", "QC_TOKEN_SESSION",
  "QC_TOKEN_SERVER", "QC_TOKEN_SERVER_OPTIONS", "QC_TOKEN_SET",
  "QC_TOKEN_SET_VAR", "QC_TOKEN_SHARE", "QC_TOKEN_SHIFT_LEFT",
  "QC_TOKEN_SHIFT_RIGHT", "QC_TOKEN_SHOW", "QC_TOKEN_SHUTDOWN",
  "QC_TOKEN_SIGNAL", "QC_TOKEN_SIGNED", "QC_TOKEN_SIMPLE",
  "QC_TOKEN_SLAVE", "QC_TOKEN_SMALLINT", "QC_TOKEN_SNAPSHOT",
  "QC_TOKEN_SOCKET", "QC_TOKEN_SONAME", "QC_TOKEN_SOUNDS",
  "QC_TOKEN_SOURCE", "QC_TOKEN_SPATIAL", "QC_TOKEN_SPECIFIC",
  "QC_TOKEN_SQLEXCEPTION", "QC_TOKEN_SQLSTATE", "QC_TOKEN_SQLWARNING",
  "QC_TOKEN_SQL_BIG_RESULT", "QC_TOKEN_SQL_BUFFER_RESULT",
  "QC_TOKEN_SQL_CACHE", "QC_TOKEN_SQL_CALC_FOUND_ROWS",
  "QC_TOKEN_SQL_NO_CACHE", "QC_TOKEN_SQL_SMALL_RESULT", "QC_TOKEN_SQL",
  "QC_TOKEN_SQL_THREAD", "QC_TOKEN_SSL", "QC_TOKEN_STARTING",
  "QC_TOKEN_STARTS", "QC_TOKEN_START", "QC_TOKEN_STATUS",
  "QC_TOKEN_STDDEV_SAMP", "QC_TOKEN_STD", "QC_TOKEN_STOP",
  "QC_TOKEN_STORAGE", "QC_TOKEN_STRAIGHT_JOIN", "QC_TOKEN_STRING",
  "QC_TOKEN_SUBCLASS_ORIGIN", "QC_TOKEN_SUBDATE", "QC_TOKEN_SUBJECT",
  "QC_TOKEN_SUBPARTITIONS", "QC_TOKEN_SUBPARTITION", "QC_TOKEN_SUBSTRING",
  "QC_TOKEN_SUM", "QC_TOKEN_SUPER", "QC_TOKEN_SUSPEND", "QC_TOKEN_SWAPS",
  "QC_TOKEN_SWITCHES", "QC_TOKEN_SYSDATE", "QC_TOKEN_TABLES",
  "QC_TOKEN_TABLESPACE", "QC_TOKEN_TABLE_REF_PRIORITY", "QC_TOKEN_TABLE",
  "QC_TOKEN_TABLE_CHECKSUM", "QC_TOKEN_TABLE_NAME", "QC_TOKEN_TEMPORARY",
  "QC_TOKEN_TEMPTABLE", "QC_TOKEN_TERMINATED", "QC_TOKEN_TEXT_STRING",
  "QC_TOKEN_TEXT", "QC_TOKEN_THAN", "QC_TOKEN_THEN", "QC_TOKEN_TIMESTAMP",
  "QC_TOKEN_TIMESTAMP_ADD", "QC_TOKEN_TIMESTAMP_DIFF", "QC_TOKEN_TIME",
  "QC_TOKEN_TINYBLOB", "QC_TOKEN_TINYINT", "QC_TOKEN_TINYTEXT",
  "QC_TOKEN_TO", "QC_TOKEN_TRAILING", "QC_TOKEN_TRANSACTION",
  "QC_TOKEN_TRIGGERS", "QC_TOKEN_TRIGGER", "QC_TOKEN_TRIM",
  "QC_TOKEN_TRUE", "QC_TOKEN_TRUNCATE", "QC_TOKEN_TYPES", "QC_TOKEN_TYPE",
  "QC_TOKEN_UDF_RETURNS", "QC_TOKEN_ULONGLONG_NUM", "QC_TOKEN_UNCOMMITTED",
  "QC_TOKEN_UNDEFINED", "QC_TOKEN_UNDERSCORE_CHARSET", "QC_TOKEN_UNDOFILE",
  "QC_TOKEN_UNDO_BUFFER_SIZE", "QC_TOKEN_UNDO", "QC_TOKEN_UNICODE",
  "QC_TOKEN_UNINSTALL", "QC_TOKEN_UNION", "QC_TOKEN_UNIQUE",
  "QC_TOKEN_UNKNOWN", "QC_TOKEN_UNLOCK", "QC_TOKEN_UNSIGNED",
  "QC_TOKEN_UNTIL", "QC_TOKEN_UPDATE", "QC_TOKEN_UPGRADE",
  "QC_TOKEN_USAGE", "QC_TOKEN_USER", "QC_TOKEN_USE_FRM", "QC_TOKEN_USE",
  "QC_TOKEN_USING", "QC_TOKEN_UTC_DATE", "QC_TOKEN_UTC_TIMESTAMP",
  "QC_TOKEN_UTC_TIME", "QC_TOKEN_VALUES", "QC_TOKEN_VALUE",
  "QC_TOKEN_VARBINARY", "QC_TOKEN_VARCHAR", "QC_TOKEN_VARIABLES",
  "QC_TOKEN_VARIANCE", "QC_TOKEN_VARYING", "QC_TOKEN_VAR_SAMP",
  "QC_TOKEN_VIEW", "QC_TOKEN_WAIT", "QC_TOKEN_WARNINGS", "QC_TOKEN_WEEK",
  "QC_TOKEN_WHEN", "QC_TOKEN_WHERE", "QC_TOKEN_WHILE", "QC_TOKEN_WITH",
  "QC_TOKEN_WITH_CUBE", "QC_TOKEN_WITH_ROLLUP", "QC_TOKEN_WORK",
  "QC_TOKEN_WRAPPER", "QC_TOKEN_WRITE", "QC_TOKEN_X509", "QC_TOKEN_XA",
  "QC_TOKEN_XML", "QC_TOKEN_XOR", "QC_TOKEN_YEAR_MONTH", "QC_TOKEN_YEAR",
  "QC_TOKEN_ZEROFILL", "QC_TOKEN_CLIENT_FLAG", "QC_TOKEN_GLOBAL_VAR",
  "QC_TOKEN_SESSION_VAR", "QC_TOKEN_BRACKET_OPEN",
  "QC_TOKEN_BRACKET_CLOSE", "QC_TOKEN_PLUS", "QC_TOKEN_MINUS",
  "QC_TOKEN_STAR", "QC_TOKEN_COMMA", "QC_TOKEN_DOT", "QC_TOKEN_SEMICOLON",
  "QC_TOKEN_NO_MORE", "QC_TOKEN_IDENTIFIER", "QC_TOKEN_INTNUM",
  "QC_TOKEN_FLOATNUM", "QC_TOKEN_ASSIGN_TO_VAR", "QC_TOKEN_TILDE",
  "$accept", "statement", "drop", "temporary", "if_exists", "restrict",
  "alter", "rename", "table_or_tables_option", "replace", "replace_option",
  "truncate", "table_token", "delete", "delete_options", "delete_option",
  "update", "low_priority", "insert", "ignore", "select", "$@1", "$@2",
  "$@3", "comment", "query_field_list", "select_field_list",
  "select_field_list_tail", "select_field", "field", "identifier",
  "ident_alias", "query_table", "table", "from_clause", "query_table_list",
  "where_clause", "where_clause_tail", "field_value", "keyword",
  "keyword_label_in_sp", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,   430,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     455,   456,   457,   458,   459,   460,   461,   462,   463,   464,
     465,   466,   467,   468,   469,   470,   471,   472,   473,   474,
     475,   476,   477,   478,   479,   480,   481,   482,   483,   484,
     485,   486,   487,   488,   489,   490,   491,   492,   493,   494,
     495,   496,   497,   498,   499,   500,   501,   502,   503,   504,
     505,   506,   507,   508,   509,   510,   511,   512,   513,   514,
     515,   516,   517,   518,   519,   520,   521,   522,   523,   524,
     525,   526,   527,   528,   529,   530,   531,   532,   533,   534,
     535,   536,   537,   538,   539,   540,   541,   542,   543,   544,
     545,   546,   547,   548,   549,   550,   551,   552,   553,   554,
     555,   556,   557,   558,   559,   560,   561,   562,   563,   564,
     565,   566,   567,   568,   569,   570,   571,   572,   573,   574,
     575,   576,   577,   578,   579,   580,   581,   582,   583,   584,
     585,   586,   587,   588,   589,   590,   591,   592,   593,   594,
     595,   596,   597,   598,   599,   600,   601,   602,   603,   604,
     605,   606,   607,   608,   609,   610,   611,   612,   613,   614,
     615,   616,   617,   618,   619,   620,   621,   622,   623,   624,
     625,   626,   627,   628,   629,   630,   631,   632,   633,   634,
     635,   636,   637,   638,   639,   640,   641,   642,   643,   644,
     645,   646,   647,   648,   649,   650,   651,   652,   653,   654,
     655,   656,   657,   658,   659,   660,   661,   662,   663,   664,
     665,   666,   667,   668,   669,   670,   671,   672,   673,   674,
     675,   676,   677,   678,   679,   680,   681,   682,   683,   684,
     685,   686,   687,   688,   689,   690,   691,   692,   693,   694,
     695,   696,   697,   698,   699,   700,   701,   702,   703,   704,
     705,   706,   707,   708,   709,   710,   711,   712,   713,   714,
     715,   716,   717,   718,   719,   720,   721,   722,   723,   724,
     725,   726,   727,   728,   729,   730,   731,   732,   733,   734,
     735,   736,   737,   738,   739,   740,   741,   742,   743,   744,
     745,   746,   747,   748,   749,   750,   751,   752,   753,   754,
     755,   756,   757,   758,   759,   760,   761,   762,   763,   764,
     765,   766,   767,   768,   769,   770,   771,   772,   773,   774,
     775,   776,   777,   778,   779,   780,   781,   782,   783,   784,
     785,   786,   787,   788,   789,   790,   791,   792,   793,   794,
     795,   796,   797,   798,   799,   800,   801,   802,   803,   804,
     805,   806,   807,   808,   809,   810,   811,   812,   813,   814,
     815,   816,   817,   818,   819,   820,   821,   822,   823,   824,
     825,   826,   827,   828,   829,   830,   831,   832,   833,   834,
     835,   836,   837,   838,   839,   840,   841,   842,   843,   844,
     845,   846,   847,   848,   849,   850,   851,   852,   853,   854,
     855,   856,   857,   858,   859,   860,   861
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   607,   608,   608,   608,   608,   608,   608,   608,   608,
     608,   609,   610,   610,   611,   611,   612,   612,   612,   613,
     614,   615,   615,   616,   617,   617,   618,   619,   619,   620,
     621,   621,   622,   622,   622,   623,   624,   624,   625,   626,
     626,   628,   629,   630,   627,   631,   631,   632,   632,   633,
     633,   634,   635,   635,   635,   636,   636,   636,   637,   637,
     638,   638,   638,   639,   640,   640,   640,   641,   642,   642,
     642,   642,   643,   644,   645,   645,   646,   646,   646,   646,
     646,   646,   646,   646,   646,   646,   646,   646,   646,   646,
     646,   646,   646,   646,   646,   646,   646,   646,   646,   646,
     646,   646,   646,   646,   646,   646,   646,   646,   646,   646,
     646,   646,   646,   646,   646,   646,   646,   646,   646,   646,
     646,   646,   646,   646,   646,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647,   647,   647,   647,   647,   647,   647,
     647,   647,   647,   647
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     7,     1,     0,     2,     0,     1,     1,     0,     5,
       4,     1,     1,     4,     1,     1,     4,     1,     0,     5,
       2,     0,     1,     1,     1,     6,     1,     0,     5,     1,
       0,     0,     0,     0,     8,     1,     0,     2,     1,     3,
       1,     3,     1,     1,     1,     1,     3,     5,     1,     1,
       2,     1,     0,     2,     1,     1,     3,     2,     1,     3,
       4,     4,     2,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
      46,    45,     0,    10,     9,     8,     7,     6,     5,     4,
       3,     2,     0,     1,    40,    31,    13,     0,     0,    37,
      41,    28,    37,    39,     0,    34,    33,    32,     0,    31,
      12,     0,    40,    22,    21,     0,    25,    36,     0,    24,
      46,    27,     0,    40,     0,     0,    30,    15,     0,   125,
     126,   127,   128,   129,   130,   131,    77,   132,   133,   135,
     134,   136,   137,    78,    79,   138,   139,   140,   142,   141,
     143,    80,    81,   144,   145,   146,    82,    83,   147,   148,
      84,   149,   150,   151,   152,   153,    85,   154,   155,   156,
     157,   158,   159,    86,   160,   161,   162,   163,   165,   164,
     166,   167,   168,    87,   169,   170,   171,   172,   173,   174,
     175,    88,    64,   176,   177,   178,   194,    89,   179,   182,
     181,   180,   183,   184,   186,   185,   187,    90,   188,   189,
     190,   192,   191,   196,   197,   198,    91,   193,   199,   195,
     366,   201,   200,   202,   204,   203,    92,   205,    93,    94,
     206,   207,   208,   210,   211,   212,   217,    95,   209,   213,
     214,   215,   216,   218,    96,   219,   220,   221,   222,   223,
     224,   225,   226,   227,   228,   238,   231,   233,   234,   236,
     232,   237,   241,   240,   242,   243,   244,   239,   230,   235,
     245,   246,   229,   247,   248,   249,   250,   251,   252,   253,
     254,   255,   256,   257,   259,   258,   260,   261,   262,   263,
     264,   266,   265,   267,   268,   269,   271,   270,   273,   274,
      97,   272,   275,   276,   277,   278,   279,    98,    99,   100,
     280,   281,   101,   282,   283,   284,   102,   285,   286,   288,
     287,   289,   290,   103,   104,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   305,   304,
     306,   307,   308,   309,   310,   105,   311,   106,   312,   313,
     107,   314,   108,   315,   316,   109,   317,   318,   319,   320,
     321,   322,   110,   323,   324,   111,   326,   325,   327,   112,
     329,   330,   113,   328,   115,   331,   114,   116,   332,   333,
     335,   334,   336,   337,   338,   117,   339,   118,   340,   341,
     342,   343,   345,   344,   346,   347,   348,   349,   350,   352,
     351,   353,   354,   355,   356,   359,   360,   361,   362,   357,
     358,   119,   363,   364,   365,   367,   368,   370,   369,   120,
     121,   371,   372,   124,   373,   374,   377,   375,   376,   379,
     378,   380,   381,   122,   382,   123,   383,    58,    65,    20,
      59,    76,    23,     0,    42,    48,    50,    26,     0,    19,
      29,     0,     0,    38,     0,    54,    47,    53,    62,    52,
      55,     0,    46,     0,    14,    18,    66,     0,    61,    51,
       0,     0,    43,     0,    49,    35,    17,    16,    11,    60,
      56,    68,    62,    67,     0,     0,    63,     0,     0,     0,
       0,    44,    57,     0,     0,    69,     0,    72,    70,    71,
       0,    75,    74,    73
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,    31,   372,   398,     4,     5,    35,     6,
      38,     7,    42,     8,    28,    29,     9,    39,    10,    24,
      11,    40,   381,   404,    12,   364,   365,   366,   378,   379,
     358,   389,   401,   402,   392,   403,   411,   417,   423,   360,
     361
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -552
static const yytype_int16 yypact[] =
{
     -61,  -552,    14,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,   403,  -552,  -197,  -187,  -495,  -193,  -498,  -112,
    -552,  -474,  -233,  -552,  -467,  -552,  -552,  -552,  -137,  -187,
    -552,  -498,  -197,  -552,  -552,  1183,  -552,  -552,  1183,  -552,
     -61,  -552,  1183,  -197,  1183,  1183,  -552,  -165,  1183,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,  -551,  -552,
    -552,  -552,  -552,    -4,  -552,  -552,  -549,  -552,  1183,  -552,
    -552,  -111,  1183,  -552,  2349,  -547,  -552,  -552,  1766,  -552,
    -544,  -125,   -61,  -394,  -552,   -40,  -552,  2349,  -552,  -552,
    2349,  1183,  -552,   596,  -552,  -552,  -552,  -552,  -552,  -552,
    -535,  -552,  1766,  -219,  -508,  2349,  -552,  -168,  -167,  1183,
    2349,  -552,  -552,  1183,  1183,  -552,   -76,  -552,  -552,  -552,
    -486,  -552,  -552,  -552
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -552,  -552,  -552,  -552,  -552,  -552,  -552,  -552,    48,  -552,
    -552,  -552,  -552,  -552,    51,  -552,  -552,    59,  -552,    -3,
    -552,  -552,  -552,  -552,   -39,  -552,  -297,  -552,  -552,  -323,
    1025,  -314,  -382,   432,  -552,  -552,  -552,  -552,  -552,  -552,
    -552
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -342
static const yytype_int16 yytable[] =
{
      49,   363,    50,    51,    52,    53,    54,   421,    33,    36,
     396,    34,     1,    55,    13,    23,    56,    30,    57,    58,
      59,    60,    61,    62,    63,    25,    64,   415,   407,    48,
      65,   418,   419,    32,    66,    41,    37,    67,    68,    69,
     368,    70,    44,    71,    72,    45,   371,    73,   374,   382,
     384,    74,  -341,    75,    76,   390,    77,   391,    78,   395,
      79,    80,    81,    82,   405,    83,    84,   410,   413,   414,
      85,    86,    87,    88,    89,    90,   420,    91,    92,    47,
      46,    43,    26,    93,    94,   394,    95,   416,   406,    96,
       0,     0,    97,     0,     0,     0,     0,     0,     0,     0,
      98,    99,   100,     0,     0,   101,     0,     0,     0,     0,
     102,   103,     0,     0,     0,     0,   104,   422,   105,     0,
       0,     0,   106,     0,   107,   108,   109,   110,     0,     0,
       0,   111,     0,     0,   113,   114,   115,     0,     0,     0,
     116,     0,   117,   118,     0,   119,   120,   121,     0,     0,
     122,     0,   123,   124,   125,   126,   127,    37,     0,   128,
     129,   130,     0,     0,   131,   132,     0,   133,   134,   135,
       0,     0,   136,     0,     0,     0,   137,   138,     0,   139,
       0,   140,     0,   141,   142,   143,   144,     0,   145,     0,
       0,     0,   146,   147,     0,   148,     0,     0,   149,   150,
       0,     0,     0,   151,     0,   152,    27,     0,     0,     0,
     153,   154,   408,     0,   155,     0,     0,     0,     0,   156,
     157,     0,     0,     0,   158,     0,   159,   160,     0,   161,
     162,     0,     0,     0,   163,     0,     0,   164,   165,     0,
       0,   166,     0,     0,   167,   168,     0,     0,     0,     0,
       0,   169,   170,     0,   171,     0,   172,     0,   173,   174,
       0,     0,     0,     0,     0,     0,     0,   175,   176,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
       0,   188,   189,     0,     0,   190,   191,   192,   193,     0,
     194,   195,   196,     0,     0,     0,   197,   198,   199,     0,
     200,   201,     0,     0,   202,   203,     0,   204,     0,   205,
       0,   206,   207,   208,   209,   210,     0,   211,   212,   213,
       0,     0,   214,   215,     0,     0,     0,   216,   217,   218,
     219,     0,     0,     0,   220,   221,     0,     0,     0,     0,
     222,   223,   224,   393,   225,   226,   227,     0,   228,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   229,   230,
     231,     0,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   243,     0,     0,   244,   245,   246,   409,
     247,     0,   248,   249,   250,   251,   397,   252,   253,   254,
       0,     0,   255,     0,     0,     0,   256,   257,   258,   259,
     260,     0,     0,     0,   261,   262,   263,     0,   264,   265,
       0,   266,   267,   268,     0,    14,   269,     0,   270,     0,
     271,   272,     0,   273,   274,     0,     0,     0,   275,   276,
     277,   278,   279,   280,   281,   282,   283,     0,     0,   284,
     285,     0,     0,     0,   286,   287,   288,   289,     0,     0,
       0,   290,     0,     0,     0,   291,     0,   292,   293,   294,
       0,   295,   296,   297,   298,   299,     0,   359,     0,     0,
     362,     0,   300,   301,   367,   302,   369,   370,   303,     0,
     373,   304,   305,   306,     0,     0,   307,   308,     0,   375,
       0,   310,   311,   312,   313,     0,     0,   314,   315,   316,
     317,     0,   318,   319,     0,     0,   320,     0,   321,   322,
       0,     0,   323,   324,     0,   325,   326,   327,   328,     0,
       0,     0,     0,     0,   329,   330,    15,     0,     0,   331,
     332,   333,   334,     0,   335,   336,     0,   337,   338,    16,
     339,   340,     0,     0,   341,     0,     0,   342,     0,   343,
       0,   344,   345,     0,     0,     0,     0,     0,     0,   346,
       0,     0,   347,     0,     0,     0,   348,   349,   350,   351,
       0,     0,     0,     0,     0,     0,   352,   353,     0,   354,
     355,     0,     0,     0,   356,     0,     0,     0,     0,     0,
       0,     0,     0,   376,     0,     0,     0,     0,   357,   377,
      49,     0,    50,    51,    52,    53,    54,     0,     0,     0,
       0,     0,     0,    55,     0,     0,    56,     0,    57,    58,
      59,    60,    61,    62,    63,    17,    64,     0,     0,     0,
      65,     0,     0,     0,    66,     0,     0,    67,    68,    69,
       0,    70,     0,    71,    72,     0,     0,    73,     0,     0,
       0,    74,     0,    75,    76,     0,    77,     0,    78,     0,
      79,    80,    81,    82,     0,    83,    84,     0,     0,     0,
      85,    86,    87,    88,    89,    90,     0,    91,    92,     0,
       0,     0,     0,    93,    94,     0,    95,     0,     0,    96,
       0,     0,    97,     0,     0,     0,     0,     0,     0,     0,
      98,    99,   100,     0,     0,   101,     0,     0,     0,     0,
     102,   103,     0,     0,     0,     0,   104,     0,   105,     0,
       0,     0,   106,     0,   107,   108,   109,   110,     0,     0,
       0,   111,     0,     0,   113,   114,   115,     0,     0,     0,
     116,     0,   117,   118,     0,   119,   120,   121,     0,     0,
     122,     0,   123,   124,   125,   126,   127,     0,     0,   128,
     129,   130,     0,     0,   131,   132,     0,   133,   134,   135,
       0,     0,   136,     0,     0,     0,   137,   138,     0,   139,
       0,   140,     0,   141,   142,   143,   144,     0,   145,     0,
       0,     0,   146,   147,     0,   148,     0,     0,   149,   150,
     383,     0,     0,   151,   385,   152,     0,     0,     0,     0,
     153,   154,     0,     0,   155,     0,     0,    18,     0,   156,
     157,     0,    19,     0,   158,     0,   159,   160,     0,   161,
     162,     0,     0,     0,   163,     0,     0,   164,   165,     0,
       0,   166,     0,     0,   167,   168,     0,     0,    20,     0,
       0,   169,   170,     0,   171,     0,   172,     0,   173,   174,
       0,     0,     0,     0,     0,     0,     0,   175,   176,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
       0,   188,   189,     0,     0,   190,   191,   192,   193,     0,
     194,   195,   196,     0,     0,     0,   197,   198,   199,     0,
     200,   201,     0,     0,   202,   203,     0,   204,     0,   205,
       0,   206,   207,   208,   209,   210,     0,   211,   212,   213,
       0,     0,   214,   215,     0,     0,     0,   216,   217,   218,
     219,     0,     0,     0,   220,   221,    21,     0,     0,     0,
     222,   223,   224,     0,   225,   226,   227,     0,   228,     0,
       0,     0,     0,     0,     0,    22,     0,     0,   229,   230,
     231,     0,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   243,     0,     0,   244,   245,   246,     0,
     247,     0,   248,   249,   250,   251,     0,   252,   253,   254,
       0,     0,   255,     0,     0,     0,   256,   257,   258,   259,
     260,     0,     0,     0,   261,   262,   263,     0,   264,   265,
       0,   266,   267,   268,     0,     0,   269,     0,   270,     0,
     271,   272,     0,   273,   274,     0,     0,     0,   275,   276,
     277,   278,   279,   280,   281,   282,   283,     0,     0,   284,
     285,     0,     0,     0,   286,   287,   288,   289,     0,     0,
       0,   290,     0,     0,     0,   291,     0,   292,   293,   294,
       0,   295,   296,   297,   298,   299,     0,     0,     0,     0,
       0,     0,   300,   301,     0,   302,     0,     0,   303,     0,
       0,   304,   305,   306,     0,     0,   307,   308,     0,   375,
       0,   310,   311,   312,   313,     0,     0,   314,   315,   316,
     317,     0,   318,   319,     0,     0,   320,     0,   321,   322,
       0,     0,   323,   324,     0,   325,   326,   327,   328,     0,
       0,     0,     0,     0,   329,   330,     0,     0,     0,   331,
     332,   333,   334,     0,   335,   336,     0,   337,   338,     0,
     339,   340,     0,     0,   341,     0,     0,   342,     0,   343,
       0,   344,   345,     0,     0,     0,     0,     0,     0,   346,
       0,     0,   347,     0,     0,     0,   348,   349,   350,   351,
       0,     0,     0,     0,     0,     0,   352,   353,     0,   354,
     355,     0,     0,     0,   356,     0,     0,    49,     0,    50,
      51,    52,    53,    54,     0,     0,     0,     0,   357,   377,
      55,     0,     0,    56,     0,    57,    58,    59,    60,    61,
      62,    63,     0,    64,     0,     0,     0,    65,     0,     0,
       0,    66,     0,     0,    67,    68,    69,     0,    70,     0,
      71,    72,     0,     0,    73,     0,     0,     0,    74,     0,
      75,    76,     0,    77,     0,    78,     0,    79,    80,    81,
      82,     0,    83,    84,     0,     0,     0,    85,    86,    87,
      88,    89,    90,     0,    91,    92,     0,     0,     0,     0,
      93,    94,     0,    95,     0,     0,    96,     0,     0,    97,
       0,     0,     0,     0,     0,     0,     0,    98,    99,   100,
       0,     0,   101,     0,     0,     0,     0,   102,   103,     0,
       0,     0,     0,   104,     0,   105,     0,     0,     0,   106,
       0,   107,   108,   109,   110,     0,     0,     0,   111,     0,
     112,   113,   114,   115,     0,     0,     0,   116,     0,   117,
     118,     0,   119,   120,   121,     0,     0,   122,     0,   123,
     124,   125,   126,   127,     0,     0,   128,   129,   130,     0,
       0,   131,   132,     0,   133,   134,   135,     0,     0,   136,
       0,     0,     0,   137,   138,     0,   139,     0,   140,     0,
     141,   142,   143,   144,     0,   145,     0,     0,     0,   146,
     147,     0,   148,     0,     0,   149,   150,     0,   380,     0,
     151,     0,   152,     0,     0,     0,     0,   153,   154,   386,
       0,   155,     0,   388,     0,     0,   156,   157,     0,     0,
       0,   158,   399,   159,   160,   400,   161,   162,   380,     0,
       0,   163,     0,     0,   164,   165,     0,   388,   166,     0,
     412,   167,   168,     0,     0,   380,     0,     0,   169,   170,
       0,   171,     0,   172,     0,   173,   174,     0,     0,     0,
       0,     0,     0,     0,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,     0,   188,   189,
       0,     0,   190,   191,   192,   193,     0,   194,   195,   196,
       0,     0,     0,   197,   198,   199,     0,   200,   201,     0,
       0,   202,   203,     0,   204,     0,   205,     0,   206,   207,
     208,   209,   210,     0,   211,   212,   213,     0,     0,   214,
     215,     0,     0,     0,   216,   217,   218,   219,     0,     0,
       0,   220,   221,     0,     0,     0,     0,   222,   223,   224,
       0,   225,   226,   227,     0,   228,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   229,   230,   231,     0,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,     0,     0,   244,   245,   246,     0,   247,     0,   248,
     249,   250,   251,     0,   252,   253,   254,     0,     0,   255,
       0,     0,     0,   256,   257,   258,   259,   260,     0,     0,
       0,   261,   262,   263,     0,   264,   265,     0,   266,   267,
     268,     0,     0,   269,     0,   270,     0,   271,   272,     0,
     273,   274,     0,     0,     0,   275,   276,   277,   278,   279,
     280,   281,   282,   283,     0,     0,   284,   285,     0,     0,
       0,   286,   287,   288,   289,     0,     0,     0,   290,     0,
       0,     0,   291,     0,   292,   293,   294,     0,   295,   296,
     297,   298,   299,     0,     0,     0,     0,     0,     0,   300,
     301,     0,   302,     0,     0,   303,     0,     0,   304,   305,
     306,     0,     0,   307,   308,     0,   309,     0,   310,   311,
     312,   313,     0,     0,   314,   315,   316,   317,     0,   318,
     319,     0,     0,   320,     0,   321,   322,     0,     0,   323,
     324,     0,   325,   326,   327,   328,     0,     0,     0,     0,
       0,   329,   330,     0,     0,     0,   331,   332,   333,   334,
       0,   335,   336,     0,   337,   338,     0,   339,   340,     0,
       0,   341,     0,     0,   342,     0,   343,     0,   344,   345,
       0,     0,     0,     0,     0,     0,   346,     0,     0,   347,
       0,     0,     0,   348,   349,   350,   351,     0,     0,     0,
       0,     0,     0,   352,   353,     0,   354,   355,     0,     0,
      49,   356,    50,    51,    52,    53,    54,     0,     0,     0,
       0,     0,     0,    55,   387,   357,    56,     0,    57,    58,
      59,    60,    61,    62,    63,     0,    64,     0,     0,     0,
      65,     0,     0,     0,    66,     0,     0,    67,    68,    69,
       0,    70,     0,    71,    72,     0,     0,    73,     0,     0,
       0,    74,     0,    75,    76,     0,    77,     0,    78,     0,
      79,    80,    81,    82,     0,    83,    84,     0,     0,     0,
      85,    86,    87,    88,    89,    90,     0,    91,    92,     0,
       0,     0,     0,    93,    94,     0,    95,     0,     0,    96,
       0,     0,    97,     0,     0,     0,     0,     0,     0,     0,
      98,    99,   100,     0,     0,   101,     0,     0,     0,     0,
     102,   103,     0,     0,     0,     0,   104,     0,   105,     0,
       0,     0,   106,     0,   107,   108,   109,   110,     0,     0,
       0,   111,     0,     0,   113,   114,   115,     0,     0,     0,
     116,     0,   117,   118,     0,   119,   120,   121,     0,     0,
     122,     0,   123,   124,   125,   126,   127,     0,     0,   128,
     129,   130,     0,     0,   131,   132,     0,   133,   134,   135,
       0,     0,   136,     0,     0,     0,   137,   138,     0,   139,
       0,   140,     0,   141,   142,   143,   144,     0,   145,     0,
       0,     0,   146,   147,     0,   148,     0,     0,   149,   150,
       0,     0,     0,   151,     0,   152,     0,     0,     0,     0,
     153,   154,     0,     0,   155,     0,     0,     0,     0,   156,
     157,     0,     0,     0,   158,     0,   159,   160,     0,   161,
     162,     0,     0,     0,   163,     0,     0,   164,   165,     0,
       0,   166,     0,     0,   167,   168,     0,     0,     0,     0,
       0,   169,   170,     0,   171,     0,   172,     0,   173,   174,
       0,     0,     0,     0,     0,     0,     0,   175,   176,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
       0,   188,   189,     0,     0,   190,   191,   192,   193,     0,
     194,   195,   196,     0,     0,     0,   197,   198,   199,     0,
     200,   201,     0,     0,   202,   203,     0,   204,     0,   205,
       0,   206,   207,   208,   209,   210,     0,   211,   212,   213,
       0,     0,   214,   215,     0,     0,     0,   216,   217,   218,
     219,     0,     0,     0,   220,   221,     0,     0,     0,     0,
     222,   223,   224,     0,   225,   226,   227,     0,   228,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   229,   230,
     231,     0,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   243,     0,     0,   244,   245,   246,     0,
     247,     0,   248,   249,   250,   251,     0,   252,   253,   254,
       0,     0,   255,     0,     0,     0,   256,   257,   258,   259,
     260,     0,     0,     0,   261,   262,   263,     0,   264,   265,
       0,   266,   267,   268,     0,     0,   269,     0,   270,     0,
     271,   272,     0,   273,   274,     0,     0,     0,   275,   276,
     277,   278,   279,   280,   281,   282,   283,     0,     0,   284,
     285,     0,     0,     0,   286,   287,   288,   289,     0,     0,
       0,   290,     0,     0,     0,   291,     0,   292,   293,   294,
       0,   295,   296,   297,   298,   299,     0,     0,     0,     0,
       0,     0,   300,   301,     0,   302,     0,     0,   303,     0,
       0,   304,   305,   306,     0,     0,   307,   308,     0,   309,
       0,   310,   311,   312,   313,     0,     0,   314,   315,   316,
     317,     0,   318,   319,     0,     0,   320,     0,   321,   322,
       0,     0,   323,   324,     0,   325,   326,   327,   328,     0,
       0,     0,     0,     0,   329,   330,     0,     0,     0,   331,
     332,   333,   334,     0,   335,   336,     0,   337,   338,     0,
     339,   340,     0,     0,   341,     0,     0,   342,     0,   343,
       0,   344,   345,     0,     0,     0,     0,     0,     0,   346,
       0,     0,   347,     0,     0,     0,   348,   349,   350,   351,
       0,     0,     0,     0,     0,     0,   352,   353,     0,   354,
     355,     0,     0,    49,   356,    50,    51,    52,    53,    54,
       0,     0,     0,     0,     0,     0,    55,     0,   357,    56,
       0,    57,    58,    59,    60,    61,    62,    63,     0,    64,
       0,     0,     0,    65,     0,     0,     0,    66,     0,     0,
      67,    68,    69,     0,    70,     0,    71,    72,     0,     0,
      73,     0,     0,     0,    74,     0,    75,    76,     0,    77,
       0,    78,     0,    79,    80,    81,    82,     0,    83,    84,
       0,     0,     0,    85,    86,    87,    88,    89,    90,     0,
      91,    92,     0,     0,     0,     0,    93,    94,     0,    95,
       0,     0,    96,     0,     0,    97,     0,     0,     0,     0,
       0,     0,     0,    98,    99,   100,     0,     0,   101,     0,
       0,     0,     0,   102,   103,     0,     0,     0,     0,   104,
       0,   105,     0,     0,     0,   106,     0,   107,   108,   109,
     110,     0,     0,     0,   111,     0,     0,   113,   114,   115,
       0,     0,     0,   116,     0,   117,   118,     0,   119,   120,
     121,     0,     0,   122,     0,   123,   124,   125,   126,   127,
       0,     0,   128,   129,   130,     0,     0,   131,   132,     0,
     133,   134,   135,     0,     0,   136,     0,     0,     0,   137,
     138,     0,   139,     0,   140,     0,   141,   142,   143,   144,
       0,   145,     0,     0,     0,   146,   147,     0,   148,     0,
       0,   149,   150,     0,     0,     0,   151,     0,   152,     0,
       0,     0,     0,   153,   154,     0,     0,   155,     0,     0,
       0,     0,   156,   157,     0,     0,     0,   158,     0,   159,
     160,     0,   161,   162,     0,     0,     0,   163,     0,     0,
     164,   165,     0,     0,   166,     0,     0,   167,   168,     0,
       0,     0,     0,     0,   169,   170,     0,   171,     0,   172,
       0,   173,   174,     0,     0,     0,     0,     0,     0,     0,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,     0,   188,   189,     0,     0,   190,   191,
     192,   193,     0,   194,   195,   196,     0,     0,     0,   197,
     198,   199,     0,   200,   201,     0,     0,   202,   203,     0,
     204,     0,   205,     0,   206,   207,   208,   209,   210,     0,
     211,   212,   213,     0,     0,   214,   215,     0,     0,     0,
     216,   217,   218,   219,     0,     0,     0,   220,   221,     0,
       0,     0,     0,   222,   223,   224,     0,   225,   226,   227,
       0,   228,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   229,   230,   231,     0,   232,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,   243,     0,     0,   244,
     245,   246,     0,   247,     0,   248,   249,   250,   251,     0,
     252,   253,   254,     0,     0,   255,     0,     0,     0,   256,
     257,   258,   259,   260,     0,     0,     0,   261,   262,   263,
       0,   264,   265,     0,   266,   267,   268,     0,     0,   269,
       0,   270,     0,   271,   272,     0,   273,   274,     0,     0,
       0,   275,   276,   277,   278,   279,   280,   281,   282,   283,
       0,     0,   284,   285,     0,     0,     0,   286,   287,   288,
     289,     0,     0,     0,   290,     0,     0,     0,   291,     0,
     292,   293,   294,     0,   295,   296,   297,   298,   299,     0,
       0,     0,     0,     0,     0,   300,   301,     0,   302,     0,
       0,   303,     0,     0,   304,   305,   306,     0,     0,   307,
     308,     0,   309,     0,   310,   311,   312,   313,     0,     0,
     314,   315,   316,   317,     0,   318,   319,     0,     0,   320,
       0,   321,   322,     0,     0,   323,   324,     0,   325,   326,
     327,   328,     0,     0,     0,     0,     0,   329,   330,     0,
       0,     0,   331,   332,   333,   334,     0,   335,   336,     0,
     337,   338,     0,   339,   340,     0,     0,   341,     0,     0,
     342,     0,   343,     0,   344,   345,     0,     0,     0,     0,
       0,     0,   346,     0,     0,   347,     0,     0,     0,   348,
     349,   350,   351,     0,     0,     0,     0,     0,     0,   352,
     353,     0,   354,   355,     0,     0,     0,   356,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   357
};

static const yytype_int16 yycheck[] =
{
       4,    40,     6,     7,     8,     9,    10,   493,   506,   121,
      50,   509,    73,    17,     0,   212,    20,   512,    22,    23,
      24,    25,    26,    27,    28,   212,    30,   409,   247,    32,
      34,   413,   414,   226,    38,   509,   269,    41,    42,    43,
      43,    45,   509,    47,    48,   182,   211,    51,   599,   598,
     161,    55,   599,    57,    58,   599,    60,   182,    62,   453,
      64,    65,    66,    67,   599,    69,    70,   575,   236,   236,
      74,    75,    76,    77,    78,    79,   152,    81,    82,    31,
      29,    22,   269,    87,    88,   382,    90,   410,   402,    93,
      -1,    -1,    96,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     104,   105,   106,    -1,    -1,   109,    -1,    -1,    -1,    -1,
     114,   115,    -1,    -1,    -1,    -1,   120,   603,   122,    -1,
      -1,    -1,   126,    -1,   128,   129,   130,   131,    -1,    -1,
      -1,   135,    -1,    -1,   138,   139,   140,    -1,    -1,    -1,
     144,    -1,   146,   147,    -1,   149,   150,   151,    -1,    -1,
     154,    -1,   156,   157,   158,   159,   160,   269,    -1,   163,
     164,   165,    -1,    -1,   168,   169,    -1,   171,   172,   173,
      -1,    -1,   176,    -1,    -1,    -1,   180,   181,    -1,   183,
      -1,   185,    -1,   187,   188,   189,   190,    -1,   192,    -1,
      -1,    -1,   196,   197,    -1,   199,    -1,    -1,   202,   203,
      -1,    -1,    -1,   207,    -1,   209,   393,    -1,    -1,    -1,
     214,   215,   431,    -1,   218,    -1,    -1,    -1,    -1,   223,
     224,    -1,    -1,    -1,   228,    -1,   230,   231,    -1,   233,
     234,    -1,    -1,    -1,   238,    -1,    -1,   241,   242,    -1,
      -1,   245,    -1,    -1,   248,   249,    -1,    -1,    -1,    -1,
      -1,   255,   256,    -1,   258,    -1,   260,    -1,   262,   263,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   271,   272,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
      -1,   285,   286,    -1,    -1,   289,   290,   291,   292,    -1,
     294,   295,   296,    -1,    -1,    -1,   300,   301,   302,    -1,
     304,   305,    -1,    -1,   308,   309,    -1,   311,    -1,   313,
      -1,   315,   316,   317,   318,   319,    -1,   321,   322,   323,
      -1,    -1,   326,   327,    -1,    -1,    -1,   331,   332,   333,
     334,    -1,    -1,    -1,   338,   339,    -1,    -1,    -1,    -1,
     344,   345,   346,   382,   348,   349,   350,    -1,   352,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   362,   363,
     364,    -1,   366,   367,   368,   369,   370,   371,   372,   373,
     374,   375,   376,   377,    -1,    -1,   380,   381,   382,   598,
     384,    -1,   386,   387,   388,   389,   426,   391,   392,   393,
      -1,    -1,   396,    -1,    -1,    -1,   400,   401,   402,   403,
     404,    -1,    -1,    -1,   408,   409,   410,    -1,   412,   413,
      -1,   415,   416,   417,    -1,    12,   420,    -1,   422,    -1,
     424,   425,    -1,   427,   428,    -1,    -1,    -1,   432,   433,
     434,   435,   436,   437,   438,   439,   440,    -1,    -1,   443,
     444,    -1,    -1,    -1,   448,   449,   450,   451,    -1,    -1,
      -1,   455,    -1,    -1,    -1,   459,    -1,   461,   462,   463,
      -1,   465,   466,   467,   468,   469,    -1,    35,    -1,    -1,
      38,    -1,   476,   477,    42,   479,    44,    45,   482,    -1,
      48,   485,   486,   487,    -1,    -1,   490,   491,    -1,   493,
      -1,   495,   496,   497,   498,    -1,    -1,   501,   502,   503,
     504,    -1,   506,   507,    -1,    -1,   510,    -1,   512,   513,
      -1,    -1,   516,   517,    -1,   519,   520,   521,   522,    -1,
      -1,    -1,    -1,    -1,   528,   529,   123,    -1,    -1,   533,
     534,   535,   536,    -1,   538,   539,    -1,   541,   542,   136,
     544,   545,    -1,    -1,   548,    -1,    -1,   551,    -1,   553,
      -1,   555,   556,    -1,    -1,    -1,    -1,    -1,    -1,   563,
      -1,    -1,   566,    -1,    -1,    -1,   570,   571,   572,   573,
      -1,    -1,    -1,    -1,    -1,    -1,   580,   581,    -1,   583,
     584,    -1,    -1,    -1,   588,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   597,    -1,    -1,    -1,    -1,   602,   603,
       4,    -1,     6,     7,     8,     9,    10,    -1,    -1,    -1,
      -1,    -1,    -1,    17,    -1,    -1,    20,    -1,    22,    23,
      24,    25,    26,    27,    28,   222,    30,    -1,    -1,    -1,
      34,    -1,    -1,    -1,    38,    -1,    -1,    41,    42,    43,
      -1,    45,    -1,    47,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    57,    58,    -1,    60,    -1,    62,    -1,
      64,    65,    66,    67,    -1,    69,    70,    -1,    -1,    -1,
      74,    75,    76,    77,    78,    79,    -1,    81,    82,    -1,
      -1,    -1,    -1,    87,    88,    -1,    90,    -1,    -1,    93,
      -1,    -1,    96,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     104,   105,   106,    -1,    -1,   109,    -1,    -1,    -1,    -1,
     114,   115,    -1,    -1,    -1,    -1,   120,    -1,   122,    -1,
      -1,    -1,   126,    -1,   128,   129,   130,   131,    -1,    -1,
      -1,   135,    -1,    -1,   138,   139,   140,    -1,    -1,    -1,
     144,    -1,   146,   147,    -1,   149,   150,   151,    -1,    -1,
     154,    -1,   156,   157,   158,   159,   160,    -1,    -1,   163,
     164,   165,    -1,    -1,   168,   169,    -1,   171,   172,   173,
      -1,    -1,   176,    -1,    -1,    -1,   180,   181,    -1,   183,
      -1,   185,    -1,   187,   188,   189,   190,    -1,   192,    -1,
      -1,    -1,   196,   197,    -1,   199,    -1,    -1,   202,   203,
     368,    -1,    -1,   207,   372,   209,    -1,    -1,    -1,    -1,
     214,   215,    -1,    -1,   218,    -1,    -1,   414,    -1,   223,
     224,    -1,   419,    -1,   228,    -1,   230,   231,    -1,   233,
     234,    -1,    -1,    -1,   238,    -1,    -1,   241,   242,    -1,
      -1,   245,    -1,    -1,   248,   249,    -1,    -1,   445,    -1,
      -1,   255,   256,    -1,   258,    -1,   260,    -1,   262,   263,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   271,   272,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
      -1,   285,   286,    -1,    -1,   289,   290,   291,   292,    -1,
     294,   295,   296,    -1,    -1,    -1,   300,   301,   302,    -1,
     304,   305,    -1,    -1,   308,   309,    -1,   311,    -1,   313,
      -1,   315,   316,   317,   318,   319,    -1,   321,   322,   323,
      -1,    -1,   326,   327,    -1,    -1,    -1,   331,   332,   333,
     334,    -1,    -1,    -1,   338,   339,   533,    -1,    -1,    -1,
     344,   345,   346,    -1,   348,   349,   350,    -1,   352,    -1,
      -1,    -1,    -1,    -1,    -1,   552,    -1,    -1,   362,   363,
     364,    -1,   366,   367,   368,   369,   370,   371,   372,   373,
     374,   375,   376,   377,    -1,    -1,   380,   381,   382,    -1,
     384,    -1,   386,   387,   388,   389,    -1,   391,   392,   393,
      -1,    -1,   396,    -1,    -1,    -1,   400,   401,   402,   403,
     404,    -1,    -1,    -1,   408,   409,   410,    -1,   412,   413,
      -1,   415,   416,   417,    -1,    -1,   420,    -1,   422,    -1,
     424,   425,    -1,   427,   428,    -1,    -1,    -1,   432,   433,
     434,   435,   436,   437,   438,   439,   440,    -1,    -1,   443,
     444,    -1,    -1,    -1,   448,   449,   450,   451,    -1,    -1,
      -1,   455,    -1,    -1,    -1,   459,    -1,   461,   462,   463,
      -1,   465,   466,   467,   468,   469,    -1,    -1,    -1,    -1,
      -1,    -1,   476,   477,    -1,   479,    -1,    -1,   482,    -1,
      -1,   485,   486,   487,    -1,    -1,   490,   491,    -1,   493,
      -1,   495,   496,   497,   498,    -1,    -1,   501,   502,   503,
     504,    -1,   506,   507,    -1,    -1,   510,    -1,   512,   513,
      -1,    -1,   516,   517,    -1,   519,   520,   521,   522,    -1,
      -1,    -1,    -1,    -1,   528,   529,    -1,    -1,    -1,   533,
     534,   535,   536,    -1,   538,   539,    -1,   541,   542,    -1,
     544,   545,    -1,    -1,   548,    -1,    -1,   551,    -1,   553,
      -1,   555,   556,    -1,    -1,    -1,    -1,    -1,    -1,   563,
      -1,    -1,   566,    -1,    -1,    -1,   570,   571,   572,   573,
      -1,    -1,    -1,    -1,    -1,    -1,   580,   581,    -1,   583,
     584,    -1,    -1,    -1,   588,    -1,    -1,     4,    -1,     6,
       7,     8,     9,    10,    -1,    -1,    -1,    -1,   602,   603,
      17,    -1,    -1,    20,    -1,    22,    23,    24,    25,    26,
      27,    28,    -1,    30,    -1,    -1,    -1,    34,    -1,    -1,
      -1,    38,    -1,    -1,    41,    42,    43,    -1,    45,    -1,
      47,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,
      57,    58,    -1,    60,    -1,    62,    -1,    64,    65,    66,
      67,    -1,    69,    70,    -1,    -1,    -1,    74,    75,    76,
      77,    78,    79,    -1,    81,    82,    -1,    -1,    -1,    -1,
      87,    88,    -1,    90,    -1,    -1,    93,    -1,    -1,    96,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   104,   105,   106,
      -1,    -1,   109,    -1,    -1,    -1,    -1,   114,   115,    -1,
      -1,    -1,    -1,   120,    -1,   122,    -1,    -1,    -1,   126,
      -1,   128,   129,   130,   131,    -1,    -1,    -1,   135,    -1,
     137,   138,   139,   140,    -1,    -1,    -1,   144,    -1,   146,
     147,    -1,   149,   150,   151,    -1,    -1,   154,    -1,   156,
     157,   158,   159,   160,    -1,    -1,   163,   164,   165,    -1,
      -1,   168,   169,    -1,   171,   172,   173,    -1,    -1,   176,
      -1,    -1,    -1,   180,   181,    -1,   183,    -1,   185,    -1,
     187,   188,   189,   190,    -1,   192,    -1,    -1,    -1,   196,
     197,    -1,   199,    -1,    -1,   202,   203,    -1,   363,    -1,
     207,    -1,   209,    -1,    -1,    -1,    -1,   214,   215,   374,
      -1,   218,    -1,   378,    -1,    -1,   223,   224,    -1,    -1,
      -1,   228,   387,   230,   231,   390,   233,   234,   393,    -1,
      -1,   238,    -1,    -1,   241,   242,    -1,   402,   245,    -1,
     405,   248,   249,    -1,    -1,   410,    -1,    -1,   255,   256,
      -1,   258,    -1,   260,    -1,   262,   263,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   271,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,    -1,   285,   286,
      -1,    -1,   289,   290,   291,   292,    -1,   294,   295,   296,
      -1,    -1,    -1,   300,   301,   302,    -1,   304,   305,    -1,
      -1,   308,   309,    -1,   311,    -1,   313,    -1,   315,   316,
     317,   318,   319,    -1,   321,   322,   323,    -1,    -1,   326,
     327,    -1,    -1,    -1,   331,   332,   333,   334,    -1,    -1,
      -1,   338,   339,    -1,    -1,    -1,    -1,   344,   345,   346,
      -1,   348,   349,   350,    -1,   352,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   362,   363,   364,    -1,   366,
     367,   368,   369,   370,   371,   372,   373,   374,   375,   376,
     377,    -1,    -1,   380,   381,   382,    -1,   384,    -1,   386,
     387,   388,   389,    -1,   391,   392,   393,    -1,    -1,   396,
      -1,    -1,    -1,   400,   401,   402,   403,   404,    -1,    -1,
      -1,   408,   409,   410,    -1,   412,   413,    -1,   415,   416,
     417,    -1,    -1,   420,    -1,   422,    -1,   424,   425,    -1,
     427,   428,    -1,    -1,    -1,   432,   433,   434,   435,   436,
     437,   438,   439,   440,    -1,    -1,   443,   444,    -1,    -1,
      -1,   448,   449,   450,   451,    -1,    -1,    -1,   455,    -1,
      -1,    -1,   459,    -1,   461,   462,   463,    -1,   465,   466,
     467,   468,   469,    -1,    -1,    -1,    -1,    -1,    -1,   476,
     477,    -1,   479,    -1,    -1,   482,    -1,    -1,   485,   486,
     487,    -1,    -1,   490,   491,    -1,   493,    -1,   495,   496,
     497,   498,    -1,    -1,   501,   502,   503,   504,    -1,   506,
     507,    -1,    -1,   510,    -1,   512,   513,    -1,    -1,   516,
     517,    -1,   519,   520,   521,   522,    -1,    -1,    -1,    -1,
      -1,   528,   529,    -1,    -1,    -1,   533,   534,   535,   536,
      -1,   538,   539,    -1,   541,   542,    -1,   544,   545,    -1,
      -1,   548,    -1,    -1,   551,    -1,   553,    -1,   555,   556,
      -1,    -1,    -1,    -1,    -1,    -1,   563,    -1,    -1,   566,
      -1,    -1,    -1,   570,   571,   572,   573,    -1,    -1,    -1,
      -1,    -1,    -1,   580,   581,    -1,   583,   584,    -1,    -1,
       4,   588,     6,     7,     8,     9,    10,    -1,    -1,    -1,
      -1,    -1,    -1,    17,    18,   602,    20,    -1,    22,    23,
      24,    25,    26,    27,    28,    -1,    30,    -1,    -1,    -1,
      34,    -1,    -1,    -1,    38,    -1,    -1,    41,    42,    43,
      -1,    45,    -1,    47,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    57,    58,    -1,    60,    -1,    62,    -1,
      64,    65,    66,    67,    -1,    69,    70,    -1,    -1,    -1,
      74,    75,    76,    77,    78,    79,    -1,    81,    82,    -1,
      -1,    -1,    -1,    87,    88,    -1,    90,    -1,    -1,    93,
      -1,    -1,    96,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     104,   105,   106,    -1,    -1,   109,    -1,    -1,    -1,    -1,
     114,   115,    -1,    -1,    -1,    -1,   120,    -1,   122,    -1,
      -1,    -1,   126,    -1,   128,   129,   130,   131,    -1,    -1,
      -1,   135,    -1,    -1,   138,   139,   140,    -1,    -1,    -1,
     144,    -1,   146,   147,    -1,   149,   150,   151,    -1,    -1,
     154,    -1,   156,   157,   158,   159,   160,    -1,    -1,   163,
     164,   165,    -1,    -1,   168,   169,    -1,   171,   172,   173,
      -1,    -1,   176,    -1,    -1,    -1,   180,   181,    -1,   183,
      -1,   185,    -1,   187,   188,   189,   190,    -1,   192,    -1,
      -1,    -1,   196,   197,    -1,   199,    -1,    -1,   202,   203,
      -1,    -1,    -1,   207,    -1,   209,    -1,    -1,    -1,    -1,
     214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,   223,
     224,    -1,    -1,    -1,   228,    -1,   230,   231,    -1,   233,
     234,    -1,    -1,    -1,   238,    -1,    -1,   241,   242,    -1,
      -1,   245,    -1,    -1,   248,   249,    -1,    -1,    -1,    -1,
      -1,   255,   256,    -1,   258,    -1,   260,    -1,   262,   263,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   271,   272,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
      -1,   285,   286,    -1,    -1,   289,   290,   291,   292,    -1,
     294,   295,   296,    -1,    -1,    -1,   300,   301,   302,    -1,
     304,   305,    -1,    -1,   308,   309,    -1,   311,    -1,   313,
      -1,   315,   316,   317,   318,   319,    -1,   321,   322,   323,
      -1,    -1,   326,   327,    -1,    -1,    -1,   331,   332,   333,
     334,    -1,    -1,    -1,   338,   339,    -1,    -1,    -1,    -1,
     344,   345,   346,    -1,   348,   349,   350,    -1,   352,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   362,   363,
     364,    -1,   366,   367,   368,   369,   370,   371,   372,   373,
     374,   375,   376,   377,    -1,    -1,   380,   381,   382,    -1,
     384,    -1,   386,   387,   388,   389,    -1,   391,   392,   393,
      -1,    -1,   396,    -1,    -1,    -1,   400,   401,   402,   403,
     404,    -1,    -1,    -1,   408,   409,   410,    -1,   412,   413,
      -1,   415,   416,   417,    -1,    -1,   420,    -1,   422,    -1,
     424,   425,    -1,   427,   428,    -1,    -1,    -1,   432,   433,
     434,   435,   436,   437,   438,   439,   440,    -1,    -1,   443,
     444,    -1,    -1,    -1,   448,   449,   450,   451,    -1,    -1,
      -1,   455,    -1,    -1,    -1,   459,    -1,   461,   462,   463,
      -1,   465,   466,   467,   468,   469,    -1,    -1,    -1,    -1,
      -1,    -1,   476,   477,    -1,   479,    -1,    -1,   482,    -1,
      -1,   485,   486,   487,    -1,    -1,   490,   491,    -1,   493,
      -1,   495,   496,   497,   498,    -1,    -1,   501,   502,   503,
     504,    -1,   506,   507,    -1,    -1,   510,    -1,   512,   513,
      -1,    -1,   516,   517,    -1,   519,   520,   521,   522,    -1,
      -1,    -1,    -1,    -1,   528,   529,    -1,    -1,    -1,   533,
     534,   535,   536,    -1,   538,   539,    -1,   541,   542,    -1,
     544,   545,    -1,    -1,   548,    -1,    -1,   551,    -1,   553,
      -1,   555,   556,    -1,    -1,    -1,    -1,    -1,    -1,   563,
      -1,    -1,   566,    -1,    -1,    -1,   570,   571,   572,   573,
      -1,    -1,    -1,    -1,    -1,    -1,   580,   581,    -1,   583,
     584,    -1,    -1,     4,   588,     6,     7,     8,     9,    10,
      -1,    -1,    -1,    -1,    -1,    -1,    17,    -1,   602,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    -1,    30,
      -1,    -1,    -1,    34,    -1,    -1,    -1,    38,    -1,    -1,
      41,    42,    43,    -1,    45,    -1,    47,    48,    -1,    -1,
      51,    -1,    -1,    -1,    55,    -1,    57,    58,    -1,    60,
      -1,    62,    -1,    64,    65,    66,    67,    -1,    69,    70,
      -1,    -1,    -1,    74,    75,    76,    77,    78,    79,    -1,
      81,    82,    -1,    -1,    -1,    -1,    87,    88,    -1,    90,
      -1,    -1,    93,    -1,    -1,    96,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   104,   105,   106,    -1,    -1,   109,    -1,
      -1,    -1,    -1,   114,   115,    -1,    -1,    -1,    -1,   120,
      -1,   122,    -1,    -1,    -1,   126,    -1,   128,   129,   130,
     131,    -1,    -1,    -1,   135,    -1,    -1,   138,   139,   140,
      -1,    -1,    -1,   144,    -1,   146,   147,    -1,   149,   150,
     151,    -1,    -1,   154,    -1,   156,   157,   158,   159,   160,
      -1,    -1,   163,   164,   165,    -1,    -1,   168,   169,    -1,
     171,   172,   173,    -1,    -1,   176,    -1,    -1,    -1,   180,
     181,    -1,   183,    -1,   185,    -1,   187,   188,   189,   190,
      -1,   192,    -1,    -1,    -1,   196,   197,    -1,   199,    -1,
      -1,   202,   203,    -1,    -1,    -1,   207,    -1,   209,    -1,
      -1,    -1,    -1,   214,   215,    -1,    -1,   218,    -1,    -1,
      -1,    -1,   223,   224,    -1,    -1,    -1,   228,    -1,   230,
     231,    -1,   233,   234,    -1,    -1,    -1,   238,    -1,    -1,
     241,   242,    -1,    -1,   245,    -1,    -1,   248,   249,    -1,
      -1,    -1,    -1,    -1,   255,   256,    -1,   258,    -1,   260,
      -1,   262,   263,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     271,   272,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,    -1,   285,   286,    -1,    -1,   289,   290,
     291,   292,    -1,   294,   295,   296,    -1,    -1,    -1,   300,
     301,   302,    -1,   304,   305,    -1,    -1,   308,   309,    -1,
     311,    -1,   313,    -1,   315,   316,   317,   318,   319,    -1,
     321,   322,   323,    -1,    -1,   326,   327,    -1,    -1,    -1,
     331,   332,   333,   334,    -1,    -1,    -1,   338,   339,    -1,
      -1,    -1,    -1,   344,   345,   346,    -1,   348,   349,   350,
      -1,   352,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   362,   363,   364,    -1,   366,   367,   368,   369,   370,
     371,   372,   373,   374,   375,   376,   377,    -1,    -1,   380,
     381,   382,    -1,   384,    -1,   386,   387,   388,   389,    -1,
     391,   392,   393,    -1,    -1,   396,    -1,    -1,    -1,   400,
     401,   402,   403,   404,    -1,    -1,    -1,   408,   409,   410,
      -1,   412,   413,    -1,   415,   416,   417,    -1,    -1,   420,
      -1,   422,    -1,   424,   425,    -1,   427,   428,    -1,    -1,
      -1,   432,   433,   434,   435,   436,   437,   438,   439,   440,
      -1,    -1,   443,   444,    -1,    -1,    -1,   448,   449,   450,
     451,    -1,    -1,    -1,   455,    -1,    -1,    -1,   459,    -1,
     461,   462,   463,    -1,   465,   466,   467,   468,   469,    -1,
      -1,    -1,    -1,    -1,    -1,   476,   477,    -1,   479,    -1,
      -1,   482,    -1,    -1,   485,   486,   487,    -1,    -1,   490,
     491,    -1,   493,    -1,   495,   496,   497,   498,    -1,    -1,
     501,   502,   503,   504,    -1,   506,   507,    -1,    -1,   510,
      -1,   512,   513,    -1,    -1,   516,   517,    -1,   519,   520,
     521,   522,    -1,    -1,    -1,    -1,    -1,   528,   529,    -1,
      -1,    -1,   533,   534,   535,   536,    -1,   538,   539,    -1,
     541,   542,    -1,   544,   545,    -1,    -1,   548,    -1,    -1,
     551,    -1,   553,    -1,   555,   556,    -1,    -1,    -1,    -1,
      -1,    -1,   563,    -1,    -1,   566,    -1,    -1,    -1,   570,
     571,   572,   573,    -1,    -1,    -1,    -1,    -1,    -1,   580,
     581,    -1,   583,   584,    -1,    -1,    -1,   588,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   602
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,    73,   608,   609,   613,   614,   616,   618,   620,   623,
     625,   627,   631,     0,    12,   123,   136,   222,   414,   419,
     445,   533,   552,   212,   626,   212,   269,   393,   621,   622,
     512,   610,   226,   506,   509,   615,   121,   269,   617,   624,
     628,   509,   619,   624,   509,   182,   621,   615,   626,     4,
       6,     7,     8,     9,    10,    17,    20,    22,    23,    24,
      25,    26,    27,    28,    30,    34,    38,    41,    42,    43,
      45,    47,    48,    51,    55,    57,    58,    60,    62,    64,
      65,    66,    67,    69,    70,    74,    75,    76,    77,    78,
      79,    81,    82,    87,    88,    90,    93,    96,   104,   105,
     106,   109,   114,   115,   120,   122,   126,   128,   129,   130,
     131,   135,   137,   138,   139,   140,   144,   146,   147,   149,
     150,   151,   154,   156,   157,   158,   159,   160,   163,   164,
     165,   168,   169,   171,   172,   173,   176,   180,   181,   183,
     185,   187,   188,   189,   190,   192,   196,   197,   199,   202,
     203,   207,   209,   214,   215,   218,   223,   224,   228,   230,
     231,   233,   234,   238,   241,   242,   245,   248,   249,   255,
     256,   258,   260,   262,   263,   271,   272,   273,   274,   275,
     276,   277,   278,   279,   280,   281,   282,   283,   285,   286,
     289,   290,   291,   292,   294,   295,   296,   300,   301,   302,
     304,   305,   308,   309,   311,   313,   315,   316,   317,   318,
     319,   321,   322,   323,   326,   327,   331,   332,   333,   334,
     338,   339,   344,   345,   346,   348,   349,   350,   352,   362,
     363,   364,   366,   367,   368,   369,   370,   371,   372,   373,
     374,   375,   376,   377,   380,   381,   382,   384,   386,   387,
     388,   389,   391,   392,   393,   396,   400,   401,   402,   403,
     404,   408,   409,   410,   412,   413,   415,   416,   417,   420,
     422,   424,   425,   427,   428,   432,   433,   434,   435,   436,
     437,   438,   439,   440,   443,   444,   448,   449,   450,   451,
     455,   459,   461,   462,   463,   465,   466,   467,   468,   469,
     476,   477,   479,   482,   485,   486,   487,   490,   491,   493,
     495,   496,   497,   498,   501,   502,   503,   504,   506,   507,
     510,   512,   513,   516,   517,   519,   520,   521,   522,   528,
     529,   533,   534,   535,   536,   538,   539,   541,   542,   544,
     545,   548,   551,   553,   555,   556,   563,   566,   570,   571,
     572,   573,   580,   581,   583,   584,   588,   602,   637,   640,
     646,   647,   640,   631,   632,   633,   634,   640,   626,   640,
     640,   211,   611,   640,   599,   493,   597,   603,   635,   636,
     637,   629,   598,   640,   161,   640,   637,    18,   637,   638,
     599,   182,   641,   631,   633,   453,    50,   426,   612,   637,
     637,   639,   640,   642,   630,   599,   638,   247,   431,   598,
     575,   643,   637,   236,   236,   639,   636,   644,   639,   639,
     152,   493,   603,   645
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */





/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 11:

/* Line 1455 of yacc.c  */
#line 676 "mysqlnd_query_parser.grammar"
    { PINFO.statement = STATEMENT_DROP; zval_dtor(&(yyvsp[(1) - (7)].zv)); ;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 694 "mysqlnd_query_parser.grammar"
    {
					PINFO.statement = STATEMENT_ALTER;
					zval_dtor(&(yyvsp[(1) - (5)].zv));
					YYACCEPT;
				;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 703 "mysqlnd_query_parser.grammar"
    {
					PINFO.statement = STATEMENT_RENAME;
					zval_dtor(&(yyvsp[(1) - (4)].zv));
					YYACCEPT;
				;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 716 "mysqlnd_query_parser.grammar"
    {
					PINFO.statement = STATEMENT_REPLACE;
					zval_dtor(&(yyvsp[(1) - (4)].zv));
					YYACCEPT;
				;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 729 "mysqlnd_query_parser.grammar"
    {
					PINFO.statement = STATEMENT_TRUNCATE;
					zval_dtor(&(yyvsp[(1) - (4)].zv));
				;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 741 "mysqlnd_query_parser.grammar"
    {
					PINFO.statement = STATEMENT_DELETE;
					zval_dtor(&(yyvsp[(1) - (5)].zv));
					YYACCEPT;
				;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 759 "mysqlnd_query_parser.grammar"
    {
					PINFO.statement = STATEMENT_UPDATE;
					zval_dtor(&(yyvsp[(1) - (6)].zv));
					YYACCEPT;
				;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 773 "mysqlnd_query_parser.grammar"
    {
					PINFO.statement = STATEMENT_INSERT;
					zval_dtor(&(yyvsp[(1) - (5)].zv));
					YYACCEPT;
				;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 785 "mysqlnd_query_parser.grammar"
    {
					zval_dtor(&(yyvsp[(1) - (2)].zv));
					PINFO.statement = STATEMENT_SELECT;
					PINFO.active_field_list = &PINFO.select_field_list;
				;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 791 "mysqlnd_query_parser.grammar"
    {
					PINFO.active_field_list = NULL;
				;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 795 "mysqlnd_query_parser.grammar"
    {
					if (!PINFO.parse_where) {
//						YYACCEPT;
					}
					PINFO.active_field_list = &PINFO.where_field_list;
				;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 802 "mysqlnd_query_parser.grammar"
    {
					PINFO.active_field_list = NULL;
					YYACCEPT;			
				;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 809 "mysqlnd_query_parser.grammar"
    { ZVAL_NULL(&(yyval.zv)); ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 812 "mysqlnd_query_parser.grammar"
    { zval_dtor(&(yyvsp[(1) - (2)].zv)); ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 820 "mysqlnd_query_parser.grammar"
    { zval_dtor(&(yyvsp[(1) - (3)].zv)); zval_dtor(&(yyvsp[(3) - (3)].zv)); ;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 827 "mysqlnd_query_parser.grammar"
    {
					DBG_BLOCK_ENTER("identifier");
					if (PINFO.active_field_list) {
						struct st_mysqlnd_ms_field_info finfo = {0};
						finfo.persistent = PINFO.persistent;
						finfo.name = mnd_pestrndup(Z_STRVAL((yyvsp[(1) - (1)].zv)), Z_STRLEN((yyvsp[(1) - (1)].zv)), finfo.persistent);
						zend_llist_add_element(PINFO.active_field_list, &finfo);
					}
					zval_dtor(&(yyvsp[(1) - (1)].zv));
					DBG_BLOCK_LEAVE;
				;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 839 "mysqlnd_query_parser.grammar"
    {
					DBG_BLOCK_ENTER("identifier . identifier");
					if (PINFO.active_field_list) {
						struct st_mysqlnd_ms_field_info finfo = {0};
						finfo.persistent = PINFO.persistent;
						finfo.table = mnd_pestrndup(Z_STRVAL((yyvsp[(1) - (3)].zv)), Z_STRLEN((yyvsp[(1) - (3)].zv)), finfo.persistent);
						finfo.name = mnd_pestrndup(Z_STRVAL((yyvsp[(3) - (3)].zv)), Z_STRLEN((yyvsp[(3) - (3)].zv)), finfo.persistent);
						zend_llist_add_element(PINFO.active_field_list, &finfo);
					}
					zval_dtor(&(yyvsp[(1) - (3)].zv));
					zval_dtor(&(yyvsp[(3) - (3)].zv));
					DBG_BLOCK_LEAVE;
				;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 853 "mysqlnd_query_parser.grammar"
    {
					DBG_BLOCK_ENTER("identifier . identifier . identifier");
					if (PINFO.active_field_list) {
						struct st_mysqlnd_ms_field_info finfo = {0};
						finfo.persistent = PINFO.persistent;
						finfo.db = mnd_pestrndup(Z_STRVAL((yyvsp[(1) - (5)].zv)), Z_STRLEN((yyvsp[(1) - (5)].zv)), finfo.persistent);
						finfo.table = mnd_pestrndup(Z_STRVAL((yyvsp[(3) - (5)].zv)), Z_STRLEN((yyvsp[(3) - (5)].zv)), finfo.persistent);
						finfo.name = mnd_pestrndup(Z_STRVAL((yyvsp[(5) - (5)].zv)), Z_STRLEN((yyvsp[(5) - (5)].zv)), finfo.persistent);
						zend_llist_add_element(PINFO.active_field_list, &finfo);
					}
					zval_dtor(&(yyvsp[(1) - (5)].zv));
					zval_dtor(&(yyvsp[(3) - (5)].zv));
					zval_dtor(&(yyvsp[(5) - (5)].zv));
					DBG_BLOCK_LEAVE;
				;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 870 "mysqlnd_query_parser.grammar"
    { (yyval.zv)=(yyvsp[(1) - (1)].zv); ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 871 "mysqlnd_query_parser.grammar"
    { ZVAL_STRING(&((yyval.zv)), (yyvsp[(1) - (1)].kn), 1); ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 875 "mysqlnd_query_parser.grammar"
    { (yyval.zv) = (yyvsp[(2) - (2)].zv); ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 876 "mysqlnd_query_parser.grammar"
    { (yyval.zv) = (yyvsp[(1) - (1)].zv); ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 877 "mysqlnd_query_parser.grammar"
    { ZVAL_NULL(&(yyval.zv)); ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 881 "mysqlnd_query_parser.grammar"
    {
					DBG_BLOCK_ENTER("alias");
					if (Z_TYPE((yyvsp[(2) - (2)].zv)) == IS_STRING) {
						zend_llist_position tmp_pos;
						struct st_mysqlnd_ms_table_info * tinfo;
						if ((tinfo = zend_llist_get_last_ex(&PINFO.table_list, &tmp_pos))) {
							tinfo->org_table = tinfo->table;
							tinfo->table = mnd_pestrndup(Z_STRVAL((yyvsp[(2) - (2)].zv)), Z_STRLEN((yyvsp[(2) - (2)].zv)), tinfo->persistent);

							DBG_INF_FMT("ident_alias type = %d", Z_TYPE((yyvsp[(2) - (2)].zv)));				
						}
					}
					zval_dtor(&(yyvsp[(2) - (2)].zv));
					DBG_BLOCK_LEAVE
				;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 899 "mysqlnd_query_parser.grammar"
    {
					struct st_mysqlnd_ms_table_info tinfo = {0};
					DBG_BLOCK_ENTER("table");
					tinfo.persistent = PINFO.persistent;
					tinfo.table = mnd_pestrndup("DUAL", sizeof("DUAL") - 1, tinfo.persistent);
					zend_llist_add_element(&PINFO.table_list, &tinfo);
					DBG_INF_FMT("table=%s", tinfo.table);
					DBG_BLOCK_LEAVE;
				;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 909 "mysqlnd_query_parser.grammar"
    {
					struct st_mysqlnd_ms_table_info tinfo = {0};
					DBG_BLOCK_ENTER("table");
					tinfo.persistent = PINFO.persistent;
					tinfo.table = mnd_pestrndup(Z_STRVAL((yyvsp[(1) - (1)].zv)), Z_STRLEN((yyvsp[(1) - (1)].zv)), tinfo.persistent);
					zend_llist_add_element(&PINFO.table_list, &tinfo);
					zval_dtor(&(yyvsp[(1) - (1)].zv));
					DBG_BLOCK_LEAVE;
				;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 919 "mysqlnd_query_parser.grammar"
    {
					struct st_mysqlnd_ms_table_info tinfo = {0};
					DBG_BLOCK_ENTER("db.table");
					tinfo.persistent = PINFO.persistent;
					tinfo.db = mnd_pestrndup(Z_STRVAL((yyvsp[(1) - (3)].zv)), Z_STRLEN((yyvsp[(1) - (3)].zv)), tinfo.persistent);
					tinfo.table = mnd_pestrndup(Z_STRVAL((yyvsp[(3) - (3)].zv)), Z_STRLEN((yyvsp[(3) - (3)].zv)), tinfo.persistent);
					zend_llist_add_element(&PINFO.table_list, &tinfo);

					DBG_INF_FMT("table=%s", Z_STRVAL((yyvsp[(3) - (3)].zv)));

					zval_dtor(&(yyvsp[(1) - (3)].zv));
					zval_dtor(&(yyvsp[(3) - (3)].zv));
					DBG_BLOCK_LEAVE
				;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 940 "mysqlnd_query_parser.grammar"
    {YYACCEPT;;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 941 "mysqlnd_query_parser.grammar"
    {YYACCEPT;;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 949 "mysqlnd_query_parser.grammar"
    {
					zend_llist_position pos;
					struct st_mysqlnd_ms_field_info * finfo = zend_llist_get_last_ex(PINFO.active_field_list, &pos);
					if (finfo) {
						/* gotta be always true */
						finfo->custom_data = "=";
					}
					YYACCEPT;
				;}
    break;



/* Line 1455 of yacc.c  */
#line 3589 "mysqlnd_query_parser.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



