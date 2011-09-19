--TEST--
table filter basics: leak
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
				'port' 	=> (double)$slave_port,
				'socket' 	=> $slave_socket,
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
if ($error = mst_create_config("test_mysqlnd_ms_table_assorted_slave.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

include_once("util.inc");
msg_mysqli_init_emulated_id_skip($slave_host, $user, $passwd, $db, $slave_port, $slave_socket, "slave");
msg_mysqli_init_emulated_id_skip($master_host, $user, $passwd, $db, $master_port, $master_socket, "master");

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_assorted_slave.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	} else {

		mst_mysqli_query(3, $link, "DROP TABLE IF EXISTS test1");
		$master = mst_mysqli_get_emulated_id(4, $link);

		/* there is no slave to run this query... */
		if ($res = mst_mysqli_query(5, $link, "SELECT 'one' AS _id FROM test1")) {
			var_dump($res->fetch_assoc());
		}
		$server_id = mst_mysqli_get_emulated_id(6, $link);
		if ($server_id == $master)
			printf("[007] Master has replied to slave query\n");

		if (!is_null($server_id))
		  printf("[008] Connected to some server, but which one?\n");

	  printf("[009] [%s/%d] %s\n", $link->sqlstate, $link->errno, $link->error);

	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_table_assorted_slave.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_assorted_slave.ini'.\n");
?>
--EXPECTF--
[009] [HY000/2002] Some meaningful message from mysqlnd_ms, e.g. some connect error
done!