--TEST--
mysqli->host_info
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
		'pick' => 'roundrobin',
		'lazy_connections' => 1,
	),
);
if ($error = create_config("test_mysqlnd_ms_host_info.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_host_info.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$threads = array();

	/* slave 1 */
	run_query(2, $link, "SELECT 1 AS _one FROM DUAL");
	$threads[$link->thread_id] = array('role' => 'Slave 1', 'info' => $link->host_info);

	/* slave 2 */
	run_query(3, $link, "SELECT 12 AS _one FROM DUAL");
	$threads[$link->thread_id] = array('role' => 'Slave 2', 'info' => $link->host_info);

	/* master */
	run_query(5, $link, "SELECT 123 AS _one FROM DUAL", MYSQLND_MS_MASTER_SWITCH);
	$threads[$link->thread_id] = array('role' => 'Master', 'info' => $link->host_info);

	foreach ($threads as $thread_id => $details) {
		printf("%d - %s: '%s'\n", $thread_id, $details['role'], $details['info']);
		if ('' == $details['info'])
			printf("Host info must not be empty!\n");
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_host_info.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_host_info.ini'.\n");
?>
--EXPECTF--
%d - Slave 1: '%s'
%d - Slave 2: '%s'
%d - Master: '%s'
done!