--TEST--
Load Balancing: random (slaves)
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

$settings = array(
	"myapp" => array(
		'pick'		=> array('random'),
		'master' 	=> array($master_host),
		'slave' 	=> array($slave_host, $slave_host, $slave_host),
	),
);
if ($error = mst_create_config("test_mysqlnd_ms_pick_random.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

include_once("util.inc");
msg_mysqli_init_emulated_id_skip($slave_host, $user, $passwd, $db, $slave_port, $slave_socket, "slave[1,2,3]");
msg_mysqli_init_emulated_id_skip($master_host, $user, $passwd, $db, $master_port, $master_socket, "master");

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_pick_random.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	function fetch_role($offset, $link, $switch = NULL) {
		$query = 'SELECT @myrole AS _role';
		if ($switch)
			$query = sprintf("/*%s*/%s", $switch, $query);

		$res = mst_mysqli_query($offset, $link, $query, $switch);
		if (!$res) {
			printf("[%03d +01] [%d] [%s\n", $offset, $link->errno, $link->error);
			return NULL;
		}

		$row = $res->fetch_assoc();
		return $row['_role'];
	}

	if (!($link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());


	/* first master */
	mst_mysqli_query(2, $link, "SET @myrole = 'Master 1'", MYSQLND_MS_MASTER_SWITCH);

	$slaves = array();
	do {
		mst_mysqli_query(3, $link, "SELECT 1");
		$server_id = mst_mysqli_get_emulated_id(4, $link);
		if (!isset($slaves[$server_id])) {
			$slaves[$server_id] = array('role' => sprintf("Slave %d", count($slaves) + 1), 'queries' => 0);
		}
	} while (count($slaves) < 3);

	$sequences = array();
	$num_queries = 1000;
	for ($i = 0; $i < $num_queries; $i+=3) {
		mst_mysqli_query(5, $link, "SELECT 1");
		$server_id = mst_mysqli_get_emulated_id(6, $link);
		$slaves[$server_id]['queries']++;
		$sequence_id = $server_id;
		
		mst_mysqli_query(7, $link, "SELECT 1");
		$server_id = mst_mysqli_get_emulated_id(8, $link);
		$slaves[$server_id]['queries']++;
		$sequence_id .= $server_id;

		mst_mysqli_query(9, $link, "SELECT 1");
		$server_id = mst_mysqli_get_emulated_id(10, $link);
		$slaves[$server_id]['queries']++;
		$sequence_id .= $server_id;

		if (!isset($sequences[$sequence_id]))
		  $sequences[$sequence_id] = 1;
		else
		  $sequences[$sequence_id]++;
	}

	if (count($sequences) == 1)
		printf("[011] Slaves are always called in the same order, does not look like random\n");

	$min = ($num_queries / 3) * 0.5;
	$max = ($num_queries / 3) * 2;

	foreach ($slaves as $thread => $details) {
		printf("%s (%s) has run %d queries.\n", $details['role'], $thread, $details['queries']);
		if ($details['queries'] < $min) {
			printf("[008] %s (%s) has run less queries (%d) than expected (> %d), check manually.\n",
			  $details['role'], $thread, $details['queries'], $min);
		}

		if ($details['queries'] > $max) {
			printf("[009] %s (%s) has run more queries (%d) than expected (< %d), check manually.\n",
			  $details['role'], $thread, $details['queries'], $max);
		}
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_pick_random.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_pick_random.ini'.\n");
?>
--EXPECTF--
Slave 1 (%s) has run %d queries.
Slave 2 (%s) has run %d queries.
Slave 3 (%s) has run %d queries.
done!