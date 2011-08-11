--TEST--
parser
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_check_feature(array("parser"));
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
		),
	),
);

if (_skipif_have_feature("table_filter")) {
	$settings['myapp']['filters']['table'] = array(
		"rules" => array(
			$db . ".test" => array(
				"master" => array("master1"),
				"slave" => array("slave1"),
			),
		),
	);
}

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
	require_once("mysqlnd_ms_table_parser.inc");

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno())
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());


	create_test_table($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);
	fetch_result(5, verbose_run_query(4, $link, "SELECT CONNECTION_ID() AS _id FROM test"));
	fetch_result(7, verbose_run_query(6, $link, "SELECT id AS _id FROM test ORDER BY id ASC"));
	fetch_result(9, verbose_run_query(8, $link, "SELECT id, id AS _id FROM test ORDER BY id ASC"));
	/* TODO - enable after fix of mysqlnd_ms_table_parser1.phpt
	fetch_result(11, verbose_run_query(10, $link, "SELECT id, id, 'a' AS _id FROM test"));
	*/
	fetch_result(13, verbose_run_query(12, $link, "SELECT 1 AS _id FROM test ORDER BY id ASC"));
	/* OK - mysqlnd_ms_table_parser2.phpt
	fetch_result(15, verbose_run_query(14, $link, "SELECT"));
	mysqlnd_ms_table_parser4.phpt
	fetch_result(17, verbose_run_query(16, $link, "SELECT NULL, 1 AS _id FROM test"));
	mysqlnd_ms_table_parser5.phpt
	fetch_result(19, verbose_run_query(18, $link, "SELECT PASSWORD('foo') AS _id FROM test"));
	*/

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_table_parser.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_parser.ini'.\n");
?>
--EXPECTF--
[004 + 01] Query 'SELECT CONNECTION_ID() AS _id FROM test'
[004 + 02] Thread '%d'
[005] _id = '%d'
[006 + 01] Query 'SELECT id AS _id FROM test ORDER BY id ASC'
[006 + 02] Thread '%d'
[007] _id = '1'
[008 + 01] Query 'SELECT id, id AS _id FROM test ORDER BY id ASC'
[008 + 02] Thread '%d'
[009] _id = '1'
[012 + 01] Query 'SELECT 1 AS _id FROM test ORDER BY id ASC'
[012 + 02] Thread '%d'
[013] _id = '1'
done!