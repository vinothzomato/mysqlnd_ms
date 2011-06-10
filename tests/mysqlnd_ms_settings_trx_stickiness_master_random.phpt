--TEST--
trx_stickiness=master (PHP 5.3.99+, pick = random
--SKIPIF--
<?php
if (version_compare(PHP_VERSION, '5.3.99-dev', '<'))
	die(sprintf("SKIP Requires PHP 5.3.99 or newer, using " . PHP_VERSION));

require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
		'trx_stickiness' => 'master',
		'pick' => array("random"),
	),
);
if ($error = create_config("test_mysqlnd_ms_settings_trx_stickiness_master_random.ini", $settings))
	die(sprintf("SKIP %d\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_settings_trx_stickiness_master_random.ini
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

	run_query(2, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH);

	$slaves = array();
	do {
		run_query(3, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);
		$slaves[$link->thread_id] = true;
	} while (count($slaves) < 2);

	/* explicitly disabling autocommit via API */
	$link->autocommit(FALSE);
	/* this can be the start of a transaction, thus it shall be run on the master */
	schnattertante(run_query(5, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role"));

	/* back to the slave for the next SELECT because autocommit  is on */
	$link->autocommit(TRUE);
	schnattertante(run_query(6, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role"));
	schnattertante(run_query(7, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role"));
	schnattertante(run_query(8, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role"));

	/* explicitly disabling autocommit via API */
	$link->autocommit(FALSE);
	/* SQL hint wins */
	schnattertante(run_query(9, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role", MYSQLND_MS_SLAVE_SWITCH));
	schnattertante(run_query(10, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role", MYSQLND_MS_LAST_USED_SWITCH));
	schnattertante(run_query(10, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role", MYSQLND_MS_MASTER_SWITCH));

	schnattertante(run_query(11, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role"));

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_settings_trx_stickiness_master_random.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_settings_trx_stickiness_master_random.ini'.\n");
?>
--EXPECTF--
This is 'master %d' speaking
This is 'slave %d' speaking
This is 'slave %d' speaking
This is 'slave %d' speaking
This is 'slave %d' speaking
This is 'slave %d' speaking
This is 'master %d' speaking
This is 'master %d' speaking
done!