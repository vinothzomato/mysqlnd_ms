--TEST--
last query field count (without PS)
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_field_count
--SKIPIF--
<?php
require_once("skipif.inc");
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
		'pick' => array("roundrobin"),

	),
);
if ($error = create_config("test_mysqlnd_field_count", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());


	$threads = array();
	run_query(2, $link, "SET @myrole='Slave 1'", MYSQLND_MS_SLAVE_SWITCH);
	if (0 !== $link->field_count)
		printf("[003] Expecting 0 got field_count = %d\n", $link->field_count);

	$res = run_query(4, $link, "SELECT @myrole AS _role", MYSQLND_MS_LAST_USED_SWITCH);
	$row = $res->fetch_assoc();
	if ($res->field_count != $link->field_count)
		printf("[005] res->field_count = %d, link->field_count = %d\n", $res->field_count, $link->field_count);
	$threads[$link->thread_id] = array("role" => $row['_role'], "fields" => $res->field_count);
	$res->close();

	run_query(6, $link, "SET @myrole='Master 1'");
	if (0 !== $link->field_count)
		printf("[007] Expecting 0 got field_count = %d\n", $link->field_count);

	$res = run_query(8, $link, "SELECT @myrole AS _role, 1 as _one", MYSQLND_MS_LAST_USED_SWITCH);
	$row = $res->fetch_assoc();
	if ($res->field_count != $link->field_count)
		printf("[009] res->field_count = %d, link->field_count = %d\n", $res->field_count, $link->field_count);
	$threads[$link->thread_id] = array("role" => $row['_role'], "fields" => $res->field_count);
	$res->close();


	run_query(10, $link, "SET @myrole='Slave 2'", MYSQLND_MS_SLAVE_SWITCH);
	if (0 !== $link->field_count)
		printf("[011] Expecting 0 got field_count = %d\n", $link->field_count);

	$res = run_query(12, $link, "SELECT @myrole AS _role, 1 AS _one, 2 as _two", MYSQLND_MS_LAST_USED_SWITCH);
	$row = $res->fetch_assoc();
	if ($res->field_count != $link->field_count)
		printf("[013] res->field_count = %d, link->field_count = %d\n", $res->field_count, $link->field_count);
	$threads[$link->thread_id] = array("role" => $row['_role'], "fields" => $res->field_count);
	$res->close();

	run_query(14, $link, "SET @myrole='Slave 1'", MYSQLND_MS_SLAVE_SWITCH);
	if (0 !== $link->field_count)
		printf("[015] Expecting 0 got field_count = %d\n", $link->field_count);

	foreach ($threads as $thread_id => $details)
		printf("%d - %s: %d\n", $thread_id, $details["role"], $details["fields"]);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_field_count"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_field_count'.\n");
?>
--EXPECTF--
%d - Slave 1: 1
%d - Master 1: 2
%d - Slave 2: 3
done!