--TEST--
Slaves configured
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"name_of_a_config_section" => array(
		'master' => array($master_host),
	),
);
if ($error = create_config("test_mysqlnd_ms_no_slaves.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_no_slaves.ini
--FILE--
<?php
	require_once("connect.inc");

	$link = my_mysqli_connect("name_of_a_config_section", $user, $passwd, $db, $port, $socket);
	if (0 !== mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	} else {
	  if (!($res = $link->query("SELECT 'Who runs this?'"))) {
		  printf("[002] [%d] %s\n", $link->errno, $link->error);
	  } else {
		  $row = $res->fetch_row();
		  printf("[003] %s\n", $row[0]);
	  }
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_no_slaves.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_no_slaves.ini'.\n");
?>
--EXPECTF--
Warning: mysqli_real_connect(): (mysqlnd_ms) Cannot find slave section in config in %s on line %d

Warning: mysqli_real_connect(): (mysqlnd_ms) Error while connecting to the slaves in %s on line %d

Warning: mysqli_real_connect(): (HY000/2000): (mysqlnd_ms) Error while connecting to the slaves in %s on line %d
[001] [2000] (mysqlnd_ms) Error while connecting to the slaves
done!