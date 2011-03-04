--TEST--
INI setting: mysqlnd_ms.force_config_usage
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

$settings = array(
	"name_of_a_config_section" => array(
		'master' => array('forced_master_hostname_abstract_name'),
		'slave' => array('forced_slave_hostname_abstract_name'),
	),

	"192.168.14.17" => array(
		'master' => array('forced_master_hostname_ip'),
		'slave' => array('forced_slave_hostname_ip'),
	),

	"my_orginal_mysql_server_host" => array(
		'master' => array('forced_master_hostname_orgname'),
		'slave' => array('forced_slave_hostname_orgname'),
	),

);
if ($error = create_config("test_mysqlnd_ms_ini_force_config.ini", $settings))
  die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.force_config_usage=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_ini_force_config.ini
--FILE--
<?php
	require_once("connect.inc");

	/* shall use host = forced_master_hostname_abstract_name from the ini file */
	$link = @my_mysqli_connect("name_of_a_config_section", $user, $passwd, $db, $port, $socket);
	printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$link = @my_mysqli_connect("192.168.14.17", $user, $passwd, $db, $port, $socket);
	printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$link = @my_mysqli_connect("my_orginal_mysql_server_host", $user, $passwd, $db, $port, $socket);
	printf("[003] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
	if (0 == mysqli_connect_errno()) {
		/* 0 means no error. This is documented behaviour! */
		$res = $link->query("SELECT 'This connection should have been forbidden!'");
		$row = $res->fetch_row();
		printf("[004] %s\n", $row[0]);
	} else if (2000 == mysqli_connect_errno()) {
		/* Error: 2000 (CR_UNKNOWN_ERROR), HYOOO */
		printf("[005] Connection failed. The plugin can't set a specific error code as none exists, we go for unspecific code 2000 (CR_UNKNOWN_ERROR), [%d] %s\n",
			  $link->connect_errno, $link->connect_error);
	} else {
		printf("[006] [%d] %s\n", $link->connect_errno, $link->connect_error);
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_ini_force_config.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_ini_force_config.ini'.\n");
?>
--EXPECTF--
[001] [%d] %s
[002] [%d] %s
[003] [%d] %s

Warning: mysqli_real_connect(): Exclusive usage of configuration enforced but did not find the correct INI file section (%s) in %s on line %d

Warning: mysqli_real_connect(): (00000/0):  in %s on line %d

[005] Connection failed. The plugin can't set a specific error code as none exists, we go for unspecific code 2000 (CR_UNKNOWN_ERROR), [2000] %s
done!