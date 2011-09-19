--TEST--
table filter: rule evaluation order
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_check_feature(array("table_filter"));
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
			"master2" => array(
				"host" => "unknown_master_i_hope",
			),
		),

		'slave' => array(
			"slave1" => array(
				'host' 	=> $slave_host_only,
				'port' 	=> (int)$slave_port,
				'socket' => $slave_socket,
			),
			"slave2" => array(
				"host" => "unknown_slave_i_hope",
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
if ($error = mst_create_config("test_mysqlnd_ms_table_evaluation_order_qualified_first.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

include_once("util.inc");
msg_mysqli_init_emulated_id_skip($slave_host, $user, $passwd, $db, $slave_port, $slave_socket, "slave[1]");
msg_mysqli_init_emulated_id_skip($master_host, $user, $passwd, $db, $master_port, $master_socket, "master[1]");

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_evaluation_order_qualified_first.ini
mysqlnd_ms.multi_master=1
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}


	$threads = array();

	/* db.test -> db.test rule -> master 1 */
	mst_mysqli_query(3, $link, "DROP TABLE IF EXISTS test");
	$threads[mst_mysqli_get_emulated_id(4, $link)] = array("master1");

	/* db.test2 -> % rule -> master 2 */
	mst_mysqli_query(5, $link, "DROP TABLE IF EXISTS test2");
	$threads[$link->thread_id] = array("master2");

	/* db.test -> db.test rule -> slave 2 */
	if ($res = mst_mysqli_query(5, $link, "SELECT 1 FROM test"))
		var_dump($res->fetch_assoc());
	$threads[$link->thread_id][] = 'slave2';

	foreach ($threads as $server_id => $roles) {
		printf("%s: ", $server_id);
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
%Aonnect error, [004] [%d] %s
%Aonnect error, [005] [%d] %s
%s: master1,
0: master2,slave2,
done!