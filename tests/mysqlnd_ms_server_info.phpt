--TEST--
mysqli->server_info
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
		'pick' => 'roundrobin',
		'lazy_connections' => 1,
	),
);
if ($error = create_config("test_mysqlnd_ms_server_info.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_server_info.ini
--FILE--
<?php
	require_once("connect.inc");

	function run_query($offset, $link, $query, $switch = NULL) {
		if ($switch)
			$query = sprintf("/*%s*/%s", $switch, $query);

		if (!($ret = $link->query($query)))
			printf("[%03d] [%d] %s\n", $offset, $link->errno, $link->error);

		return $ret;
	}

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$threads = array();

	/* slave 1 */
	run_query(2, $link, "SELECT 1 AS _one FROM DUAL");
	$threads[$link->thread_id] = array('role' => 'Slave 1', 'info' => $link->server_info);

	/* slave 2 */
	run_query(3, $link, "SELECT 12 AS _one FROM DUAL");
	$threads[$link->thread_id] = array('role' => 'Slave 2', 'info' => $link->server_info);

	/* master */
	run_query(5, $link, "SELECT 123 AS _one FROM DUAL", MYSQLND_MS_MASTER_SWITCH);
	$threads[$link->thread_id] = array('role' => 'Master', 'info' => $link->server_info);

	foreach ($threads as $thread_id => $details) {
		printf("%d - %s: '%s'\n", $thread_id, $details['role'], $details['info']);
		if ('' == $details['info'])
			printf("Server info must not be empty!\n");
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_server_info.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_server_info.ini'.\n");
?>
--EXPECTF--
%d - Slave 1: '%s'
%d - Slave 2: '%s'
%d - Master: '%s'
done!