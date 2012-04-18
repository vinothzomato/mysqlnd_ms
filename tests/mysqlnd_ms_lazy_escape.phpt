--TEST--
lazy connections and string escaping
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

$settings = array(
	"myapp" => array(
		'master' => array(
			"master1" => array(
				'host' 		=> $master_host_only,
				'port' 		=> (int)$master_port,
				'socket' 	=> $master_socket,
			),
		),

		'slave' => array(
			"slave1" => array(
				'host' 	=> $slave_host_only,
				'port' 	=> (int)$slave_port,
				'socket' => $slave_socket,
			),
		 ),

		'lazy_connections' => 1,
		'filters' => array(
			"random" => array('sticky' => '1'),
		),
	),

);
if ($error = mst_create_config("test_mysqlnd_ms_lazy_escape.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_lazy_escape.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	var_dump($link->real_escape_string("test"));
	printf("[003] [%d/%s] %s\n", $link->errno, $link->sqlstate, $link->error);

	print "done!";
?>
--CLEAN--
<?php
	require_once("connect.inc");

	if (!unlink("test_mysqlnd_ms_lazy_escape.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_lazy_escape.ini'.\n");
?>
--EXPECTF--
Warning: mysqli::real_escape_string(): (mysqlnd_ms) string escaping doesn't work without established connection. Possible solution is to add offline_server_charset to your configuration in %s on line %d
string(0) ""
[003] [2014/HY000] (mysqlnd_ms) string escaping doesn't work without established connection. Possible solution is to add offline_server_charset to your configuration
done!