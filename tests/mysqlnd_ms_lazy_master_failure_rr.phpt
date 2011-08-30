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
if ($error = create_config("test_mysqlnd_ms_lazy_master_failure_rr.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_lazy_master_failure_rr.ini
mysqlnd_ms.collect_statistics=1
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$connections = array();
	echo "----\n";
	compare_stats();
	echo "----\n";

	run_query(2, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH, true, true);
	$connections[$link->thread_id] = array('master');
	echo "----\n";
	compare_stats();
	echo "----\n";

	run_query(3, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);
	$connections[$link->thread_id][] = 'slave';
	echo "----\n";
	compare_stats();
	echo "----\n";

	schnattertante(run_query(4, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role"));
	$connections[$link->thread_id][] = 'slave';
	echo "----\n";
	compare_stats();
	echo "----\n";

	schnattertante(run_query(5, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role", MYSQLND_MS_MASTER_SWITCH, true, true));
	$connections[$link->thread_id][] = 'master';
	echo "----\n";
	compare_stats();
	echo "----\n";

	foreach ($connections as $thread_id => $details) {
		printf("Connection %d -\n", $thread_id);
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
Stats lazy_connections_slave_success: 2
----
Connect error, [005] [%d] %s
----
Stats use_master: 2
Stats use_master_sql_hint: 2
Stats lazy_connections_master_failure: 2
----
Connection 0 -
... master
... master
Connection %d -
... slave
Connection %d -
... slave
done!