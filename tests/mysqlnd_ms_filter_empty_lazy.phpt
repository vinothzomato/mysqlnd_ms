--TEST--
Empty filters section, lazy = 1
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

		'lazy_connections' => 1,
		'filters' => array(
		),
	),

);
if ($error = mst_create_config("test_mysqlnd_ms_filter_empty_lazy.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_filter_empty_lazy.ini
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
	$threads[$link->thread_id] = array("master");

	$res = mst_mysqli_query(3, $link, "SELECT 1 FROM DUAL");
	$threads[$link->thread_id] = array("slave");
	if (!$res)
		printf("[004] [%d] %s\n", $link->errno, $link->error);

	$res = mst_mysqli_query(5, $link, "SELECT 1 FROM DUAL");
	$threads[$link->thread_id][] = "slave";
	if (!$res)
		printf("[006] [%d] %s\n", $link->errno, $link->error);


	foreach ($threads as $id => $roles) {
		printf("%d: ", $id);
		foreach ($roles as $role)
		  printf("%s\n", $role);
	}


	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_filter_empty_lazy.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_filter_empty_lazy.ini'.\n");
?>
--EXPECTF--
%d: master
%d: slave
slave
done!