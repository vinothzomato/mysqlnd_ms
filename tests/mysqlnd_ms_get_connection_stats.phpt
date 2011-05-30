--TEST--
mysqlnd connection statistics
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
if ($error = create_config("test_mysqlnd_ms_connection_stats.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_connection_stats.ini
mysqlnd.debug=d:t:O,/tmp/mysqlnd.trace
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
	$stats = mysqli_get_connection_stats($link);
	$bytes = $stats['bytes_sent'];
	$threads[$link->thread_id] = array('role' => 'Slave 1', 'bytes' => $bytes, 'com_query' => $stats['com_query']);

	/* slave 2 */
	run_query(3, $link, "SELECT 12 AS _one FROM DUAL");
	$stats = mysqli_get_connection_stats($link);
	$bytes_new = $stats['bytes_sent'];
	if ($bytes_new <= $bytes)
	  printf("[004] Expecting bytes_sent >= %d, got %d\n", $bytes, $bytes_new);
	$bytes = $bytes_new;

	$threads[$link->thread_id] = array('role' => 'Slave 2', 'bytes' => $bytes, 'com_query' => $stats['com_query']);

	/* master */
	run_query(5, $link, "SELECT 123 AS _one FROM DUAL", MYSQLND_MS_MASTER_SWITCH);
	$stats = mysqli_get_connection_stats($link);
	$bytes_new = $stats['bytes_sent'];
	if ($bytes_new <= $bytes)
	  printf("[006] Expecting bytes_sent >= %d, got %d\n", $bytes, $bytes_new);
	$bytes = $bytes_new;

	$threads[$link->thread_id] = array('role' => 'Master', 'bytes' => $bytes, 'com_query' => $stats['com_query']);

	foreach ($threads as $thread_id => $details)
		printf("%d - %s: %d bytes, %d queries\n", $thread_id, $details['role'], $details['bytes'], $details['com_query']);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_connection_stats.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_connection_stats.ini'.\n");
?>
--EXPECTF--
%d - Slave 1: %d bytes, 1 queries
%d - Slave 2: %d bytes, 1 queries
%d - Master: %d bytes, 1 queries
done!