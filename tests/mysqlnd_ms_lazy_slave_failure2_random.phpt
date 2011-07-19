--TEST--
Lazy connect, slave failure and existing slave, random
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
		'slave' => array("unreachable:6033", "unreachable2:6033", $slave_host, "unreachable3:6033"),
		'pick' 	=> array('random'),
		'lazy_connections' => 1
	),
);
if ($error = create_config("test_mysqlnd_ms_lazy_slave_failure_random.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_lazy_slave_failure_random.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$connections = array();

	run_query(2, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH);
	$connections[$link->thread_id] = array('master');

	run_query(3, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH, true);
	$connections[$link->thread_id][1] = 'slave (no fallback)';

	$states = array("failure" => false, "connect" => false);
	do {
	  $res = @run_query(4, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role", NULL, true);
	  if ($res) {
		  $row = $res->fetch_assoc();
		  $res->close();
		  if (!$states['connect']) {
			printf("This is '%s' speaking\n", $row['_role']);
			$connections[$link->thread_id] = array('slave');
		  }
		  $states['connect'] = true;
	  } else {
		  $states['failure'] = true;
		  $connections[$link->thread_id][2] = 'slave (no fallback)';
	  }
	} while (!($states['connect'] && $states['failure']));



	ksort($connections);
	foreach ($connections as $thread_id => $details) {
		printf("Connection %d -\n", $thread_id);
		foreach ($details as $msg)
		  printf("... %s\n", $msg);
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_lazy_slave_failure_random.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_lazy_slave_failure_random.ini'.\n");
?>
--EXPECTF--
This is %s speaking
Connection 0 -
... slave (no fallback)
... slave (no fallback)
Connection %d -
... master
Connection %d -
... slave
done!
