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
if ($error = mst_mysqli_setup_gtid_table($master_host_only, $user, $passwd, $db, $master_port, $master_socket))
  die(sprintf("SKIP Failed to setup GTID on master, %s\n", $error));

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
		),

		'lazy_connections' => 1,
		'trx_stickiness' => 'disabled',
		'filters' => array(
			"roundrobin" => array(),
		),
	),

);
if ($error = mst_create_config("test_mysqlnd_ms_get_last_gtid.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_get_last_gtid.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	/* autocommit, master */
	if (!$link->query("DROP TABLE IF EXISTS test"))
		printf("[002] [%d] %s\n", $link->errno, $link->error);

	if (1 != ($gtid = mysqlnd_ms_get_last_gtid($link))) {
		printf("[003] Expecting 1, got %s\n", var_export($gtid, true));
	} else {
		printf("[004] [%d] %s\n", $link->errno, $link->error);
	}
	$last_gtid = $gtid;

	$link->autocommit(false);

	if (!$link->query("CREATE TABLE test(id INT)"))
		printf("[005] [%d] %s\n", $link->errno, $link->error);

	if ($last_gtid !== ($gtid = mysqlnd_ms_get_last_gtid($link)))
		printf("[006] Expecting %s got %s, [%d] %s\n", $last_gtid, $gtid, $link->errno, $link->error);

	if (!$link->rollback())
		printf("[007] [%d] %s\n", $link->errno, $link->error);

	if (!$link->commit())
		printf("[008] [%d] %s\n", $link->errno, $link->error);

	if (false === ($gtid = mysqlnd_ms_get_last_gtid($link)))
		printf("[009] [%d] %s\n", $last_gtid, $gtid, $link->errno, $link->error);

	if ($gtid <= $last_gtid)
		printf("[010] last %s, new %s\n", $last_gtid, $gtid);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_get_last_gtid.ini"))
		printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_get_last_gtid.ini'.\n");

	require_once("connect.inc");
	require_once("util.inc");
	if ($error = mst_mysqli_drop_test_table($master_host_only, $user, $passwd, $db, $master_port, $master_socket))
		printf("[clean] %s\n");

	if ($error = mst_mysqli_drop_gtid_table($master_host_only, $user, $passwd, $db, $master_port, $master_socket))
		printf("[clean] %s\n", $error));


?>
--EXPECTF--
[004] [0%s
done!