--TEST--
Global Transaction ID Injection
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

msg_mysqli_init_emulated_id_skip($slave_host, $user, $passwd, $db, $slave_port, $slave_socket, "slave");
msg_mysqli_init_emulated_id_skip($master_host, $user, $passwd, $db, $master_port, $master_socket, "master");

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
		),

		'lazy_connections' => 1,
		'trx_stickiness' => 'disabled',
		'filters' => array(
			"roundrobin" => array(),
		),
	),

);
if ($error = mst_create_config("test_mysqlnd_ms_gtid_basics.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_gtid_basics.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}
	/* we need an extra non-MS link for checking GTID. If we use MS link, the check itself will change GTID */
	$master_link = mst_mysqli_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);

	/* auto commit on (default) */
	$gtid = mst_mysqli_fetch_gtid(3, $master_link, $db);
	mst_mysqli_query(4, $link, "SET @myrole = 'Master'");
	$new_gtid = mst_mysqli_fetch_gtid(6, $master_link, $db);
	if ($new_gtid <= $gtid) {
		printf("[007] GTID has not been incremented on master in auto commit mode\n");
	}
	$gtid = $new_gtid;

	/* auto commit off */
	$link->autocommit(FALSE);

	/* run on master. Thus, must increment GTID - regardless if SELECT or not! */
	$res = mst_mysqli_query(8, $link, "SELECT @myrole AS _role FROM DUAL", MYSQLND_MS_MASTER_SWITCH);
	if (!$res) {
		printf("[010] [%d] %s\n", $link->errno, $link->error);
	}
	$row = $res->fetch_assoc();
	printf("Heho from '%s'\n", $row['_role']);
	$new_gtid = mst_mysqli_fetch_gtid(12, $master_link, $db);
	if ($new_gtid != $gtid) {
		printf("[013] GTID has changed in non auto commit without commit\n");
	}
	$gtid = $new_gtid;

	$link->commit();
	$new_gtid = mst_mysqli_fetch_gtid(14, $master_link, $db);
	if ($new_gtid <= $gtid) {
		printf("[016] GTID has been not been incremented after commit\n");
	}
	$gtid = $new_gtid;

	/* we are in manual commit mode, increment on *every* commit */
	$link->commit();
	$new_gtid = mst_mysqli_fetch_gtid(17, $master_link, $db);
	if ($new_gtid <= $gtid) {
		printf("[019] GTID has been not been incremented after commit\n");
	}
	$gtid = $new_gtid;

	/* back to auto commit */
	$link->autocommit(TRUE);
	/* commit shall be ignored, it is not needed. We increment after every master statement */
	$link->commit();
	$new_gtid = mst_mysqli_fetch_gtid(20, $master_link, $db);
	if ($new_gtid != $gtid) {
		printf("[022] commit() shall not change GTID in auto commit mode\n");
	}
	$gtid = $new_gtid;

	/* increment, tested above ...*/
	$res = mst_mysqli_query(23, $link, "SELECT @myrole AS _role FROM DUAL", MYSQLND_MS_MASTER_SWITCH);
	if (!$res) {
		printf("[025] [%d] %s\n", $link->errno, $link->error);
	}
	$row = $res->fetch_assoc();
	printf("Heho from '%s'\n", $row['_role']);
	$gtid = mst_mysqli_fetch_gtid(26, $master_link, $db);

	/* commit shall be ignored, it is not needed. We increment after every master statement */
	$link->commit();
	$new_gtid = mst_mysqli_fetch_gtid(28, $master_link, $db);
	if ($new_gtid != $gtid) {
		printf("[029] commit() shall not change GTID in auto commit mode\n");
	}
	$gtid = $new_gtid;

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_gtid_basics.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_gtid_basics.ini'.\n");
?>
--EXPECTF--
Heho from 'Master'
Heho from 'Master'
done!