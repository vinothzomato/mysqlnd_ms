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
if ($error = create_config("test_mysqlnd_ms_pick_user_multi.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_pick_user_multi.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	if (!$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket))
		printf("[001] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n",
			$host, $user, $db, $port, $socket);

	run_query(2, $link, "SELECT 1 FROM DUAL");

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_pick_user_multi.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_pick_user_multi.ini'.\n");
?>
--EXPECTF--
Slave 1 (%d) has run 100 queries.
done!