--TEST--
Wrong port (9999)
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

if (($emulated_master_socket && $emulated_slave_socket) ||
	(9999 == $emulated_master_port) || (9999 == $emulated_slave_port))
	die("SKIP Socket connections used, can't test for port, will not be used");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($emulated_master_host_only, $user, $passwd, $db, $emulated_master_port, $emulated_master_socket);
_skipif_connect($emulated_slave_host_only, $user, $passwd, $db, $emulated_slave_port, $emulated_slave_socket);


$settings = array(
	"myapp" => array(
		'master' => array(
			  array(
				'host' 		=> $emulated_master_host_only,
				'port' 		=> 9999,
				'socket' 	=> $emulated_master_socket,
				'db'		=> $db,
				'user'		=> $user,
				'password'	=> $passwd,
			  ),
		),
		'slave' => array(
			array(
			  'host' 	=> $emulated_slave_host_only,
			  'port' 	=> 9999,
			  'socket' 	=> $emulated_slave_socket,
			  'db'		=> $db,
			  'user'	=> $user,
			  'password'=> $passwd,
			),
		),
		'pick' => 'roundrobin',
		'lazy_connections' => 0,
	),
);
if ($error = mst_create_config("test_mysqlnd_ms_settings_port_wrong.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.config_file=test_mysqlnd_ms_settings_port_wrong.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	/* note that user etc are to be taken from the config! */
	if (!($link = mysqli_connect("myapp")))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	if ($emulated_slave_socket) {
		$res = mst_mysqli_query(2, $link, "SELECT 1 AS _one FROM DUAL", MYSQLND_MS_MASTER_SWITCH);
	} else {
		$res = mst_mysqli_query(2, $link, "SELECT 1 AS _one FROM DUAL");
	}
	var_dump($res->fetch_assoc());

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_settings_port_wrong.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_settings_port_wrong.ini'.\n");
?>
--EXPECTF--
Warning: mysqli_connect(): (mysqlnd_ms) Cannot connect to %s in %s on line %d

Warning: mysqli_connect(): (mysqlnd_ms) Error while connecting to the slaves in %s on line %d
%A