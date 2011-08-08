--TEST--
Thread id
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
		'pick' => array("roundrobin"),
	),
);
if ($error = create_config("test_mysqlnd_ms_warning_count.ini", $settings))
  die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_warning_count.ini
--FILE--
<?php
	require_once("connect.inc");
	$threads = array();

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (0 !== mysqli_connect_errno())
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	if (!mysqli_query($link, "DROP TABLE IF EXISTS this_table_does_not_exist"))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (1 !== ($tmp = mysqli_warning_count($link)))
		printf("[003] Expecting warning count = 1, got %d\n", $tmp);

	$threads[$link->thread_id] = 'master (1)';

	if (!mysqli_query($link, "SELECT 1 FROM DUAL"))
		printf("[004] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (0 !== ($tmp = mysqli_warning_count($link)))
		printf("[005] Expecting warning count = 0, got %d\n", $tmp);

	 $threads[$link->thread_id] = 'slave 1';

	 if (!mysqli_query($link, "SELECT 1 FROM DUAL"))
		printf("[006] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (0 !== ($tmp = mysqli_warning_count($link)))
		printf("[007] Expecting warning count = 0, got %d\n", $tmp);

	$threads[$link->thread_id] = 'slave 2';

	if (!mysqli_query($link, "/*" . MYSQLND_MS_MASTER_SWITCH . "*/SELECT 1 FROM DUAL"))
		printf("[008] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (0 !== ($tmp = mysqli_warning_count($link)))
		printf("[009] Expecting warning count = 0, got %d\n", $tmp);

 	$threads[$link->thread_id] = 'master (2)';

	if (!mysqli_query($link, "/*" . MYSQLND_MS_SLAVE_SWITCH . "*/DROP TABLE IF EXISTS this_table_does_not_exist"))
		printf("[010] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (1 !== ($tmp = mysqli_warning_count($link)))
		printf("[011] Expecting warning count = 1, got %d\n", $tmp);

 	$threads[$link->thread_id] = 'slave 1 (2)';

	if (!mysqli_query($link, "/*" . MYSQLND_MS_SLAVE_SWITCH . "*/DROP TABLE IF EXISTS this_table_does_not_exist"))
		printf("[012] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (1 !== ($tmp = mysqli_warning_count($link)))
		printf("[013] Expecting warning count = 1, got %d\n", $tmp);

 	$threads[$link->thread_id] = 'slave 2 (2)';


	foreach ($threads as $thread_id => $label)
		printf("%d - %s\n", $thread_id, $label);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_warning_count.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_warning_count.ini'.\n");
?>
--EXPECTF--
%d - master (2)
%d - slave 1 (2)
%d - slave 2 (2)
done!
