--TEST--
table filter: "numeric" pattern/key
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
			"master2" => array(
				'host'		=> 'Thursday_1546_pm_really',
			),
		),
		'slave' => array(
			"slave1" => array(
				'host' 	=> $slave_host_only,
				'port' 	=> (int)$slave_port,
				'socket' => $slave_socket,
			),
			"master2" => array(
				'host'		=> 'Thursday_1546_pm_really',
			),
		 ),
		'lazy_connections' => 0,
		'filters' => array(
			"table" => array(
				"rules" => array(
					-1.01 => array(
					  "master" => array("master2"),
					  "slave" => array("slave2"),
					),
				),
			),
			"random" => array("sticky" => true),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_table_rule_numeric.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_rule_numeric.ini
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
	if (!unlink("test_mysqlnd_ms_table_rule_numeric.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_rule_numeric.ini'.\n");
?>
--EXPECTF--
Bail about invalid config
done!