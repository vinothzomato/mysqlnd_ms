--TEST--
mysqli->server_version
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

if (($emulated_master_host == $slave_host)) {
	die("SKIP master and slave seem to the the same, see tests/README");
}

_skipif_check_extensions(array("mysqli"));
_skipif_connect($emulated_master_host_only, $user, $passwd, $db, $emulated_master_port, $emulated_master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

include_once("util.inc");
$ret = mst_is_slave_of($emulated_slave_host_only, $emulated_slave_port, $emulated_slave_socket, $emulated_master_host_only, $emulated_master_port, $emulated_master_socket, $user, $passwd, $db);
if (is_string($ret))
	die(sprintf("SKIP Failed to check relation of configured master and slave, %s\n", $ret));

if (true == $ret)
	die("SKIP Configured emulated master and emulated slave could be part of a replication cluster\n");

$settings = array(
	"myapp" => array(
		'master' => array($emulated_master_host),
		'slave' => array($slave_host, $slave_host),
		'pick' => 'roundrobin',
		'lazy_connections' => 1,
	),
);
if ($error = mst_create_config("test_mysqlnd_ms_server_version.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

include_once("util.inc");
msg_mysqli_init_emulated_id_skip($slave_host, $user, $passwd, $db, $slave_port, $slave_socket, "slave[1,2]");
msg_mysqli_init_emulated_id_skip($emulated_master_host_only, $user, $passwd, $db, $emulated_master_port, $emulated_master_socket, "master");
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.config_file=test_mysqlnd_ms_server_version.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	if (!($link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$threads = array();

	/* slave 1 */
	mst_mysqli_query(2, $link, "SELECT 1 AS _one FROM DUAL");
	$server_id = mst_mysqli_get_emulated_id(3, $link);
	$threads[$server_id] = array('role' => 'Slave 1', 'version' => $link->server_version);

	/* slave 2 */
	mst_mysqli_query(4, $link, "SELECT 12 AS _one FROM DUAL");
	$server_id = mst_mysqli_get_emulated_id(5, $link);
	$threads[$server_id] = array('role' => 'Slave 2', 'version' => $link->server_version);

	/* master */
	mst_mysqli_query(6, $link, "SELECT 123 AS _one FROM DUAL", MYSQLND_MS_MASTER_SWITCH);
	$server_id = mst_mysqli_get_emulated_id(7, $link);
	$threads[$server_id] = array('role' => 'Master', 'version' => $link->server_version);

	foreach ($threads as $server_id => $details) {
		printf("%s - %s: %d\n", $server_id, $details['role'], $details['version']);
		if ('' == $details['version'])
			printf("Server version must not be empty!\n");
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_server_version.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_server_version.ini'.\n");
?>
--EXPECTF--
%s - Slave 1: %d
%s - Slave 2: %d
%s - Master: %d
done!