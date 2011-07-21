--TEST--
Empty filters section, lazy = 0
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array(
			"master1" => array(
				"host" => $master_host_only,
			),
		),
		'slave' => array(

			"slave1" => array(
				'host' => $slave_host_only,
			),

		 ),

		'lazy_connections' => 0,
		'filters' => array(
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_filter_empty_non_lazy.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_filter_empty_non_lazy.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	/* shall use host = forced_master_hostname_abstract_name from the ini file */
	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	$threads = array();

	run_query(2, $link, "DROP TABLE IF EXISTS test");
	$threads[$link->thread_id] = array("master");

	$res = run_query(3, $link, "SELECT 1 FROM DUAL");
	$threads[$link->thread_id] = array("slave");
	if (!$res)
		printf("[004] [%d] %s\n", $link->errno, $link->error);

	foreach ($threads as $id => $roles) {
		printf("%d: ", $id);
		foreach ($roles as $role)
		  printf("%s\n", $role);
	}


	print "done!";
?>
--CLEAN--
<?php
//	if (!unlink("test_mysqlnd_ms_filter_empty_non_lazy.ini"))
//	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_filter_empty_non_lazy.ini'.\n");
?>
--EXPECTF--
%d:
master
%d:
slave
done!