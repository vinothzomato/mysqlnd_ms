--TEST--
Lazy connect, failover = master, pick = random_once
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
		'lazy_connections' => 1,
		'failover' => 'master'
	),
);
if ($error = create_config("test_mysqlnd_ms_settings_lazy_failover_master.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_settings_lazy_failover_master.ini
mysqlnd.debug="d:t:O,/tmp/mysqlnd.trace";
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

	function schnattertante($res) {
		$row = $res->fetch_assoc();
		$res->close();
		printf("This is '%s' speaking\n", $row['_role']);
	}

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$connections = array();

	run_query(2, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH);
	$connections[$link->thread_id] = array('master');

	run_query(3, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);
	$connections[$link->thread_id][] = 'slave (fallback to master)';

	schnattertante(run_query(4, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role"));
	$connections[$link->thread_id][] = 'slave (fallback to master)';

	foreach ($connections as $thread_id => $details) {
		printf("Connection %d -\n", $thread_id);
		foreach ($details as $msg)
		  printf("... %s\n", $msg);
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_settings_lazy_failover_master.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_settings_lazy_failover_master.ini'.\n");
?>
--EXPECTF--

Warning: mysqli::query(): [%d] %s

Warning: mysqli::query(): [%d] %s
This is 'slave %d' speaking
Connection %d -
... master
... slave (fallback to master)
... slave (fallback to master)
done!