--TEST--
Round robin load balancing
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		/* CAUTION: during development we sometimes did support multi-master somestimes not */
		'master' => array($master_host, $master_host),
		'slave' => array($slave_host, $slave_host),
		'pick' => array('roundrobin'),
	),
);
if ($error = create_config("test_mysqlnd_round_robin.ini", $settings))
	die(sprintf("SKIP %d\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_round_robin.ini
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

	/* second master */
	run_query(3, $link, "SET @myrole = 'Master 2'", MYSQLND_MS_MASTER_SWITCH);

	/* pick first in row */
	run_query(4, $link, "SET @myrole = 'Slave 1'", MYSQLND_MS_SLAVE_SWITCH);

	/* move to second in row */
	run_query(5, $link, "SET @myrole = 'Slave 2'", MYSQLND_MS_SLAVE_SWITCH);

	$servers = array();

	/* wrap around to first slave */
	$role = fetch_role(6, $link);
	if (isset($servers[$link->thread_id][$role]))
		$servers[$link->thread_id][$role] = $servers[$link->thread_id][$role] + 1;
	else
		$servers[$link->thread_id] = array($role => 1);

	/* move forward to second slave */
	$role = fetch_role(7, $link);
	if (isset($servers[$link->thread_id][$role]))
		$servers[$link->thread_id][$role] = $servers[$link->thread_id][$role] + 1;
	else
		$servers[$link->thread_id] = array($role => 1);

	/* wrap around to first master */
	$role = fetch_role(8, $link, MYSQLND_MS_MASTER_SWITCH);
	if (isset($servers[$link->thread_id][$role]))
		$servers[$link->thread_id][$role] = $servers[$link->thread_id][$role] + 1;
	else
		$servers[$link->thread_id] = array($role => 1);

	/* move forward to the second master */
	$role = fetch_role(9, $link, MYSQLND_MS_MASTER_SWITCH);
	if (isset($servers[$link->thread_id][$role]))
		$servers[$link->thread_id][$role] = $servers[$link->thread_id][$role] + 1;
	else
		$servers[$link->thread_id] = array($role => 1);


	foreach ($servers as $thread_id => $roles) {
		foreach ($roles as $role => $num_queries) {
			printf("%s (%d) has run %d queries\n", $role, $thread_id, $num_queries);
		}
	}
	print "done!";

?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_round_robin.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_round_robin.ini'.\n");
?>
--EXPECTF--
Slave 1 (%d) has run 1 queries
Slave 2 (%d) has run 1 queries
Master 2 (%d) has run 2 queries
done!