--TEST--
PECL #22784 - mysql_select_db won't work
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysql"));
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
if ($error = create_config("test_mysqlnd_ms_bug_pecl_22784.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_bug_pecl_22784.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	$link = my_mysql_connect("myapp", $user, $passwd, NULL, $port, $socket);
	if (mysql_errno($link))
		printf("[001] [[%d] %s\n", mysql_errno($link), mysql_error($link));

	if (!mysql_select_db($db, $link));
		printf("[002] [[%d] %s\n", mysql_errno($link), mysql_error($link));

	if (!mysql_query("SELECT 1", $link))
		printf("[003] [[%d] %s\n", mysql_errno($link), mysql_error($link));

	print "done!";
?>
--CLEAN--
<?php
	require_once("connect.inc");

	if (!unlink("test_mysqlnd_ms_bug_pecl_22784.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_bug_pecl_22784.ini'.\n");
?>
--EXPECTF--
done!