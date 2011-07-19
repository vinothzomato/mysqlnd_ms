--TEST--
trx_stickiness=master (PHP 5.3.99+), pick = roundrobin
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
		'pick' => array("roundrobin"),
	),
);
if ($error = create_config("test_mysqlnd_ms_trx_stickiness_master_round_robin.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_trx_stickiness_master_round_robin.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	run_query(2, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH);
	run_query(3, $link, "SET @myrole='slave 1'", MYSQLND_MS_SLAVE_SWITCH);
	run_query(4, $link, "SET @myrole='slave 2'", MYSQLND_MS_SLAVE_SWITCH);

	/* explicitly disabling autocommit via API */
	$link->autocommit(FALSE);
	/* this can be the start of a transaction, thus it shall be run on the master */
	schnattertante(run_query(5, $link, "SELECT @myrole AS _role"));

	/* back to the slave for the next SELECT because autocommit  is on */
	$link->autocommit(TRUE);
	schnattertante(run_query(6, $link, "SELECT @myrole AS _role"));
	schnattertante(run_query(7, $link, "SELECT @myrole AS _role"));
	schnattertante(run_query(8, $link, "SELECT @myrole AS _role"));

	/* explicitly disabling autocommit via API */
	$link->autocommit(FALSE);
	/* SQL hint wins */
	schnattertante(run_query(9, $link, "SELECT @myrole AS _role", MYSQLND_MS_SLAVE_SWITCH));
	schnattertante(run_query(10, $link, "SELECT @myrole AS _role", MYSQLND_MS_LAST_USED_SWITCH));
	schnattertante(run_query(10, $link, "SELECT @myrole AS _role", MYSQLND_MS_MASTER_SWITCH));

	schnattertante(run_query(11, $link, "SELECT @myrole AS _role"));

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_trx_stickiness_master_round_robin.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_trx_stickiness_master_round_robin.ini'.\n");
?>
--EXPECTF--
This is 'master' speaking
This is 'slave 1' speaking
This is 'slave 2' speaking
This is 'slave 1' speaking
This is 'slave 2' speaking
This is 'slave 2' speaking
This is 'master' speaking
This is 'master' speaking
done!