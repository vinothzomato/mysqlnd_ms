--TEST--
parser: SELECT X'4D7953514C'
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

if ($error = create_config("test_mysqlnd_ms_table_parser22.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_parser22.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");
	require_once("mysqlnd_ms_table_parser.inc");

	$sql = "SELECT X'4D7953514C' AS _id FROM DUAL";
	if (server_supports_query(1, $sql, $slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket)) {

		$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
		if (mysqli_connect_errno())
			printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

		run_query(3, $link, "SELECT 1 FROM DUAL", MYSQLND_MS_SLAVE_SWITCH);
		$thread_id = $link->thread_id;

		fetch_result(5, run_query(4, $link, $sql));
		if ($thread_id != $link->thread_id)
			printf("[006] Statement has not been executed on the slave\n");

	} else {
		/* fake result */
		printf("[005] _id = 'MySQL'\n");
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_table_parser22.ini"))
		printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_parser22.ini'.\n");
?>
--EXPECTF--
[001] Testing server support of 'SELECT X'4D7953514C' AS _id FROM DUAL'
[005] _id = 'MySQL'
done!