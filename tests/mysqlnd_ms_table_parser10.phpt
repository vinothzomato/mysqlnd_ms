--TEST--
parser: SELECT SQL_SMALL_RESULT
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_check_feature(array("table_filter"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

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
if ($error = create_config("test_mysqlnd_ms_table_parser10.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_parser10.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");
	require_once("mysqlnd_ms_table_parser.inc");

	create_test_table($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);
	$sql = "SELECT SQL_SMALL_RESULT id AS _id FROM test GROUP BY id ORDER BY id ASC";
	if (server_supports_query(1, $sql, $slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket)) {

		$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
		if (mysqli_connect_errno())
			printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

		fetch_result(4, verbose_run_query(3, $link, $sql));
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_table_parser10.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_parser10.ini'.\n");
?>
--EXPECTF--
[001] Testing server support of 'SELECT SQL_SMALL_RESULT id AS _id FROM test GROUP BY id ORDER BY id ASC'
[003 + 01] Query 'SELECT SQL_SMALL_RESULT id AS _id FROM test GROUP BY id ORDER BY id ASC'
[003 + 02] Thread '%d'
[004] _id = '%d'
done!