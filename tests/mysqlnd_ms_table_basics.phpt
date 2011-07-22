--TEST--
table filter basics
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

		'lazy_connections' => 1,
		'filters' => array(
			"table" => array(
				"rules" => array(
					$db . ".test%" => array(
						"master" => array("master1"),
						"slave" => array("slave1"),
					),
				),
			),

			"roundrobin" => array(),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_table_basics.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_basics.ini
mysqlnd.debug="d:t:O,/tmp/mysqlnd.trace"
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");
	require_once("mysqlnd_ms_table.inc");

	if (($error = set_server_ids("test_mysqlnd_ms_table_basics.ini", $user, $passwd, $db, $port, $socket)))
	  printf("[001] %s\n", $error);

	/* shall use host = forced_master_hostname_abstract_name from the ini file */
	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	run_query(3, $link, "DROP TABLE IF EXISTS test1");
	var_dump($server_id = get_server_id($link, $db));



	$link->query("CREATE TABLE test1(id INT)");

	print "done!";
?>
--CLEAN--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_table.inc");

	clean_server_id("test_mysqlnd_ms_table_basics.ini", $user, $passwd, $db, $port, $socket);

	if (!unlink("test_mysqlnd_ms_table_basics.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_basics.ini'.\n");
?>
--EXPECTF--
string(7) "master1"
done!