--TEST--
table filter: master referencing slave (rr)
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

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
		'lazy_connections' => 0,
		'filters' => array(
			"table" => array(
				"rules" => array(
					$db . ".%" => array(
					  "master" => array("slave1"),
					  "slave" => array("slave1"),
					),
				),
			),
			"roundrobin" => array(),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_table_master_ref_slave_rr.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_master_ref_slave_rr.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	/* valid config or not? */
	verbose_run_query(2, $link, "DROP TABLE IF EXISTS test", MYSQLND_MS_SLAVE_SWITCH);
	verbose_run_query(3, $link, "DROP TABLE IF EXISTS test");
	verbose_run_query(4, $link, "CREATE TABLE test(id INT)");
	verbose_run_query(5, $link, "SELECT * FROM test");
	verbose_run_query(6, $link, "INSERT INTO test(id) VALUES (1)");
	$res = verbose_run_query(7, $link, "SELECT * FROM test");
	if ($res) {
		printf("[008] Who has stored the table ?!");
		var_dump($res->fetch_assoc());
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_table_master_ref_slave_rr.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_master_ref_slave_rr.ini'.\n");
?>
--EXPECTF--
[002 + 01] Query 'DROP TABLE IF EXISTS test'
[002 + 02] Thread '%d'
[003 + 01] Query 'DROP TABLE IF EXISTS test'

Warning: mysqli::query(): (mysqlnd_ms) Couldn't find the appropriate master connection. Something is wrong in %s on line %d
[003] [2000] (mysqlnd_ms) Couldn't find the appropriate master connection. Something is wrong
[003 + 02] Thread '%d'
[004 + 01] Query 'CREATE TABLE test(id INT)'

Warning: mysqli::query(): (mysqlnd_ms) Couldn't find the appropriate master connection. Something is wrong in %s on line %d
[004] [2000] (mysqlnd_ms) Couldn't find the appropriate master connection. Something is wrong
[004 + 02] Thread '%d'
[005 + 01] Query 'SELECT * FROM test'
[005 + 02] Thread '%d'
[006 + 01] Query 'INSERT INTO test(id) VALUES (1)'

Warning: mysqli::query(): (mysqlnd_ms) Couldn't find the appropriate master connection. Something is wrong in %s on line %d
[006 + 02] Thread '%d'
[007 + 01] Query 'SELECT * FROM test'
[007 + 02] Thread '%d'
done!