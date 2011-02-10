--TEST--
int mysqlnd_ms_query_is_select(string query) - check where to send query
--SKIPIF--
<?php
require_once("connect.inc");
require_once('skipif.inc');
$settings = array(
	"myapp" => array(
		'master' => array($host),
		'slave' => array($host),
	),
);
if ($error = create_config("mysqlnd_ms_query_is_select.ini", $settings))
  die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=mysqlnd_ms_query_is_select.ini
--FILE--
<?php
	function serverflag2name($where) {
		$server = "";
		switch ($where) {
			case MYSQLND_MS_QUERY_USE_LAST_USED:
			  $server = "last used";
			  break;
			case MYSQLND_MS_QUERY_USE_MASTER:
			  $server = "master";
			  break;
			case MYSQLND_MS_QUERY_USE_SLAVE:
 			  $server = "slave";
			  break;
			default:
			  $server = "unknown";
			  break;
		}
		return $server;
	}

	$queries = array(
		""			=> MYSQLND_MS_QUERY_USE_MASTER,
		"SELECT 1 FROM DUAL" 	=> MYSQLND_MS_QUERY_USE_SLAVE,
		"INSERT INTO test(id) VALUES (1)" => MYSQLND_MS_QUERY_USE_MASTER,
		"/*SELECT*/DELETE FROM test" => MYSQLND_MS_QUERY_USE_MASTER,
		"CREATE TABLE test(id INT)" => MYSQLND_MS_QUERY_USE_MASTER,
		"/*INSERT*/SELECT 2 FROM DUAL" => MYSQLND_MS_QUERY_USE_SLAVE,

		"/*" . MYSQLND_MS_LAST_USED_SWITCH . "*/INSERT INTO test(id) VALUES(2)" => MYSQLND_MS_QUERY_USE_LAST_USED,
		"/*" . MYSQLND_MS_MASTER_SWITCH . "*/SELECT 3 FROM DUAL" => MYSQLND_MS_QUERY_USE_MASTER,
		"/*" . MYSQLND_MS_SLAVE_SWITCH . "*/DELETE FROM test" => MYSQLND_MS_QUERY_USE_SLAVE,

		"CALL p()/*" . MYSQLND_MS_MASTER_SWITCH . "*/" => MYSQLND_MS_QUERY_USE_MASTER,

		 /* Note: tokenizer is stupid - only switches at the very beginning are recognized */
		"SELECT 1 FROM DUAL/*" . MYSQLND_MS_MASTER_SWITCH . "*/" => MYSQLND_MS_QUERY_USE_SLAVE,
		"INSERT INTO test(id) VALUES (1); SELECT 1 FROM DUAL/*" . MYSQLND_MS_SLAVE_SWITCH . "*/" => MYSQLND_MS_QUERY_USE_MASTER,
	);

	if (!is_null(($tmp = @mysqlnd_ms_query_is_select())))
		printf("[001] Expecting NULL got %s/%s\n", gettype($tmp), var_export($tmp, true));

	foreach ($queries as $query => $expected) {
		$where = mysqlnd_ms_query_is_select($query);
		$server = serverflag2name($where);
		if ($where != $expected) {
			$expected_server = serverflag2name($expected);
			printf("[002] '%s' should be run on '%s' but function recommends '%s'\n",
				$query, $expected_server, $server);
		} else {
		  printf("'%s' => '%s'\n", $query, $server);
		}
	}
	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("mysqlnd_ms_query_is_select.ini"))
	  printf("[clean] Cannot unlink ini file 'mysqlnd_ms_query_is_select.ini'.\n");
?>
--EXPECTF--
'' => 'master'
'SELECT 1 FROM DUAL' => 'slave'
'INSERT INTO test(id) VALUES (1)' => 'master'
'/*SELECT*/DELETE FROM test' => 'master'
'CREATE TABLE test(id INT)' => 'master'
'/*INSERT*/SELECT 2 FROM DUAL' => 'slave'
'/*ms=last_used*/INSERT INTO test(id) VALUES(2)' => 'last used'
'/*ms=master*/SELECT 3 FROM DUAL' => 'master'
'/*ms=slave*/DELETE FROM test' => 'slave'
'CALL p()/*ms=master*/' => 'master'
'SELECT 1 FROM DUAL/*ms=master*/' => 'slave'
'INSERT INTO test(id) VALUES (1); SELECT 1 FROM DUAL/*ms=slave*/' => 'master'
done!