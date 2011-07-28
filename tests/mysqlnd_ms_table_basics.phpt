--TEST--
table filter basics
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
			"master2" => array(
				"host" => "unknown_i_hope",
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
			"table" => array(
				"rules" => array(
					$db . ".test%" => array(
						"master" => array("master2"),
						"slave" => array("slave1"),
					),
				),
			),

			"roundrobin" => array(),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_table_basics.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_basics.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	/* shall use host = forced_master_hostname_abstract_name from the ini file */
	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	/* db.ulf -> master 1 */
	run_query(3, $link, "DROP TABLE IF EXISTS ulf");

	/* db.test1 -> master 2 -> no such host */
	if (!$link->query("DROP TABLE IF EXISTS test1"))
		printf("[004] [%d] %s\n", $link->errno, $link->error);

	print "done!";
?>
--CLEAN--
<?php
	require_once("connect.inc");

	if (!unlink("test_mysqlnd_ms_table_basics.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_basics.ini'.\n");
?>
--EXPECTF--
[004] [%d] %s
done!