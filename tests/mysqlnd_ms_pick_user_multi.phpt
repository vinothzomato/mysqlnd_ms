--TEST--
Load Balancing: random_once (slaves)
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);

$settings = array(
	"myapp" => array(
		'filters'	=> array(
			'user_multi' => array('callback' => 'pick_server'),
		),
		'master' 	=> array($master_host),
		'slave' 	=> array("unknown"),
	),
);
if ($error = mst_create_config("test_mysqlnd_ms_pick_user_multi.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_pick_user_multi.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	if (!$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket))
		printf("[001] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n",
			$host, $user, $db, $port, $socket);

	mst_mysqli_query(2, $link, "SELECT 1 FROM DUAL");

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_pick_user_multi.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_pick_user_multi.ini'.\n");
?>
--EXPECTF--
Catchable fatal error: mysqli::query(): (mysqlnd_ms) Failed to call 'pick_server' in %s on line %d