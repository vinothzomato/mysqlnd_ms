--TEST--
table filter: slave rule
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
			"slave3" => array(
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
						"slave" => array("slave2"),
					),
				),
			),

			"roundrobin" => array(),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_table_slave_rr.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_slave_rr.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	/* shall use host = forced_master_hostname_abstract_name from the ini file */
	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	$threads = array();

	/* db.ulf -> master 1 */
	run_query(3, $link, "DROP TABLE IF EXISTS ulf");
	$threads[$link->thread_id] = array('master1');

	/* db.dual -> rr, slave 1 */
	run_query(4, $link, "SELECT 1 FROM DUAL");
	$threads[$link->thread_id] = array('slave1');

	/* db.test -> slave 2 -> no such host */
	if (!@$link->query("SELECT * FROM test")) {
		if (isset($connect_errno_codes[$link->errno]))
			printf("[005] Connect error, [%d] %s\n", $link->errno, $link->error);
		else
			printf("[005] Unexpected error, [%d] %s\n", $link->errno, $link->error);
	}
	$threads[$link->thread_id] = array('slave2');

	/* db.dual -> rr, slave 3 */
	run_query(6, $link, "SELECT 1 FROM DUAL");
	$threads[$link->thread_id] = array('slave3');

	/* db.dual -> rr, slave 1 */
	run_query(7, $link, "SELECT 1 FROM DUAL");
	$threads[$link->thread_id][] = 'slave1';

	/* db.dual -> rr, slave 3 */
	run_query(6, $link, "SELECT 1 FROM DUAL");
	$threads[$link->thread_id][] = 'slave3';

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

	if (!unlink("test_mysqlnd_ms_table_slave_rr.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_slave_rr.ini'.\n");
?>
--EXPECTF--
[005] Connect error, [%d] %s
%d: master1,
%d: slave1,slave1,
0: slave2,
%d: slave3,slave3,
done!