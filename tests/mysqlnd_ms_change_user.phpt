--TEST--
change_user() - covered by the prototype
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

if ($master_host == $slave_host) {
	die("SKIP master and slave seem to the the same, see tests/README");
}

if ($db == 'mysql')
	die("SKIP Default test database must not be 'mysql', use 'test' or the like");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host),
	),
);
if ($error = create_config("test_mysqlnd_ms_change_user.ini", $settings))
	die(sprintf("SKIP %d\n", $error));

function test_mysql_access($host, $user, $passwd, $db, $port, $socket) {

	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket))
		die(sprintf("skip Cannot connect, [%d] %s", mysqli_connect_errno(), mysqli_connect_error()));

	return $link->select_db("mysql");
}

if (!test_mysql_access($master_host, $user, $passwd, $db, $port, $socket))
	die("skip Master server account cannot access mysql database");

if (!test_mysql_access($slave_host, $user, $passwd, $db, $port, $socket))
	die("skip Slave server account cannot access mysql database");
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_change_user.ini
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

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno($link), mysqli_connect_error($link));

	run_query(2, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH);
	$master_thread = $link->thread_id;
	run_query(3, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);
	$slave_thread = $link->thread_id;

	$link->change_user($user, $passwd, 'mysql');

	$res = run_query(5, $link, "SELECT @myrole AS _role, DATABASE() as _database", MYSQLND_MS_SLAVE_SWITCH);
	if (!$row = $res->fetch_assoc())
		printf("[006] [%d] %s\n", $link->errno, $link->error);

	if ($row['_database'] != 'mysql')
		printf("[007] Expecting database 'mysql' got '%s'\n", $row['_database']);

	if ($row['_role'] != '')
		printf("[008] Expecting role '' got '%s'\n", $row['_role']);

	if ($link->thread_id != $slave_thread)
		printf("[009] Expecting slave connection thread id %d got %d\n", $slave_thread, $link->thread_id);

	$res = run_query(10, $link, "SELECT @myrole AS _role, DATABASE() as _database", MYSQLND_MS_MASTER_SWITCH);
	if (!$row = $res->fetch_assoc())
		printf("[011] [%d] %s\n", $link->errno, $link->error);

	if ($row['_database'] != 'mysql')
		printf("[012] Expecting database 'mysql' got '%s'\n", $row['_database']);

	if ($row['_role'] != '')
		printf("[013] Expecting role '' got '%s'\n", $row['_role']);

	if ($link->thread_id != $master_thread)
		printf("[009] Expecting master connection thread id %d got %d\n", $master_thread, $link->thread_id);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_change_user.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_ini_force_config.ini'.\n");
?>
--EXPECTF--
done!