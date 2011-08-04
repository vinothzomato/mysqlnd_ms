--TEST--
parser
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
if ($error = create_config("test_mysqlnd_ms_table_parser.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_parser.ini
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
	fetch_result(5, verbose_run_query(4, $link, "SELECT CONNECTION_ID() AS _id FROM test"));
	fetch_result(7, verbose_run_query(6, $link, "SELECT id AS _id FROM test"));
	fetch_result(9, verbose_run_query(8, $link, "SELECT id, id AS _id FROM test"));
	/* TODO - enable after fix of mysqlnd_ms_table_parser1.phpt
	fetch_result(11, verbose_run_query(10, $link, "SELECT id, id, 'a' AS _id FROM test"));
	*/
	fetch_result(14, verbose_run_query(12, $link, "SELECT 1 AS _id FROM test"));
	/* OK - mysqlnd_ms_table_parser2.phpt
	fetch_result(16, verbose_run_query(14, $link, "SELECT"));
	*/

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_table_parser.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_parser.ini'.\n");
?>
--EXPECTF--
[004 + 01] Query 'SELECT CONNECTION_ID() AS _id FROM test1'
[004 + 02] Thread '%d'
[005] _id = '%d'
done!