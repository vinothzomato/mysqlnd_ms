--TEST--
table filter: SELECT * FROM DUAL (no db)
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");
if (($master_host == $slave_host)) {
	die("SKIP master and slave seem to the the same, see tests/README");
}

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
			"master2" => array(
				"host" => "unknown_master_i_hope",
			),
		),

		'slave' => array(
			"slave1" => array(
				'host' 	=> $slave_host_only,
				'port' 	=> (int)$slave_port,
				'socket' => $slave_socket,
			),
			"slave2" => array(
				"host" => "unknown_slave_i_hope",
			),
		 ),

		'lazy_connections' => 1,
		'filters' => array(
			"table" => array(
				"rules" => array(
					$db . ".%" => array(
						"master"=> array("master1"),
						"slave" => array("slave1"),
					),
				),
			),

			"roundrobin" => array(),
		),
	),

);
if ($error = mst_create_config("test_mysqlnd_ms_table_dual.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

include_once("util.inc");
msg_mysqli_init_emulated_id_skip($slave_host, $user, $passwd, $db, $slave_port, $slave_socket, "slave[1]");
msg_mysqli_init_emulated_id_skip($master_host, $user, $passwd, $db, $master_port, $master_socket, "master[1]");
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_dual.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}


	$threads = array();

	/* DUAL has not db in its metadata, where will we send it? */
	mst_mysqli_query(3, $link, "SELECT 1 FROM DUAL");
	$threads[mst_mysqli_get_emulated_id(4, $link)] = array("slave1");

	$res = mst_mysqli_query(5, $link, "SELECT NOW() AS _now");
	$threads[mst_mysqli_get_emulated_id(6, $link)][] = "slave1";

	$res = mst_mysqli_query(7, $link, "SELECT NOW() AS _now", MYSQLND_MS_MASTER_SWITCH);
	$threads[mst_mysqli_get_emulated_id(8, $link)] = array("master1");

	$res = mst_mysqli_query(9, $link, "SELECT NOW() AS _now", MYSQLND_MS_MASTER_SWITCH);
	$threads[mst_mysqli_get_emulated_id(10, $link)][] = "master1";

	foreach ($threads as $server_id => $roles) {
		printf("%s: ", $server_id);
		foreach ($roles as $k => $role)
		  printf("%s,", $role);
		printf("\n");
	}

	print "done!";
?>
--CLEAN--
<?php
	require_once("connect.inc");

	if (!unlink("test_mysqlnd_ms_table_dual.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_dual.ini'.\n");
?>
--EXPECTF--
%s: slave1, slave1,
%s: master1, master1,
done!