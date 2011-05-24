--TEST--
No master configured
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

$settings = array(
	"name_of_a_config_section" => array(
		'slave' => array($slave_host),
	),
);
if ($error = create_config("test_mysqlnd_ms_no_master.ini", $settings))
  die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_no_master.ini
--FILE--
<?php
	require_once("connect.inc");

	$link = my_mysqli_connect("name_of_a_config_section", $user, $passwd, $db, $port, $socket);
	if (0 !== mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	} else {
	  if (!$link->query("DROP TABLE IF EXISTS test")) {
		  printf("[002] [%d] %s\n", $link->errno, $link->error);
	  }
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_no_master.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_no_master.ini'.\n");
?>
--EXPECTF--
Warning: mysqli_real_connect(): (mysqlnd_ms) Cannot find master section in config in %s on line %d

Warning: mysqli_real_connect(): (mysqlnd_ms) Error while connecting to the master(s) in %s on line %d

Warning: mysqli_real_connect(): (HY000/2002): (mysqlnd_ms) Error while connecting to the master(s) in %s on line %d
[001] [2002] (mysqlnd_ms) Error while connecting to the master(s)
done!