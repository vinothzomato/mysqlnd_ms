--TEST--
table filter: slave referencing master
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
					  "master" => array("master1"),
					  "slave" => array("master1"),
					),
				),
			),
			"random" => array(),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_table_slave_ref_master_random.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_slave_ref_master_random.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	/* valid config or not? */
	verbose_run_query(2, $link, "DROP TABLE IF EXISTS test");
	verbose_run_query(3, $link, "CREATE TABLE test(id INT)");
	verbose_run_query(4, $link, "SELECT * FROM test");

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_table_slave_ref_master_random.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_slave_ref_master_random.ini'.\n");
?>
--EXPECTF--
[002 + 01] Query 'DROP TABLE IF EXISTS test'
[002 + 02] Thread '%d'
[003 + 01] Query 'CREATE TABLE test(id INT)'
[003 + 02] Thread '%d'
[004 + 01] Query 'SELECT * FROM test'

Fatal error: mysqli::query(): (mysqlnd_ms) Couldn't find the appropriate slave connection. 0 slaves to choose from. Something is wrong in %s on line %d