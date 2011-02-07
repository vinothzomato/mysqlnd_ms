--TEST--
mysqlnd_ms.force_config_usage
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

$settings = array(
	"app_force_config" => array(
		'master' => array('forced_master_hostname'),
		'slave' => array('forced_slave_hostname'),
	),
);
if ($error = create_config("mysqlnd_ms_ini_force_config_phpt.ini", $settings))
  die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.force_config_usage="app_force_config"
mysqlnd_ms.ini_file="mysqlnd_ms_ini_force_config_phpt.ini"
--FILE--
<?php
	require_once("connect.inc");
	$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("mysqlnd_ms_ini_force_config_phpt.ini"))
	  printf("[clean] Cannot unlink ini file 'mysqlnd_ms_ini_force_config_phpt.ini'.\n");
?>
--EXPECTF--
done!
