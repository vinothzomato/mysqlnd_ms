--TEST--
Limits: SQL transactions
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
if ($error = create_config("test_mysqlnd_ms_sql_transactions.ini", $settings))
	die(sprintf("SKIP %d\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.force_config_usage=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_sql_transactions.ini
mysqlnd.debug=d:t:O,/tmp/mysqlnd.trace
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
		The tricky part about transactions is that the plugin ignores
		them altogether. Autocommit on or off, the plugin ignores it
		and does query redirection regardless of transaction settings.
	*/

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno($link), mysqli_connect_error($link));


	/* Note: unlike with C/J the autocommit setting does not impact the redirection */
	run_query(2, $link, "SET autocommit = 0", MYSQLND_MS_MASTER_SWITCH);
	run_query(3, $link, "SET autocommit = 0", MYSQLND_MS_SLAVE_SWITCH);
	$link->autocommit = false;

	/* slave */
	run_query(100, $link, "DROP TABLE IF EXISTS test", MYSQLND_MS_SLAVE_SWITCH);
	run_query(4, $link, "CREATE TABLE test(id VARCHAR(20)) ENGINE=InnoDB", MYSQLND_MS_LAST_USED_SWITCH);
	run_query(5, $link, "START TRANSACTION", MYSQLND_MS_LAST_USED_SWITCH);
	run_query(6, $link, "INSERT INTO test(id) VALUES ('slave')", MYSQLND_MS_LAST_USED_SWITCH);
	run_query(7, $link, "COMMIT", MYSQLND_MS_LAST_USED_SWITCH);

	/* master */
	run_query(101, $link, "DROP TABLE IF EXISTS test", MYSQLND_MS_MASTER_SWITCH);
	run_query(8, $link, "CREATE TABLE test(id VARCHAR(20)) ENGINE=InnoDB", MYSQLND_MS_LAST_USED_SWITCH);
	run_query(9, $link, "START TRANSACTION", MYSQLND_MS_LAST_USED_SWITCH);
	run_query(10, $link, "INSERT INTO test(id) VALUES ('master')", MYSQLND_MS_LAST_USED_SWITCH);
	run_query(11, $link, "COMMIT", MYSQLND_MS_LAST_USED_SWITCH);
	run_query(12, $link, "START TRANSACTION", MYSQLND_MS_LAST_USED_SWITCH);
	run_query(13, $link, "INSERT INTO test(id) VALUES ('master')", MYSQLND_MS_LAST_USED_SWITCH);

	/* leaving master with open transaction */
	run_query(14, $link, "START TRANSACTION", MYSQLND_MS_SLAVE_SWITCH);
	run_query(15, $link, "INSERT INTO test(id) VALUES ('slave')", MYSQLND_MS_LAST_USED_SWITCH);
	run_query(16, $link, "COMMIT", MYSQLND_MS_LAST_USED_SWITCH);

	/* back to the master for a rollback */
	run_query(17, $link, "ROLLBACK", MYSQLND_MS_MASTER_SWITCH);
	$res = run_query(18, $link, "SELECT id FROM test", MYSQLND_MS_LAST_USED_SWITCH);
	while ($row = $res->fetch_assoc())
		printf("Master, id = '%s'\n", $row['id']);

	/* back to the slave for a select */
	$res = run_query(19, $link, "SELECT id FROM test");
	while ($row = $res->fetch_assoc())
		printf("Slave, id = '%s'\n", $row['id']);

	/*
	  No hint. Goes to the master. Open transaction, no autocommit. Slave will not see it.
	  For test running we do not require $master_host and $slave_host to
	  identify an actual master and slave in a MySQL replication setup.
	  However, you get the point when looking at the test...
	*/
	$res = run_query(20, $link, "INSERT INTO test(id) VALUES ('master')");
	/* back to the slave for a select */
	$res = run_query(21, $link, "SELECT id FROM test");
	while ($row = $res->fetch_assoc())
		printf("Slave, id = '%s'\n", $row['id']);

	print "done!";

?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_sql_transactions.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_ini_force_config.ini'.\n");


?>
--EXPECTF--
Master, id = 'master'
Slave, id = 'slave'
Slave, id = 'slave'
Slave, id = 'slave'
Slave, id = 'slave'
done!