--TEST--
table filter: no master for DROP, pick = rr
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

		'lazy_connections' => 1,
		'filters' => array(
			"table" => array(
				"rules" => array(
					$db . "%" => array(
						"master" => array("master1"),
						"slave" => array("slave1"),
					),
				),
			),

			"roundrobin" => array(),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_table_master_no_match_rr.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_master_no_match_rr.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	/* shall use host = forced_master_hostname_abstract_name from the ini file */
	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	/* db.test -> no matching master -> no such host */
	if (!$link->query("DROP TABLE IF EXISTS test")) {
		printf("[003] Warn user about misconfiguration, [%d] %s\n", $link->errno, $link->error);
	}

	print "done!";
?>
--CLEAN--
<?php
	require_once("connect.inc");

	if (!unlink("test_mysqlnd_ms_table_master_no_match_rr.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_master_no_match_rr.ini'.\n");
?>
--EXPECTF--
[003] Warn user about misconfiguration, [%d] %s
done!