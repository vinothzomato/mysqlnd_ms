--TEST--
Limits: autocommit - NOT handled by plugin
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

if ($master_host == $slave_host) {
	die("SKIP master and slave seem to the the same, see tests/README");
}

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host),
	),
);
if ($error = create_config("test_mysqlnd_ms_autocommit.ini", $settings))
	die(sprintf("SKIP %d\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_autocommit.ini
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

	/*
	Note: link->autocommit is not handled by the plugin! Don't rely on it!
	*/

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno($link), mysqli_connect_error($link));

	if (!mysqli_autocommit($link, false))
		printf("[002] Failed to change autocommit setting\n");

	run_query(3, $link, "SET autocommit=0", MYSQLND_MS_MASTER_SWITCH);
	run_query(4, $link, "SET autocommit=0", MYSQLND_MS_SLAVE_SWITCH);

	/* applied to master connection because master is the first one contacted by plugin and autocommit is not dispatched */
	if (!mysqli_autocommit($link, true))
		printf("[005] Failed to change autocommit setting\n");

	/* slave because SELECT */
	$res = run_query(6, $link, "SELECT @@autocommit AS auto_commit");
	$row = $res->fetch_assoc();
	if (1 != $row['auto_commit'])
		printf("[007] Autocommit should be on, got '%s'\n", $row['auto_commit']);

	/* master because of hint */
	$res = run_query(8, $link, "SELECT @@autocommit AS auto_commit", MYSQLND_MS_MASTER_SWITCH);
	$row = $res->fetch_assoc();
	if (1 != $row['auto_commit'])
		printf("[009] Autocommit should be on, got '%s'\n", $row['auto_commit']);

	$link->close();

	/* no plugin magic */

	if (!($link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)))
		printf("[010] [%d] %s\n", mysqli_connect_errno($link), mysqli_connect_error($link));

	if (!mysqli_autocommit($link, false))
		printf("[011] Failed to change autocommit setting\n");

	run_query(12, $link, "SET autocommit=0");

	if (!mysqli_autocommit($link, false))
		printf("[013] Failed to change autocommit setting\n");

	$res = run_query(14, $link, "SELECT @@autocommit AS auto_commit");
	$row = $res->fetch_assoc();
	if (0 != $row['auto_commit'])
		printf("[015] Autocommit should be off, got '%s'\n", $row['auto_commit']);

	if (!mysqli_autocommit($link, true))
		printf("[016] Failed to change autocommit setting\n");

	$res = run_query(17, $link, "SELECT @@autocommit AS auto_commit");
	$row = $res->fetch_assoc();
	if (1 != $row['auto_commit'])
		printf("[018] Autocommit should be on, got '%s'\n", $row['auto_commit']);

	print "done!";

?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_autocommit.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_ini_force_config.ini'.\n");
?>
--EXPECTF--
[007] Autocommit should be on, got '0'
done!