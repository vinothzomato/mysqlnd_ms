--TEST--
mysqlnd connection statistics
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

if (($emulated_master_host == $emulated_slave_host)) {
	die("SKIP master and slave seem to the the same, see tests/README");
}

_skipif_check_extensions(array("mysqli"));
_skipif_connect($emulated_master_host_only, $user, $passwd, $db, $emulated_master_port, $emulated_master_socket);
_skipif_connect($emulated_slave_host_only, $user, $passwd, $db, $emulated_slave_port, $emulated_slave_socket);

include_once("util.inc");
$ret = mst_is_slave_of($emulated_slave_host_only, $emulated_slave_port, $emulated_slave_socket, $emulated_master_host_only, $emulated_master_port, $emulated_master_socket, $user, $passwd, $db);
if (is_string($ret))
	die(sprintf("SKIP Failed to check relation of configured master and slave, %s\n", $ret));

if (true == $ret)
	die("SKIP Configured emulated master and emulated slave could be part of a replication cluster\n");

$settings = array(
	"myapp" => array(
		'master' => array($emulated_master_host),
		'slave' => array($emulated_slave_host, $emulated_slave_host),
		'pick' => 'roundrobin',
		'lazy_connections' => 1,

	),
);
if ($error = mst_create_config("test_mysqlnd_ms_connection_stats.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

msg_mysqli_init_emulated_id_skip($emulated_slave_host, $user, $passwd, $db, $emulated_slave_port, $emulated_slave_socket, "slave[1,2]");
msg_mysqli_init_emulated_id_skip($emulated_master_host, $user, $passwd, $db, $emulated_master_port, $emulated_master_socket, "master");
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.config_file=test_mysqlnd_ms_connection_stats.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	if (!($link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$threads = array();

	/* slave 1 */
	mst_mysqli_query(2, $link, "SELECT 1 AS _one FROM DUAL");
	$stats = mysqli_get_connection_stats($link);
	$bytes = $stats['bytes_sent'];
	$threads[mst_mysqli_get_emulated_id(3, $link)] = array('role' => 'Slave 1', 'bytes' => $bytes, 'com_query' => $stats['com_query']);

	/* slave 2 */
	mst_mysqli_query(4, $link, "SELECT 12 AS _one FROM DUAL");
	$stats = mysqli_get_connection_stats($link);
	$bytes_new = $stats['bytes_sent'];
	if ($bytes_new <= $bytes)
	  printf("[005] Expecting bytes_sent >= %d, got %d\n", $bytes, $bytes_new);
	$bytes = $bytes_new;
	$threads[mst_mysqli_get_emulated_id(6, $link)] = array('role' => 'Slave 2', 'bytes' => $bytes, 'com_query' => $stats['com_query']);

	/* master */
	mst_mysqli_query(7, $link, "SELECT 123 AS _one FROM DUAL", MYSQLND_MS_MASTER_SWITCH);
	$stats = mysqli_get_connection_stats($link);
	$bytes_new = $stats['bytes_sent'];
	if ($bytes_new <= $bytes)
	  printf("[008] Expecting bytes_sent >= %d, got %d\n", $bytes, $bytes_new);
	$bytes = $bytes_new;

	$threads[mst_mysqli_get_emulated_id(9, $link)] = array('role' => 'Master', 'bytes' => $bytes, 'com_query' => $stats['com_query']);

	foreach ($threads as $thread_id => $details)
		printf("%s - %s: %d bytes, %d queries\n", $thread_id, $details['role'], $details['bytes'], $details['com_query']);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_connection_stats.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_connection_stats.ini'.\n");
?>
--EXPECTF--
slave[1,2]-%d - Slave 1: %d bytes, 1 queries
slave[1,2]-%d - Slave 2: %d bytes, 1 queries
master-%d - Master: %d bytes, 1 queries
done!