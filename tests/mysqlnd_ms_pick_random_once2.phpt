--TEST--
Load Balancing: random_once (slaves)
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

$settings = array(
	"myapp" => array(
		'pick'		=> array('random' => array('sticky' => true)),
		'master' 	=> array($master_host),
		'slave' 	=> array($slave_host, $slave_host),
	),
);
if ($error = mst_create_config("test_mysqlnd_ms_pick_random_once2.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

include_once("util.inc");
msg_mysqli_init_emulated_id_skip($slave_host, $user, $passwd, $db, $slave_port, $slave_socket, "slave[1,2]");
msg_mysqli_init_emulated_id_skip($master_host, $user, $passwd, $db, $master_port, $master_socket, "master");
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_pick_random_once2.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	if (!($link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());


	/* first master */
	mst_mysqli_query(2, $link, "SET @myrole = 'Master 1'", MYSQLND_MS_MASTER_SWITCH);
	$master = mst_mysqli_get_emulated_id(3, $link);

	$slaves = array();
	$num_queries = 100;
	for ($i = 0; $i <= $num_queries; $i++) {
		mst_mysqli_query(4, $link, "SELECT 1");
		$id = mst_mysqli_get_emulated_id(5, $link);
		if (!isset($slaves[$id])) {
			$slaves[$id] = array('role' => sprintf("Slave %d", count($slaves) + 1), 'queries' => 0);
		} else {
			$slaves[$id]['queries']++;
		}
	}

	foreach ($slaves as $thread => $details) {
		printf("%s (%s) has run %d queries.\n", $details['role'], $thread, $details['queries']);
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_pick_random_once2.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_pick_random_once2.ini'.\n");
?>
--EXPECTF--
Slave 1 (%s) has run 100 queries.
done!