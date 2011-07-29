--TEST--
table filter: rule evaluation order
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
			"slave2" => array(
				"host" => "unknown_i_hope",
			),
		 ),

		'lazy_connections' => 1,
		'filters' => array(
			"table" => array(
				"rules" => array(
					$db . ".test" => array(
						"master"=> array("master1"),
						"slave" => array("slave2"),
					),

					"%" => array(
						"master"=> array("master2"),
						"slave" => array("slave1"),
					),
				),
			),

			"random" => array('sticky' => '1'),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_table_evaluation_order_qualified_first.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_evaluation_order_qualified_first.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}


	$threads = array();

	/* db.test -> db.test rule -> master 1 */
	run_query(3, $link, "DROP TABLE IF EXISTS test");
	$threads[$link->thread_id] = array("master1");

	/* db.test2 -> % rule -> master 2 */
	run_query(4, $link, "DROP TABLE IF EXISTS test2");
	$threads[$link->thread_id] = array("master2");

	/* db.test -> db.test rule -> slave 2 */
	if ($res = run_query(5, $link, "SELECT 1 FROM test"))
		var_dump($res->fetch_assoc());
	$threads[$link->thread_id] = array('slave2');

	foreach ($threads as $thread_id => $roles) {
		printf("%d: ", $thread_id);
		foreach ($roles as $k => $role)
		  printf("%s,", $role);
		printf("\n");
	}

	print "done!";
?>
--CLEAN--
<?php
	require_once("connect.inc");

	if (!unlink("test_mysqlnd_ms_table_evaluation_order_qualified_first.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_evaluation_order_qualified_first.ini'.\n");
?>
--EXPECTF--
[004] [%d] %s Some error, DROP TABLE db.test2 -> master2
[005] [%d] %s Some error, SELECT db.test -> slave2
%d: master1,
0: master2,
0: slave2,
done!