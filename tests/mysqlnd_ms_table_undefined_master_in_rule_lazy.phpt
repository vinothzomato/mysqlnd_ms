--TEST--
table filter basics: undefined master, lazy
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
				'port' 	=> (double)$slave_port,
				'socket' 	=> $slave_socket,
			),
		),
		'lazy_connections' => 1,
		'filters' => array(
			"table" => array(
				"rules" => array(
					$db . ".test1%" => array(
						"master" => array("master2"),
						"slave" => array("slave1"),
					),
				),
			),
			"roundrobin" => array(),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_table_undefined_master_in_rule_lazy.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_undefined_master_in_rule_lazy.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	/* shall use host = forced_master_hostname_abstract_name from the ini file */
	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	} else {

		run_query(2, $link, "DROP TABLE IF EXISTS test1");
		if ($link->thread_id != 0)
		  printf("[003] Connected to some server, but which one?\n");

	  printf("[004] [%s/%d] %s\n", $link->sqlstate, $link->errno, $link->error);

	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_table_undefined_master_in_rule_lazy.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_undefined_master_in_rule_lazy.ini'.\n");
?>
--EXPECTF--
Warning: mysqli::query(): (mysqlnd_ms) Couldn't find the appropriate master connection. Something is wrong in %s on line %d
[002] [2000] (mysqlnd_ms) Couldn't find the appropriate master connection. Something is wrong
[004] [HY000/2000] (mysqlnd_ms) Couldn't find the appropriate master connection. Something is wrong
done!
