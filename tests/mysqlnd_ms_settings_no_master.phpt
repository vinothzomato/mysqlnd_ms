--TEST--
No master
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

$settings = array(
	"myapp" => array(

		'slave' => array(
			array(
			  'port' 	=> (double)$slave_port,
			  'socket' 	=> $slave_socket,
			  'db'		=> $db,
			  'user'	=> $user,
			  'password'=> $passwd,
			),
		),
		'pick' => 'roundrobin',
		'lazy_connections' => 0,
	),
);
if ($error = create_config("test_mysqlnd_ms_settings_no_host.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_settings_no_host.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	set_error_handler('my_error_handler');

	/* note that user etc are to be taken from the config! */
	if (!($link = my_mysqli_connect("myapp", NULL, NULL, NULL, NULL, NULL)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_settings_no_host.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_settings_no_host.ini'.\n");
?>
--EXPECTF--
Fatal error: mysqli_real_connect(): (mysqlnd_ms) Section [master] doesn't exist for host [myapp] in %s on line %d