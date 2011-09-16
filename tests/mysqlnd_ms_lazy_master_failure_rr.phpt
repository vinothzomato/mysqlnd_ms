--TEST--
Lazy connect, master failure, roundrobin
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

if (($master_host == $slave_host)) {
	die("SKIP master and slave seem to the the same, see tests/README");
}

$settings = array(
	"myapp" => array(
		'master' => array("unreachable:6033"),
		'slave' => array($slave_host, $slave_host, $slave_host),
		'pick' 	=> array('roundrobin'),
		'lazy_connections' => 1
	),
);
if ($error = mst_create_config("test_mysqlnd_ms_lazy_master_failure_rr.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

include_once("util.inc");
msg_mysqli_init_emulated_id_skip($slave_host, $user, $passwd, $db, $slave_port, $slave_socket, "slave[1,2,3]");
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_lazy_master_failure_rr.ini
mysqlnd_ms.collect_statistics=1
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	if (!($link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$connections = array();
	echo "----\n";
	mst_compare_stats();
	echo "----\n";

	mst_mysqli_query(2, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH, true, true);
	$connections[$link->thread_id] = array('master');
	echo "----\n";
	mst_compare_stats();
	echo "----\n";

	mst_mysqli_query(3, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);	
	echo "----\n";
	mst_compare_stats();
	$connections[mst_mysqli_get_emulated_id(4, $link)][] = 'slave';
	echo "----\n";

	mst_mysqli_fech_role(mst_mysqli_query(5, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role"));	
	echo "----\n";
	mst_compare_stats();
	$connections[mst_mysqli_get_emulated_id(6, $link)][] = 'slave';
	echo "----\n";

	mst_mysqli_fech_role(mst_mysqli_query(7, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role", MYSQLND_MS_MASTER_SWITCH, true, true));
	$connections[$link->thread_id][] = 'master';
	echo "----\n";
	mst_compare_stats();
	echo "----\n";

	foreach ($connections as $thread_id => $details) {
		printf("Connection %s -\n", $thread_id);
		foreach ($details as $msg)
		  printf("... %s\n", $msg);
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_lazy_master_failure_rr.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_lazy_master_failure_rr.ini'.\n");
?>
--EXPECTF--
----
----
Connect error, [002] [%d] %s
----
Stats use_master: 1
Stats use_master_sql_hint: 1
Stats lazy_connections_master_failure: 1
----
----
Stats use_slave: 1
Stats use_slave_sql_hint: 1
Stats lazy_connections_slave_success: 1
----
This is '' speaking
----
Stats use_slave: 2
Stats use_slave_guess: 1
Stats use_last_used_sql_hint: 1
Stats lazy_connections_slave_success: 2
----
Connect error, [007] [%d] %s
----
Stats use_master: 2
Stats use_master_sql_hint: 2
Stats use_last_used_sql_hint: 2
Stats lazy_connections_master_failure: 2
----
Connection 0 -
... master
... master
Connection slave[1,2,3]-%d -
... slave
Connection slave[1,2,3]-%d -
... slave
done!
