--TEST--
Per host credentials
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$obj = new stdClass();

$settings = array(
	"myapp" => array(
		'master' => array(
			  array(
				'host' 		=> (string)$master_host_only,
				'port' 		=> (int)$master_port,
				'socket' 	=> PHP_INT_MAX * 2,
				'db'		=> 1.124
				'user'		=> array($user),
				'password'	=> $obj,
			  ),
		),
		'slave' => array(
			array(
			  'host' 	=> $slave_host_only,
			  'port' 	=> (double)$slave_port,
			  'socket' 	=> "an"
			  'db'		=> false,
			  'user'	=> NULL,
			  'password'=> 123,
			),
		),
		'pick' => 'roundrobin',
		'lazy_connections' => 0,
	),
);
if ($error = create_config("test_mysqlnd_ms_settings_host_credentials_types.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_settings_host_credentials_types.ini
--FILE--
<?php
	require_once("connect.inc");

	/* note that user etc are to be taken from the config! */
	if (!($link = my_mysqli_connect("myapp", NULL, NULL, NULL, NULL, NULL)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_settings_host_credentials_types.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_settings_host_credentials_types.ini'.\n");
?>
--EXPECTF--

Warning: Unknown: failed to open stream: No such file or directory in Unknown on line 0

Warning: Unknown: (mysqlnd_ms) Failed to parse server list ini file test_mysqlnd_ms_settings_host_credentials_types.ini in Unknown on line 0

Warning: mysqli_real_connect(): [%d] %s

Warning: mysqli_real_connect(): (%s/%d): %s
[001] [%d] %s
done!