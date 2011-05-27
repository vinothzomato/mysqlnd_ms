--TEST--
select_db() - covered by plugin prototype
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

if ($db == 'mysql')
	die("Default test database must not be 'mysql', use 'test' or the like");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host),
	),
);
if ($error = create_config("test_mysqlnd_ms_select_db.ini", $settings))
	die(sprintf("SKIP %d\n", $error));

function test_mysql_access($host, $user, $passwd, $db, $port, $socket) {

	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket))
		die(sprintf("skip Cannot connect, [%d] %s", mysqli_connect_errno(), mysqli_connect_error()));

	return $link->select_db("mysql");
}

if (!test_mysql_access($master_host_only, $user, $passwd, $db, $port, $socket))
	die("skip Master server account cannot access mysql database");

if (!test_mysql_access($slave_host_only, $user, $passwd, $db, $port, $socket))
	die("skip Slave server account cannot access mysql database");
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_select_db.ini
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
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	run_query(2, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH);
	run_query(3, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);

	/* will change schema of all connections */
	if (!$link->select_db("mysql"))
		printf("[004] [%d] %s\n", $link->errno, $link->error);

	$res = run_query(5, $link, "SELECT @myrole AS _role, DATABASE() as _database", MYSQLND_MS_SLAVE_SWITCH);
	if (!$row = $res->fetch_assoc())
		printf("[006] [%d] %s\n", $link->errno, $link->error);

	if ($row['_database'] != 'mysql')
		printf("[007] Expecting database 'mysql' got '%s'\n", $row['_database']);

	if ($row['_role'] != 'slave')
		printf("[008] Expecting role 'slave' got '%s'\n", $row['_role']);

	$res = run_query(9, $link, "SELECT @myrole AS _role, DATABASE() as _database", MYSQLND_MS_MASTER_SWITCH);
	if (!$row = $res->fetch_assoc())
		printf("[010] [%d] %s\n", $link->errno, $link->error);

	if ($row['_database'] != 'mysql')
		printf("[011] Expecting database 'mysql' got '%s'\n", $row['_database']);

	if ($row['_role'] != 'master')
		printf("[012] Expecting role 'master' got '%s'\n", $row['_role']);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_select_db.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_select_db.ini'.\n");
?>
--EXPECTF--
done!