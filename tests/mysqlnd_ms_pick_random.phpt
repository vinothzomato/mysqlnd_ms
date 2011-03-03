--TEST--
Load Balancing: random (slaves)
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'pick'		=> array('random'),
		'master' 	=> array($master_host),
		'slave' 	=> array($slave_host, $slave_host, $slave_host),
	),
);
if ($error = create_config("test_mysqlnd_random_pick.ini", $settings))
	die(sprintf("SKIP %d\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_random_pick.ini
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

	$slaves = array();
	do {
		run_query(3, $link, "SELECT 1");
		if (!isset($slaves[$link->thread_id])) {
			$slaves[$link->thread_id] = array('role' => sprintf("Slave %d", count($slaves) + 1), 'queries' => 0);
		}
	} while (count($slaves) < 3);

	$sequences = array();
	$num_queries = 1000;
	for ($i = 0; $i < $num_queries; $i+=3) {
		run_query(4, $link, "SELECT 1");
		$slaves[$link->thread_id]['queries']++;
		$sequence_id = $link->thread_id;
		run_query(5, $link, "SELECT 1");
		$slaves[$link->thread_id]['queries']++;
		$sequence_id .= sprintf("-%d", $link->thread_id);
		run_query(6, $link, "SELECT 1");
		$slaves[$link->thread_id]['queries']++;
		$sequence_id .= sprintf("-%d", $link->thread_id);

		if (!isset($sequences[$sequence_id]))
		  $sequences[$sequence_id] = 1;
		else
		  $sequences[$sequence_id]++;
	}

	if (count($sequences) == 1)
		printf("[007] Slaves are always called in the same order, does not look like random\n");

	$min = ($num_queries / 3) * 0.5;
	$max = ($num_queries / 3) * 2;

	foreach ($slaves as $thread => $details) {
		printf("%s (%d) has run %d queries.\n", $details['role'], $thread, $details['queries']);
		if ($details['queries'] < $min) {
			printf("[008] %s (%d) has run less queries (%d) than expected (> %d), check manually.\n",
			  $details['role'], $thread, $details['queries'], $min);
		}

		if ($details['queries'] > $max) {
			printf("[009] %s (%d) has run more queries (%d) than expected (< %d), check manually.\n",
			  $details['role'], $thread, $details['queries'], $max);
		}
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_random_pick.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_random_pick.ini'.\n");
?>
--EXPECTF--
Slave 1 (%d) has run %d queries.
Slave 2 (%d) has run %d queries.
Slave 3 (%d) has run %d queries.
done!