--TEST--
Lazy connect, slave failure and existing slave, user
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
		'slave' => array("unreachable:6033", $slave_host, "unreachable2:6033"),
		'pick' 	=> array('user' => array("callback" => "pick_server")),
		'lazy_connections' => 1
	),
);
if ($error = create_config("test_mysqlnd_lazy_slave_failure_rr.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_lazy_slave_failure_rr.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");
	require_once("mysqlnd_ms_pick_user.inc");

	function pick_server($connected_host, $query, $master, $slaves, $last_used_connection, $in_transaction) {
		static $slave_idx = 0;

		$where = mysqlnd_ms_query_is_select($query);
		$server = '';
		switch ($where) {
			case MYSQLND_MS_QUERY_USE_LAST_USED:
			  $ret = $last_used_connection;
			  $server = 'last used';
			  break;
			case MYSQLND_MS_QUERY_USE_MASTER:
			  $ret = $master[0];
			  $server = 'master';
			  break;
			case MYSQLND_MS_QUERY_USE_SLAVE:
			  if ($slave_idx > 2)
				$slave_idx = 0;
			  $server = 'slave';
 			  $ret = $slaves[$slave_idx++];
			  break;
			default:
			  printf("Unknown return value from mysqlnd_ms_query_is_select, where = %s .\n", $where);
			  $ret = $master[0];
			  $server = 'unknown';
			  break;
		}
		printf("pick_server('%s', '%s') => %s\n", $connected_host, $query, $server);
		return $ret;
	}

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$connections = array();

	run_query(2, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH);
	$connections[$link->thread_id] = array('master');

	run_query(3, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);
	$connections[$link->thread_id][] = 'slave (no fallback)';

	schnattertante(run_query(4, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role"));
	$connections[$link->thread_id] = array('slave');

	schnattertante(run_query(5, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role"));
	$connections[$link->thread_id][] = 'slave (no fallback)';

	schnattertante(run_query(6, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role"));
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
	if (!unlink("test_mysqlnd_lazy_slave_failure_rr.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_lazy_slave_failure_rr.ini'.\n");
?>
--EXPECTF--
pick_server('myapp', '/*ms=master*/SET @myrole='master'') => master
pick_server('myapp', '/*ms=slave*/SET @myrole='slave'') => slave
%AE_WARNING] mysqli::query(): [%d] %s
[E_WARNING] mysqli::query(): Callback chose tcp://unreachable:6033 but connection failed in %s on line %d
Connect error, [003] [%d] %s
pick_server('myapp', 'SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role') => slave
This is '' speaking
pick_server('myapp', 'SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role') => slave
%AE_WARNING] mysqli::query(): [%d] %s
[E_WARNING] mysqli::query(): Callback chose tcp://unreachable2:6033 but connection failed in %s on line %d
Connect error, [005] [%d] %s
pick_server('myapp', 'SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role') => slave
%AE_WARNING] mysqli::query(): [%d] %s
[E_WARNING] mysqli::query(): Callback chose tcp://unreachable:6033 but connection failed in %s on line %d
Connect error, [006] [%d] %s
Connection %d -
... master
Connection 0 -
... slave (no fallback)
... slave (no fallback)
... slave (no fallback)
Connection %d -
... slave
done!