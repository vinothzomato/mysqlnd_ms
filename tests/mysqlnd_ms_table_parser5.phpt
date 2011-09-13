--TEST--
parser: SELECT PASSWORD('foo') AS _id
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

if ($error = mst_create_config("test_mysqlnd_ms_table_parser5.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_parser5.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	function mst_mysqli_fetch_id($offset, $res) {
		if (!$res) {
			printf("[%03d] No result\n", $offset);
			return;
		}
		$row = $res->fetch_assoc();
		printf("[%03d] _id = '%s'\n", $offset, $row['_id']);
	}

	$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno())
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	mst_mysqli_create_test_table($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);
	mst_mysqli_query(2, $link, "SELECT 1 FROM test", MYSQLND_MS_SLAVE_SWITCH);
	$thread_id = $link->thread_id;

	mst_mysqli_fetch_id(4, mst_mysqli_query(3, $link, "SELECT PASSWORD('foo') AS _id FROM test"));
	if ($thread_id != $link->thread_id)
		printf("[005] Statement has not been executed on the slave\n");

	print "done!";
?>
--CLEAN--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	if (!unlink("test_mysqlnd_ms_table_parser5.ini"))
		printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_parser5.ini'.\n");

	if ($err = mst_mysqli_drop_test_table($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket))
		printf("[clean] %s\n", $err);
?>
--EXPECTF--
[004] _id = '%s'
done!