--TEST--
SQL hints LAST_USED called before any server has been selected, pick = random
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host),
		'pick' => array("random"),
	),
);
if ($error = create_config("test_mysqlnd_ms_last_used_random.ini", $settings))
  die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_last_used_random.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	if (!$link->query(sprintf("/*%s*/SELECT 1", MYSQLND_MS_LAST_USED_SWITCH)))
		printf("[002] [%d][%s] %s\n", $link->errno, $link->sqlstate, $link->error);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_last_used_random.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_last_used_random.ini'.\n");
?>
--EXPECTF--
[002] [0][HYOO] Some meaningful mysqlnd ms error message
done!