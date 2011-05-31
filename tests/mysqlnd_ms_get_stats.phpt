--TEST--
mysqlnd_ms_get_stats()
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host),
		'lazy_connections' => 0
	),
);
if ($error = create_config("test_mysqlnd_ms_get_stats.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_get_stats.ini
mysqlnd_ms.collect_statistics=1
--FILE--
<?php
	require_once("connect.inc");


	$expected = array(
		"use_slave" 							=> true,
		"use_master"							=> true,
		"use_slave_sql_hint"					=> true,
		"use_master_sql_hint"					=> true,
		"use_last_used_sql_hint" 				=> true,
		"use_slave_callback"					=> true,
		"use_master_callback"					=> true,
		"non_lazy_connections_slave_success"	=> true,
		"non_lazy_connections_slave_failure"	=> true,
		"non_lazy_connections_master_success"	=> true,
		"non_lazy_connections_master_failure"	=> true,
		"lazy_connections_slave_success"		=> true,
		"lazy_connections_slave_failure"		=> true,
		"lazy_connections_master_success"		=> true,
		"lazy_connections_master_failure"		=> true,
		"trx_autocommit_on"						=> true,
		"trx_autocommit_off"					=> true,
		"trx_master_forced"						=> true,
	);

	function run_query($offset, $link, $query, $switch = NULL) {
		if ($switch)
			$query = sprintf("/*%s*/%s", $switch, $query);

		if (!($ret = $link->query($query)))
			printf("[%03d] [%d] %s\n", $offset, $link->errno, $link->error);

		return $ret;
	}

	if (NULL !== ($ret = @mysqlnd_ms_get_stats(123))) {
		printf("[001] Expecting NULL got %s/%s\n", gettype($ret), $ret);
	}

	$stats = mysqlnd_ms_get_stats();
	$exp_stats = $stats;

	foreach ($expected as $k => $v) {
		if (!isset($stats[$k])) {
			printf("[002] Statistic '%s' missing\n", $k);
		} else {
			unset($stats[$k]);
		}
	}
	if (!empty($stats)) {
		printf("[003] Dumping unknown statistics\n");
		var_dump($stats);
	}

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[004] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$exp_stats['non_lazy_connections_slave_success']++;
	$exp_stats['non_lazy_connections_master_success']++;

	run_query(5, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH);
	$exp_stats['use_master_sql_hint']++;

	run_query(6, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);
	$exp_stats['use_slave_sql_hint']++;

	$res = run_query(7, $link, "SELECT @myrole AS _role");
	$exp_stats['use_slave']++;

	$row = $res->fetch_assoc();
	$res->close();
	if ($row['_role'] != 'slave')
		printf("[008] Expecting role = slave got role = '%s'\n", $row['_role']);

	$stats = mysqlnd_ms_get_stats();
	foreach ($stats as $k => $v) {
		if ($exp_stats[$k] != $v) {
			printf("[009] Expecting %s = '%s', got '%s'\n", $k, $exp_stats[$k], $v);
		}
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_get_stats.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_get_stats.ini'.\n");
?>
--EXPECTF--
done!