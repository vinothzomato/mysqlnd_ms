--TEST--
table filter basics: SQL hint
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array(
			"master1" => array(
				"host" => $master_host_only,
			),
		),
		'slave' => array(
			"slave1" => array(
				'host' => $slave_host_only,
			),
		 ),
		'lazy_connections' => 0,
		'filters' => array(
			"table" => array(
				"rules" => array(
					$db . ".test1%" => array(
						"master" => array("master1"),
						"slave" => array("slave1"),
					),
				),
			),
			"roundrobin" => array(),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_table_sql_hint.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.force_config_usage=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_sql_hint.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	/* shall use host = forced_master_hostname_abstract_name from the ini file */
	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	run_query(2, $link, "DROP TABLE IF EXISTS test1");
	run_query(3, $link, "CREATE TABLE test1(id INT)");
	run_query(4, $link, "INSERT INTO test1(id) VALUES (CONNECTION_ID())", MYSQLND_MS_LAST_USED_SWITCH);
	var_dump($link->thread_id);
	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_table_sql_hint.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_sql_hint.ini'.\n");
?>
--EXPECTF--
%d
done!