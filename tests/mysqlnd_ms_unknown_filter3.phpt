--TEST--
Unknown filter, filtername empty string
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

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
		 ),
		'lazy_connections' => 0,
		'filters' => array(
			"random" => array("sticky" => true),
			"" => array( "is" => "not", "fair" => "!"),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_unknown_filter3.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_unknown_filter3.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	/* valid config or not? */
	run_query(2, $link, "DROP TABLE IF EXISTS test");
	if (0 == $link->thread_id)
		printf("[003] Not connected to any server.");

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_unknown_filter3.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_unknown_filter3.ini'.\n");
?>
--EXPECTF--
Catchable fatal error: mysqli_real_connect(): (mysqlnd_ms) Error loading filters. Filter with empty name found in %s on line %d