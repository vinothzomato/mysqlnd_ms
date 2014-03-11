--TEST--
Fabric: sharding.lookup_servers command + warn about trx boundaries off
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

if (!getenv("MYSQL_TEST_FABRIC")) {
	die(sprintf("SKIP Fabric - set MYSQL_TEST_FABRIC=1 (config.inc) to enable\n"));
}

$settings = array(
	"myapp" => array(
		'fabric' => array(
			'hosts' => array(
						array('host' => '127.0.0.1', 'port' => '8080')
					),
		),
		'trx_stickiness' => 'on',
	),
);
if ($error = mst_create_config("test_mysqlnd_ms_fabric_warn_trx_boundaries_off.ini", $settings))
  die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.config_file=test_mysqlnd_ms_fabric_warn_trx_boundaries_off.ini
mysqlnd_ms.collect_statistics=1
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (0 !== mysqli_connect_errno())
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	/* Would be surprised if anybody set a mapping for table=DUAL... */
	@mysqlnd_ms_fabric_select_global($link, 1);
	$link->begin_transaction();
	@$link->query("DROP TABLE IF EXISTS test");
	mysqlnd_ms_fabric_select_global($link, 1);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_fabric_warn_trx_boundaries_off.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_fabric_warn_trx_boundaries_off.ini'.\n");
?>
--EXPECTF--
done!
