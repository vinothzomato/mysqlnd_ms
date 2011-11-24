--TEST--
mysqlnd_ms_get_last_gtid()
--SKIPIF--
<?php
if (version_compare(PHP_VERSION, '5.3.99-dev', '<'))
	die(sprintf("SKIP Requires PHP >= 5.3.99, using " . PHP_VERSION));

require_once('skipif.inc');
  require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

include_once("util.inc");
$sql = mst_get_gtid_sql($db);

if ($error = mst_mysqli_drop_gtid_table($master_host_only, $user, $passwd, $db, $master_port, $master_socket))
  die(sprintf("SKIP Failed to drop GTID on master, %s\n", $error));

if ($error = mst_mysqli_setup_gtid_table($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket))
  die(sprintf("SKIP Failed to setup GTID on slave, %s\n", $error));

$settings = array(
	"myapp" => array(
		'master' => array(
			"master1" => array(
				'host' 		=> $master_host_only,
				'port' 		=> (int)$master_port,
				'socket' 	=> $master_socket,
			),
		),
		'slave' => array(
			"slave1" => array(
				'host' 	=> $slave_host_only,
				'port' 	=> (int)$slave_port,
				'socket' => $slave_socket,
			),
		),

		'global_transaction_id_injection' => array(
			'on_commit'	 				=> $sql['update'],
			'fetch_last_gtid'			=> $sql['fetch_last_gtid'],
			'check_for_gtid'			=> $sql['check_for_gtid'],
			'report_error'				=> true,
			'set_on_slave'				=> true,
		),

		'lazy_connections' => 1,
		'trx_stickiness' => 'disabled',
		'filters' => array(
			"roundrobin" => array(),
		),
	),

);
if ($error = mst_create_config("test_mysqlnd_ms_get_last_gtid_slave.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_get_last_gtid_slave.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	/* autocommit, slave */
	mst_mysqli_query(2, $link, "DROP TABLE IF EXISTS test", MYSQLND_MS_SLAVE_SWITCH);
	if (1 != ($gtid = mysqlnd_ms_get_last_gtid($link))) {
		printf("[004] Expecting 1, got %s\n", var_export($gtid, true));
	} else {
		printf("[005] [%d] %s\n", $link->errno, $link->error);
	}
	$last_gtid = $gtid;

	$link->autocommit(false);

	mst_mysqli_query(6, $link, "CREATE TABLE test(id INT)", MYSQLND_MS_SLAVE_SWITCH);

	if ($last_gtid !== ($gtid = mysqlnd_ms_get_last_gtid($link)))
		printf("[008] Expecting %s got %s, [%d] %s\n", $last_gtid, $gtid, $link->errno, $link->error);

	if (!$link->rollback())
		printf("[009] [%d] %s\n", $link->errno, $link->error);

	if (!$link->commit())
		printf("[010] [%d] %s\n", $link->errno, $link->error);

	if (false === ($gtid = mysqlnd_ms_get_last_gtid($link)))
		printf("[011] [%d] %s\n", $last_gtid, $gtid, $link->errno, $link->error);

	if ($gtid <= $last_gtid)
		printf("[012] last %s, new %s\n", $last_gtid, $gtid);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_get_last_gtid_slave.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_get_last_gtid_slave.ini'.\n");
?>
--EXPECTF--
[005] [0%s
done!