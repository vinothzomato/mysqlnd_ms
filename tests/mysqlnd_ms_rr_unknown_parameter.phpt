--TEST--
LB round robin: unknown parameter
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
			"slave2" => array(
				'host' 	=> $slave_host_only,
				'port' 	=> (int)$slave_port,
				'socket' => $slave_socket,
			),
		 ),
		'lazy_connections' => 1,
		'filters' => array(
			"roundrobin" => array(
				"please" => "warn me",
				"sticky" => 1,
				"" => "please",
				"\n" => "SOS"
			),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_rr_unknown_parameter.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_rr_unknown_parameter.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	run_query(2, $link, "DROP TABLE IF EXISTS test");
	if ($link->thread_id == 0)
		printf("[003] Which server has run this?");

	$last_thread_id = NULL;
	for ($i = 0; $i < 10; $i++) {
		run_query(5, $link, "SELECT 1 FROM DUAL");
		if (!is_null($last_thread_id) && ($last_thread_id == $link->thread_id))
			printf("[006] Connection has not been switched!\n");

		$last_thread_id = $link->thread_id;
	}

	if ($link->thread_id == 0)
		printf("[007] Which server has run this?");

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_rr_unknown_parameter.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_rr_unknown_parameter.ini'.\n");
?>
--EXPECTF--
done!