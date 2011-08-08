--TEST--
table filter: SELECT * FROM DUAL (no db)
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
if ($error = create_config("test_mysqlnd_ms_table_dual.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_dual.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}


	$threads = array();

	/* DUAL has not db in its metadata, where will we send it? */
	run_query(3, $link, "SELECT 1 FROM DUAL");
	$threads[$link->thread_id] = array("slave1");

	$res = run_query(4, $link, "SELECT NOW() AS _now");
	$threads[$link->thread_id][] = "slave1";

	$res = run_query(5, $link, "SELECT NOW() AS _now", MYSQLND_MS_MASTER_SWITCH);
	$threads[$link->thread_id] = array("master1");

	$res = run_query(5, $link, "SELECT NOW() AS _now", MYSQLND_MS_MASTER_SWITCH);
	$threads[$link->thread_id][] = "master1";

	foreach ($threads as $thread_id => $roles) {
		printf("%d: ", $thread_id);
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
%d: slave1, slave1,
%d: master1, master1,
done!