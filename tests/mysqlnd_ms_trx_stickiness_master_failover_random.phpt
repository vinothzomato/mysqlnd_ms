--TEST--
trx_stickiness=master, failover, random
--SKIPIF--
<?php
if (version_compare(PHP_VERSION, '5.3.99-dev', '<'))
	die(sprintf("SKIP Requires PHP 5.3.99 or newer, using " . PHP_VERSION));

require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

$settings = array(
	"myapp" => array(
		'master' => array($master_host, "unknown:6033"),
		'slave' => array($slave_host),
		'trx_stickiness' => 'master',
		'pick' => array("random"),
		'lazy_connections' => 1,
		'failover' => array('strategy' => 'loop_before_master'),
	),
);
if ($error = mst_create_config("test_mysqlnd_ms_trx_stickiness_master_failover_random.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.multi_master=1
mysqlnd_ms.config_file=test_mysqlnd_ms_trx_stickiness_master_failover_random.ini
mysqlnd.debug=d:t:O,/tmp/mysqlnd.trace
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	if (!($link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	mst_mysqli_query(2, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);
	$link->autocommit(FALSE);

	do {
		$res = $link->query("SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _master_role");
		if ($res)
			var_dump($res->fetch_assoc());

	} while (0 == $link->errno);
	printf("[004] [%d] %s\n", $link->errno, $link->error);

	$link->autocommit(TRUE);
	for ($i = 0; $i < 10; $i++) {
		$res = $link->query("SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role");
		printf("[005] [%d] '%s'\n", $link->errno, $link->error);
	}
	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_trx_stickiness_master_failover_random.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_trx_stickiness_master_failover_random.ini'.\n");
?>
--EXPECTF--
%AWarning: mysqli::query(): php_network_getaddresses: getaddrinfo failed: Name or service not known in %s on line %d

Warning: mysqli::query(): (mysqlnd_ms) Automatic failover is not permitted in the middle of a transaction in %s on line %d

Warning: mysqli::query(): (mysqlnd_ms) No connection selected by the last filter in %s on line %A
[004] [2000] (mysqlnd_ms) No connection selected by the last filter
[005] [0] ''
[005] [0] ''
[005] [0] ''
[005] [0] ''
[005] [0] ''
[005] [0] ''
[005] [0] ''
[005] [0] ''
[005] [0] ''
[005] [0] ''
done!