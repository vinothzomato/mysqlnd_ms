--TEST--
Lazy connect, slave failure, random_once
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

if (($master_host == $slave_host)) {
	die("SKIP master and slave seem to the the same, see tests/README");
}

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array("unreachable:6033", "unreachable2:6033"),
		'pick' 	=> array('random' => array('sticky' => '1')),
		'lazy_connections' => 1
	),
);
if ($error = create_config("test_mysqlnd_ms_lazy_slave_failure_random_once.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_lazy_slave_failure_random_once.ini
--FILE--
<?php
	require_once("connect.inc");

	function run_query($offset, $link, $query, $switch = NULL) {
		global $connect_errno_codes;
		if ($switch)
			$query = sprintf("/*%s*/%s", $switch, $query);

		if (!($ret = $link->query($query))) {
			if (isset($connect_errno_codes[$link->errno]))
				printf("Expected error, ");
			printf("[%03d] [%d] %s\n", $offset, $link->errno, $link->error);
		}

		return $ret;
	}

	function schnattertante($res) {
		if (!$res)
		  return;
		$row = $res->fetch_assoc();
		$res->close();
		printf("This is '%s' speaking\n", $row['_role']);
	}

	/*
	Error: 2002 (CR_CONNECTION_ERROR)
	Message: Can't connect to local MySQL server through socket '%s' (%d)
	Error: 2003 (CR_CONN_HOST_ERROR)
	Message: Can't connect to MySQL server on '%s' (%d)
	Error: 2005 (CR_UNKNOWN_HOST)
	Message: Unknown MySQL server host '%s' (%d)
	*/
	$connect_errno_codes = array(
		2002 => true,
		2003 => true,
		2005 => true,
	);

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$connections = array();

	run_query(2, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH);
	$connections[$link->thread_id] = array('master');

	run_query(3, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);
	$connections[$link->thread_id][] = 'slave (no fallback)';

	schnattertante(run_query(4, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role"));
	$connections[$link->thread_id][] = 'slave (no fallback)';

	foreach ($connections as $thread_id => $details) {
		printf("Connection %d -\n", $thread_id);
		foreach ($details as $msg)
		  printf("... %s\n", $msg);
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_lazy_slave_failure_random_once.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_lazy_slave_failure_random_once.ini'.\n");
?>
--EXPECTF--
Warning: mysqli::query(): [%d] %s
Expected error, [003] [%d] %s

Warning: mysqli::query(): [%d] %s
Expected error, [004] [%d] %s
Connection %d -
... master
Connection 0 -
... slave (no fallback)
... slave (no fallback)
done!
