--TEST--
Manual failover, unknown slave host
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

if (($master_host == $slave_host)) {
	die("SKIP master and slave seem to the the same, see tests/README");
}

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

$settings = array(
	"myapp" => array(
		'master' 	=> array($master_host),
		'slave' 	=> array($slave_host, "unknown", $slave_host),
		'pick' 		=> array("roundrobin"),
		'failover'	=> 'disabled',
	),
);
if ($error = mst_create_config("test_mysqlnd_ms_failover_unknown.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

include_once("util.inc");
msg_mysqli_init_emulated_id_skip($slave_host, $user, $passwd, $db, $slave_port, $slave_socket, "slave[1,2,3]");
msg_mysqli_init_emulated_id_skip($master_host, $user, $passwd, $db, $master_port, $master_socket, "master");
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_failover_unknown.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	function my_mysqli_query($offset, $link, $query) {
		global $mst_connect_errno_codes;

		if (!($res = @$link->query($query))) {
			if (isset($mst_connect_errno_codes[$link->errno]))
			  printf("[%03d + 01] Expected connect error, [%d] %s\n", $offset, $link->errno, $link->error);
			else
			  printf("[%03d + 02] Unexpected error, [%d] %s\n", $offset, $link->errno, $link->error);
			return 0;
		}

		if (!is_object($res)) {
			printf("[%03d + 04] Thread %d, %s\n", $offset, $link->thread_id, $query);
			return mst_mysqli_get_emulated_id($offset, $link);
		}

		if (!($row = $res->fetch_assoc())) {
			printf("[%03d + 04] [%d] %s\n", $offset, $link->errno, $link->error);
			return 0;
		}

		printf("[%03d + 05] Thread %d, %s\n", $offset, $link->thread_id, $row['msg']);
		return mst_mysqli_get_emulated_id($offset, $link);
	}

	$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (0 !== mysqli_connect_errno())
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$threads = array();
	$threads[my_mysqli_query(10, $link, "DROP TABLE IF EXISTS test")] = 'master';
	$threads[my_mysqli_query(20, $link, "SELECT 'Slave 1' AS msg")] = 'slave 1';
	$threads[my_mysqli_query(30, $link, "SELECT 'Slave 2' AS msg")] = 'slave 2';
	$threads[my_mysqli_query(40, $link, "SELECT 'Slave 3' AS msg")] = 'slave 3';
	$threads[my_mysqli_query(50, $link, "SELECT 'Slave 1' AS msg")] = 'slave 1';
	$threads[my_mysqli_query(60, $link, "SELECT 'Slave 2' AS msg")] = 'slave 2';
	$threads[my_mysqli_query(70, $link, "SELECT 'Slave 3' AS msg")] = 'slave 3';

	foreach ($threads as $thread_id => $role)
		printf("Thread ID %s, role %s\n", $thread_id, $role);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_failover_unknown.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_failover_unknown.ini'.\n");
?>
--EXPECTF--
[010 + 04] Thread %s, DROP TABLE IF EXISTS test
[020 + 05] Thread %s, Slave 1
[030 + 01] Expected connect error, [%d] %s
[040 + 05] Thread %s, Slave 3
[050 + 05] Thread %s, Slave 1
[060 + 01] Expected connect error, [%d] %s
[070 + 05] Thread %s, Slave 3
Thread ID %s, role master
Thread ID %s, role slave 1
Thread ID 0, role slave 2
Thread ID %s, role slave 3
done!