--TEST--
table filter: multiple master
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
						"master" => array("master1", "master2"),
						"slave" => array("slave1"),
					),
				),
			),
			"roundrobin" => array(),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_table_write_non_unique.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_write_non_unique.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	/* shall use host = forced_master_hostname_abstract_name from the ini file */
	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	$masters = array();
	/* master 1 */
	run_query(2, $link, "DROP TABLE IF EXISTS test");
	$masters[] = $link->thread_id;

	/* master 2 */
	run_query(3, $link, "DROP TABLE IF EXISTS test");
	$masters[] = $link->thread_id;

	/* master 1 */
	run_query(4, $link, "DROP TABLE IF EXISTS test");
	$masters[] = $link->thread_id;

	/* master 2 */
	run_query(5, $link, "DROP TABLE IF EXISTS test");
	$masters[] = $link->thread_id;

	$last = NULL;
	foreach ($masters as $k => $thread_id) {
		printf("%d -> %d\n", $k, $thread_id);
		if (!is_null($last)) {
			if ($last != $thread_id)
			  printf("Server switch\n");
		}
		$last = $thread_id;
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_table_write_non_unique.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_write_non_unique.ini'.\n");
?>
--EXPECTF--
Unclear what shall happen and what to test for. Best would be errors.
Load balancer may accept more than one master returned from partitioning filter but
end result is random data distribution and that's something to avoid.
[002] [%d] %s
[003] [%d] %s
[004] [%d] %s
[005] [%d] %s
0 -> 0
1 -> 0
2 -> 0
3 -> 0
done!