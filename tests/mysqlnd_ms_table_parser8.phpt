--TEST--
parser: SELECT DISTINCT
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
			"roundrobin" => array(),
		),
	),
);

if (_skipif_have_feature("table_filter")) {
	$settings['filters']['table'] = array(
		"rules" => array(
			 $db . ".test" => array(
				  "master" => array("master1"),
				  "slave" => array("slave1"),
			),
		),
	);
}

if ($error = create_config("test_mysqlnd_ms_table_parser8.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_parser8.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");
	require_once("mysqlnd_ms_table_parser.inc");

	create_test_table($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);
	$sql = "SELECT DISTINCT id AS _id FROM test ORDER BY id ASC";
	if (server_supports_query(1, $sql, $slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket)) {

		$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
		if (mysqli_connect_errno())
			printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

		run_query(3, $link, "SELECT 1 FROM test", MYSQLND_MS_SLAVE_SWITCH);
		$thread_id = $link->thread_id;

		fetch_result(5, run_query(4, $link, $sql));
		if ($thread_id != $link->thread_id)
			printf("[006] Statement has not been executed on the slave\n");

	} else {
		/* fake result */
		printf("[005] _id = '1'\n");
	}

	print "done!";
?>
--CLEAN--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	if (!unlink("test_mysqlnd_ms_table_parser8.ini"))
		printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_parser8.ini'.\n");

	if ($err = drop_test_table($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket))
		printf("[clean] %s\n", $err);
?>
--EXPECTF--
[001] Testing server support of 'SELECT DISTINCT id AS _id FROM test ORDER BY id ASC'
[005] _id = '1'
done!