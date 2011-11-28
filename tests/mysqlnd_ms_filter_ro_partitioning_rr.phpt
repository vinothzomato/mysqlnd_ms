--TEST--
Stacking LB filters and partitioning filter
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
		),
		'slave' => array(
			"slave1" => array(
				'host' 	=> $slave_host_only,
				'port' 	=> (int)$slave_port,
				'socket' => $slave_socket,
			),
			"slave2" => array(
				'host' 	=> $slave_host_only,
				'port' 	=> (int)$slave_port,
				'socket' => $slave_socket,
			),
		 ),
		'lazy_connections' => 0,
		'filters' => array(
			"random" => array("sticky" => 1),
			"table" => array(
				"rules" => array(
					"%" => array(
					  "master" => array("master1"),
					  "slave" => array("slave1", "slave2"),
					),
				),
			),
			"roundrobin" => array(),
		),
	),

);
if ($error = mst_create_config("test_mysqlnd_ms_filter_ro_partitioning_rr.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

include_once("util.inc");
msg_mysqli_init_emulated_id_skip($slave_host, $user, $passwd, $db, $slave_port, $slave_socket, "slave[1,2]");
msg_mysqli_init_emulated_id_skip($master_host, $user, $passwd, $db, $master_port, $master_socket, "master");
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_filter_ro_partitioning_rr.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	$threads = array();
	mst_mysqli_query(2, $link, "SET @myrole='master'");
	$threads[mst_mysqli_get_emulated_id(3, $link)] = 'master';
	mst_mysqli_query(4, $link, "SET @myrole='slave1'", MYSQLND_MS_SLAVE_SWITCH);
	$threads[mst_mysqli_get_emulated_id(5, $link)] = 'slave1';
	mst_mysqli_query(6, $link, "SET @myrole='slave2'", MYSQLND_MS_SLAVE_SWITCH);
	$threads[mst_mysqli_get_emulated_id(7, $link)] = 'slave2';

	$res = mst_mysqli_query(8, $link, "SELECT @myrole AS _role");
	$row = $res->fetch_assoc();
	printf("[009] Hi folks, %s speaking.\n", $row['_role']);


	foreach ($threads as $id => $role)
		printf("%s => %s\n", $id, $role);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_filter_ro_partitioning_rr.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_filter_ro_partitioning_rr.ini'.\n");
?>
--EXPECTF--
[009] Hi folks, slave%d speaking.
master-%d => master
slave[1,2]-%d => slave%d
done!