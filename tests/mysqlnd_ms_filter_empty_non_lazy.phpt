--TEST--
Empty filters section, lazy = 0
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
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
		),
	),

);
if ($error = mst_create_config("test_mysqlnd_ms_filter_empty_non_lazy.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

include_once("util.inc");
msg_mysqli_init_emulated_id_skip($slave_host, $user, $passwd, $db, $slave_port, $slave_socket, "slave[1,2]");
msg_mysqli_init_emulated_id_skip($master_host, $user, $passwd, $db, $master_port, $master_socket, "master");
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_filter_empty_non_lazy.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	/* shall use host = forced_master_hostname_abstract_name from the ini file */
	$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	$threads = array();

	mst_mysqli_query(2, $link, "DROP TABLE IF EXISTS test");
	$threads[mst_mysqli_get_emulated_id(3, $link)] = array("master");

	$res = mst_mysqli_query(4, $link, "SELECT 1 FROM DUAL");
	$threads[mst_mysqli_get_emulated_id(5, $link)] = array("slave");
	if (!$res)
		printf("[006] [%d] %s\n", $link->errno, $link->error);

	$res = mst_mysqli_query(7, $link, "SELECT 1 FROM DUAL");
	$threads[mst_mysqli_get_emulated_id(8, $link)][] = "slave";
	if (!$res)
		printf("[009] [%d] %s\n", $link->errno, $link->error);

	foreach ($threads as $id => $roles) {
		printf("%s: ", $id);
		foreach ($roles as $role)
		  printf("%s\n", $role);
	}


	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_filter_empty_non_lazy.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_filter_empty_non_lazy.ini'.\n");
?>
--EXPECTF--
master-%d: master
slave[1,2]-%d: slave
slave
done!