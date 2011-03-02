--TEST--
Load Balancing: random_once (slaves)
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

if ($master_host == $slave_host) {
	die("SKIP master and slave seem to the the same, see tests/README");
}

$settings = array(
	"myapp" => array(
		'pick'		=> array('random_once'),
		'master' 	=> array($master_host),
		'slave' 	=> array($slave_host, $slave_host, $slave_host),
	),
);
if ($error = create_config("test_mysqlnd_pick_random_once.ini", $settings))
	die(sprintf("SKIP %d\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_pick_random_once.ini
--FILE--
<?php
	require_once("connect.inc");

	function run_query($offset, $link, $query, $switch = NULL) {
		if ($switch)
			$query = sprintf("/*%s*/%s", $switch, $query);

		if (!($ret = $link->query($query)))
			printf("[%03d] [%d] %s\n", $offset, $link->errno, $link->error);

		return $ret;
	}

	function fetch_role($offset, $link, $switch = NULL) {
		$query = 'SELECT @myrole AS _role';
		if ($switch)
			$query = sprintf("/*%s*/%s", $switch, $query);

		$res = run_query($offset, $link, $query, $switch);
		if (!$res) {
			printf("[%03d +01] [%d] [%s\n", $offset, $link->errno, $link->error);
			return NULL;
		}

		$row = $res->fetch_assoc();
		return $row['_role'];
	}

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());


	/* first master */
	run_query(2, $link, "SET @myrole = 'Master 1'", MYSQLND_MS_MASTER_SWITCH);
	$master = $link->thread_id;

	$slaves = array();
	$num_queries = 100;
	for ($i = 0; $i <= $num_queries; $i++) {
		run_query(3, $link, "SELECT 1");
		if (!isset($slaves[$link->thread_id])) {
			$slaves[$link->thread_id] = array('role' => sprintf("Slave %d", count($slaves) + 1), 'queries' => 0);
		} else {
			$slaves[$link->thread_id]['queries']++;
		}
		if ($link->thread_id == $master)
			printf("[004] Master and slave use the same connection!\n");

		if (mt_rand(0, 10) > 9) {
			/* switch to master to check if next read goes to same slave */
			run_query(5, $link, "DROP TABLE IF EXISTS test");

		}
	}

	foreach ($slaves as $thread => $details) {
		printf("%s (%d) has run %d queries.\n", $details['role'], $thread, $details['queries']);
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_pick_random_once.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_pick_random_once.ini'.\n");
?>
--EXPECTF--
Slave 1 (%d) has run 100 queries.
done!