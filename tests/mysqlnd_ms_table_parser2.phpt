--TEST--
parser: SELECT
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
			"%" => array(
				"master" => array("master1"),
				"slave" => array("slave1"),
			),
		),
	);
}

if ($error = create_config("test_mysqlnd_ms_table_parser2.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_parser2.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");
	require_once("mysqlnd_ms_table_parser.inc");

	/* TODO: not sure if we can make non-table and table filter behaviour identical! */

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno())
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	run_query(2, $link, "SELECT 1", MYSQLND_MS_SLAVE_SWITCH);
	$slave_thread_id = $link->thread_id;

	fetch_result(4, run_query(3, $link, "SELECT"));
	if ($slave_thread_id != $link->thread_id)
		printf("[005] Statement has not been executed on the slave\n");

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_table_parser2.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_parser2.ini'.\n");
?>
--EXPECTF--
[003] [1064] %s
[004] No result
done!