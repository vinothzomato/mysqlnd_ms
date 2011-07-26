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
				"host" => $master_host_only,
			),
			"master2" => array(
				"host" => "unknown_i_hope",
			),
		),

		'slave' => array(
			"slave1" => array(
				'host' => $slave_host_only,
			),
			"slave2" => array(
				"host" => "unknown_i_hope",
			),
		 ),

		'lazy_connections' => 1,
		'filters' => array(
			"table" => array(
				"rules" => array(
					"%" => array(
						"master"=> array("master2"),
						"slave" => array("slave1"),
					),
				),

				"rules" => array(
					$db . ".test" => array(
						"master"=> array("master1"),
						"slave" => array("slave2"),
					),
				),
			),

			"random" => array('sticky' => '1'),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_table_evaluation_order.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_evaluation_order.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	create_test_table($slave_host_only, $user, $passwd, $db, $port, $socket);

	/* shall use host = forced_master_hostname_abstract_name from the ini file */
	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	/* match all rule at the beginning -> master 2 -> error */
	$threads = array();
	if (!@mysqli_query($link, "DROP TABLE IF EXISTS test")) {
		if (isset($connect_errno_codes[$link->errno])) {
		  printf("Expected error, ");
		}
		printf("[%d] %s\n", $link->errno, $link->error);
	}
	$threads[$link->thread_id] = array("master2");

	/* db.test but match all rule at the beginning -> slave 1 -> no error  */
	if ($res = run_query(4, $link, "SELECT 1 FROM test"))
		var_dump($res->fetch_assoc());
	$threads[$link->thread_id] = array('slave1');

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

	if (!unlink("test_mysqlnd_ms_table_evaluation_order.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_evaluation_order.ini'.\n");
?>
--EXPECTF--
[%d] %s Some error, % shall match DROP TABLE and try to use master2 which does not exist.
array(1) {
  [1]=>
  string(1) "1"
}
0: master2,
%d: slave1,
done!