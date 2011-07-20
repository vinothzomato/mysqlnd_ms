--TEST--
Use master on write (pick = random_once)
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	$host => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
		'master_on_write' => 1,
		'pick' => array("random" => array("sticky" => "1")),
	),
);
if ($error = create_config("test_mysqlnd_ms_master_on_write.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_master_on_write.ini
mysqlnd_ms.collect_statistics=1
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	compare_stats();
	if (!($link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	run_query(2, $link, "SET @myrole='Slave 1'", MYSQLND_MS_SLAVE_SWITCH);
	compare_stats();

	$res = run_query(3, $link, "SELECT @myrole AS _role");
	$row = $res->fetch_assoc();
	$res->close();
	printf("This is '%s' speaking\n", $row['_role']);
	compare_stats();
	/* not a select -> master query */
	run_query(4, $link, "SET @myrole='Master 1'");
	compare_stats();

	/* master on write is active, master should reply */
	$res = run_query(5, $link, "SELECT @myrole AS _role");
	$row = $res->fetch_assoc();
	$res->close();
	printf("This is '%s' speaking\n", $row['_role']);
	compare_stats();

	/* SQL hint wins */
	$res = run_query(6, $link, "SELECT @myrole AS _role",  MYSQLND_MS_SLAVE_SWITCH);
	$row = $res->fetch_assoc();
	$res->close();
	printf("This is '%s' speaking\n", $row['_role']);
	compare_stats();

	/* master on write is active, master should reply */
	$res = run_query(7, $link, "SELECT @myrole AS _role");
	$row = $res->fetch_assoc();
	$res->close();
	printf("This is '%s' speaking\n", $row['_role']);

	/* SQL hint wins */
	$res = run_query(8, $link, "SELECT @myrole AS _role",  MYSQLND_MS_SLAVE_SWITCH);
	$row = $res->fetch_assoc();
	$res->close();
	printf("This is '%s' speaking\n", $row['_role']);

	/* SQL hint wins */
	$res = run_query(8, $link, "SELECT @myrole AS _role",  MYSQLND_MS_LAST_USED_SWITCH);
	$row = $res->fetch_assoc();
	$res->close();
	printf("This is '%s' speaking\n", $row['_role']);

	print "done!";

?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_master_on_write.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_master_on_write.ini'.\n");
?>
--EXPECTF--
Stats use_slave_sql_hint: 1
Stats lazy_connections_slave_success: 1
This is 'Slave 1' speaking
Stats use_slave: 1
Stats use_master: 1
Stats lazy_connections_master_success: 1
This is 'Master 1' speaking
Stats use_slave: 2
This is 'Slave 1' speaking
Stats use_slave_sql_hint: 2
This is 'Master 1' speaking
This is 'Slave 1' speaking
This is 'Slave 1' speaking
done!