--TEST--
parser: SELECT 'a' AS _id FROM test
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array(
			 "master1" => array(
				'host' 		=> $master_host_only,
				'port' 		=> (int)$master_port,
				'socket' 	=> $master_socket,
			),
		),
		'slave' => array(
			 "slave1" => array(
				'host' 	=> $slave_host_only,
				'port' 	=> (int)$slave_port,
				'socket' 	=> $slave_socket,
			),
		),
		'lazy_connections' => 0,
		'filters' => array(
			"table" => array(
				"rules" => array(
					$db . ".test" => array(
						"master" => array("master1"),
						"slave" => array("slave1"),
					),
				),
			),
			"roundrobin" => array(),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_table_parser1.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_parser1.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	function fetch_result($offset, $res) {
		if (!$res) {
			printf("[%03d] No result\n", $offset);
			return;
		}
		$row = $res->fetch_assoc();
		printf("[%03d] _id = '%s'\n", $offset, $row['_id']);
	}

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno())
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());


	create_test_table($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);
	fetch_result(3, verbose_run_query(2, $link, "SELECT 'a' AS _id FROM test"));

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_table_parser1.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_parser1.ini'.\n");
?>
--EXPECTF--
[002 + 01] Query 'SELECT 'a' AS _id FROM test'
[002 + 02] Thread '%d'
[003] _id = 'a'
done!