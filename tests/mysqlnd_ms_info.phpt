--TEST--
Thread id
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
		'pick' => array("roundrobin"),
	),
);
if ($error = create_config("test_mysqlnd_ms_info.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_info.ini
--FILE--
<?php
	require_once("connect.inc");
	$threads = array();

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (0 !== mysqli_connect_errno())
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	if (!mysqli_query($link, "DROP TABLE IF EXISTS test"))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$threads[$link->thread_id] = array('role' => 'master', 'info' => mysqli_info($link));

	if (!mysqli_query($link, "SELECT 1 FROM DUAL"))
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$threads[$link->thread_id] = array('role' => 'slave', 'info' => mysqli_info($link));

	foreach ($threads as $thread_id => $info)
		printf("%d - %s - '%s'\n", $thread_id, $info['role'], $info['info']);


	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_info.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_info.ini'.\n");
?>
--EXPECTF--
%d - master - %s
%d - slave - %s
done!
