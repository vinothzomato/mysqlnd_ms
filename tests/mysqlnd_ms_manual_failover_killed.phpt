--TEST--
Manual failover, unknown slave host
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' 	=> array($master_host),
		'slave' 	=> array($slave_host, $slave_host, $slave_host),
		'pick' 		=> array("roundrobin"),
		'failover'	=> 'master',
	),
);
if ($error = create_config("test_mysqlnd_ms_failover_killed.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_failover_killed.ini
--FILE--
<?php
	require_once("connect.inc");

	function run_query($offset, $link, $query) {
		global $connect_errno_codes;

		if (!($res = @$link->query($query))) {
			printf("[%03d + 01] [%d] %s\n", $offset, $link->errno, $link->error);
			return 0;
		}

		if (!is_object($res)) {
			return $link->thread_id;
		}

		if (!($row = $res->fetch_assoc())) {
			printf("[%03d + 03] [%d] %s\n", $offset, $link->errno, $link->error);
			return 0;
		}
		return $link->thread_id;
	}

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (0 !== mysqli_connect_errno())
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$threads = array(
		'master' 	=> array(),
		'slave 1' 	=> array(),
		'slave 2' 	=> array(),
		'slave 3' 	=> array(),
	);

	$threads['master'][run_query(10, $link, "DROP TABLE IF EXISTS test")] = 1;
	$threads['slave 1'][run_query(20, $link, "SELECT 'Slave 1' AS msg")] = 1;
	$threads['slave 2'][$thread_id = run_query(30, $link, "SELECT 'Slave 2' AS msg")] = 1;
	$threads['slave 3'][run_query(40, $link, "SELECT 'Slave 3' AS msg")] = 1;

	$link->kill($thread_id);

	$threads['slave 1'][run_query(50, $link, "SELECT 'Slave 1' AS msg")]++;
	$threads['slave 2'][run_query(60, $link, "SELECT 'Slave 2' AS msg")] = 1;
	$threads['slave 3'][run_query(70, $link, "SELECT 'Slave 3' AS msg")]++;

	$threads['slave 1'][run_query(80, $link, "SELECT 'Slave 1' AS msg")]++;
	$threads['slave 2'][run_query(90, $link, "SELECT 'Slave 2' AS msg")]++;
	$threads['slave 3'][run_query(100, $link, "SELECT 'Slave 3' AS msg")]++;

	foreach ($threads['slave 2'] as $thread_id => $num_queries) {
		printf("Slave 2, %d\n", $thread_id);
		if (isset($threads['slave 1'][$thread_id])) {
			printf("[201] Slave 2 is Slave 1 ?!\n");
			var_dump($threads);
		}
		if (isset($threads['slave 3'][$thread_id])) {
			printf("[202] Slave 2 is Slave 3 ?!\n");
			var_dump($threads);
		}
		if (isset($threads['master'][$thread_id])) {
			printf("[203] Slave 2 is the Master ?!\n");
			var_dump($threads);
		}

	}
	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_failover_killed.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_failover_killed.ini'.\n");
?>
--EXPECTF--
[060 + 01] [2006] MySQL server has gone away
[090 + 01] [2006] MySQL server has gone away
Slave 2, %d
Slave 2, 0
done!