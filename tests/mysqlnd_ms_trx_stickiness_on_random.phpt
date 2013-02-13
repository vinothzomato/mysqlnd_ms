--TEST--
trx_stickiness=on, pick = random
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
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
		'trx_stickiness' => 'on',
		'pick' => array("random"),
	),
);
if ($error = mst_create_config("test_mysqlnd_ms_trx_stickiness_on_random.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.config_file=test_mysqlnd_ms_trx_stickiness_on_random.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	if (!($link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	mst_mysqli_query(2, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH);

	$slaves = array();
	do {
		mst_mysqli_query(3, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);
		$slaves[$link->thread_id] = true;
	} while (count($slaves) < 2);


	$link->begin_transaction();
	/* this can be the start of a transaction, thus it shall be run on the master */
	mst_mysqli_fech_role(mst_mysqli_query(5, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role"));

	print "done!";
?>
--XFAIL--
Playground, feature in the works
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_trx_stickiness_on_random.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_trx_stickiness_on_random.ini'.\n");
?>
--EXPECTF--
This is 'master %d' speaking
done!