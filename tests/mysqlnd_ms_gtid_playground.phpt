--TEST--
Global Transaction Injection
--SKIPIF--
<?php
if (version_compare(PHP_VERSION, '5.3.99-dev', '<'))
	die(sprintf("SKIP Requires PHP >= 5.3.99, using " . PHP_VERSION));

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

		'global_transaction_id_injection' => array(
			'on_commit'	 				=> "UPDATE test.trx SET trx_id = trx_id + 1",
			'set_on_slave'				=> true,
			'report_error'				=> true,
			'use_multi_statement'		=> true,
		),

		'lazy_connections' => 1,
		'trx_stickiness' => 'disabled',
		'filters' => array(
			"roundrobin" => array(),
		),
	),

);
if ($error = mst_create_config("test_mysqlnd_ms_gtid_playground.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

include_once("util.inc");
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_gtid_playground.ini
mysqlnd.debug=d:t:O,/tmp/mysqlnd.trace
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}


/*
printf("...Master\n");
	$link->query("SET @myrole='master'");
$link->query("SET @myrole='master'");
var_dump($link->error);
*/
printf("...Slave\n");
	$res = $link->query("SELECT 1 FROM DUAL");
	var_dump($link->error);
	var_dump($res->fetch_assoc());
	var_dump($link->thread_id);
printf("...Slave\n");
$res = $link->query("SELECT 1 FROM DUAL");
var_dump($link->error);
var_dump($res->fetch_assoc());
var_dump($link->thread_id);
printf("...Slave\n");
	$res = $link->query("SELECT 1 FROM DUAL");
	var_dump($link->error);
	var_dump($res->fetch_assoc());
	var_dump($link->thread_id);

printf("...Master\n");
	$link->query("SET @myrole='master'");
var_dump($link->error);
var_dump($link->thread_id);
$link->commit();
var_dump($link->error);
var_dump($link->thread_id);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_gtid_playground.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_gtid_playground.ini'.\n");
?>
--XFAIL--
Playground
--EXPECTF--
string(0) ""
array(1) {
  ["@myrole"]=>
  NULL
}
string(0) ""
array(1) {
  [1]=>
  string(1) "1"
}
string(0) ""
string(0) ""
done!