--TEST--
trx_stickiness=master (PHP 5.3.99+), pick = random_once = default
--SKIPIF--
<?php
if (version_compare(PHP_VERSION, '5.3.99-dev', '<'))
	die(sprintf("SKIP Requires PHP 5.3.99 or newer, using " . PHP_VERSION));

require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host),
		'trx_stickiness' => 'master',
	),
);
if ($error = create_config("test_mysqlnd_ms_settings_trx_stickiness_master.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_settings_trx_stickiness_master.ini
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

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	run_query(2, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH);
	$master_thread = $link->thread_id;
	run_query(3, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);
	$slave_thread = $link->thread_id;

	/* DDL, implicit commit */
	run_query(4, $link, "DROP TABLE IF EXISTS test");
	run_query(5, $link, "CREATE TABLE test(id INT) ENGINE=InnoDB");
	run_query(6, $link, "INSERT INTO test(id) VALUES(1), (2), (3)");

	/* autocommit is on, not "in transaction", slave shall be used */

	/* NOTE: we do not run SELECT id FROM test! It shall be possible
	to run the test suite without having to setup MySQL replication.
	The configured master and slave server may not be related, thus
	the table test may not be replicated. It is good enough to
	select from DUAL for testing. What needs to be tested is if any
	read-only query would be still run on the slave or on the master
	because we are in a transaction. The thread id tells us if
	the plugin has chosen the master or the salve. */

	$res = run_query(7, $link, "SELECT 1 AS id FROM DUAL");
	if ($link->thread_id != $slave_thread) {
		printf("[008] SELECT in autocommit mode should have been run on the slave\n");
	}
	$row = $res->fetch_assoc();
	$res->close();
	if ($row['id'] != 1)
		printf("[009] Expecting id = 1 got id = '%s'\n", $row['id']);

	/* explicitly setting autocommit via API */
	$link->autocommit(TRUE);
	$res = run_query(10, $link, "SELECT 1 AS id FROM DUAL");
	if ($link->thread_id != $slave_thread) {
		printf("[011] SELECT in autocommit mode should have been run on the slave\n");
	}
	$row = $res->fetch_assoc();
	$res->close();
	if ($row['id'] != 1)
		printf("[012] Expecting id = 1 got id = '%s'\n", $row['id']);

	/* explicitly disabling autocommit via API */
	$link->autocommit(FALSE);
	/* this can be the start of a transaction, thus it shall be run on the master */
	$res = run_query(13, $link, "SELECT 1 AS id FROM DUAL");
	if ($link->thread_id != $master_thread) {
		printf("[014] SELECT not run in autocommit mode should have been run on the master\n");
	}
	$row = $res->fetch_assoc();
	$res->close();
	if ($row['id'] != 1)
		printf("[015] Expecting id = 1 got id = '%s'\n", $row['id']);

	if (!$link->commit())
		printf("[016] [%d] %s\n", $link->errno, $link->error);

	/* autocommit is still off, thus it shall be run on the master */
	$res = run_query(17, $link, "SELECT id FROM test WHERE id = 1");
	if ($link->thread_id != $master_thread) {
		printf("[018] SELECT not run in autocommit mode should have been run on the master\n");
	}
	$row = $res->fetch_assoc();
	$res->close();
	if ($row['id'] != 1)
		printf("[019] Expecting id = 1 got id = '%s'\n", $row['id']);

	/* back to the slave for the next SELECT because autocommit  is on */
	$link->autocommit(TRUE);

	$res = run_query(20, $link, "SELECT 1 AS id FROM DUAL");
	if ($link->thread_id != $slave_thread) {
		printf("[021] SELECT in autocommit mode should have been run on the slave\n");
	}
	$row = $res->fetch_assoc();
	$res->close();
	if ($row['id'] != 1)
		printf("[022] Expecting id = 1 got id = '%s'\n", $row['id']);

	/* master because update... */
	run_query(23, $link, "UPDATE test SET id = 100 WHERE id = 1");

	/* back to the master because autocommit is off */
	$link->autocommit(FALSE);

	$res = run_query(24, $link, "SELECT id FROM test WHERE id = 100");
	if ($link->thread_id != $master_thread) {
		printf("[025] SELECT not run in autocommit mode should have been run on the master\n");
	}
	$row = $res->fetch_assoc();
	$res->close();
	if ($row['id'] != 100)
		printf("[026] Expecting id = 100 got id = '%s'\n", $row['id']);

	run_query(27, $link, "DELETE FROM test WHERE id = 100");
	if (!$link->rollback())
		printf("[028] [%s] %s\n", $link->errno, $link->error);

	$res = run_query(29, $link, "SELECT id FROM test WHERE id = 100");
	if ($link->thread_id != $master_thread) {
		printf("[030] SELECT not run in autocommit mode should have been run on the master\n");
	}
	$row = $res->fetch_assoc();
	$res->close();
	if ($row['id'] != 100)
		printf("[031] Expecting id = 100 got id = '%s'\n", $row['id']);

	/* SQL hint wins: use slave although autocommit is off */
	$res = run_query(32, $link, "SELECT 1 AS id FROM DUAL", MYSQLND_MS_SLAVE_SWITCH);
	if ($link->thread_id != $slave_thread) {
		printf("[033] SELECT in autocommit mode should have been run on the slave\n");
	}
	$row = $res->fetch_assoc();
	$res->close();
	if ($row['id'] != 1)
		printf("[034] Expecting id = 1 got id = '%s'\n", $row['id']);

	$res = run_query(35, $link, "SELECT 1 AS id FROM DUAL", MYSQLND_MS_LAST_USED_SWITCH);
	if ($link->thread_id != $slave_thread) {
		printf("[036] SELECT in autocommit mode should have been run on the slave\n");
	}
	$row = $res->fetch_assoc();
	$res->close();
	if ($row['id'] != 1)
		printf("[037] Expecting id = 1 got id = '%s'\n", $row['id']);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_settings_trx_stickiness_master.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_settings_trx_stickiness_master.ini'.\n");
?>
--EXPECTF--
done!