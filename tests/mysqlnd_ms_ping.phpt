--TEST--
ping
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

if ($master_host == $slave_host) {
	die("SKIP master and slave seem to the the same, see tests/README");
}

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
	),
);
if ($error = create_config("test_mysqlnd_ms_ping.ini", $settings))
	die(sprintf("SKIP %d\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_ping.ini
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

	run_query(2, $link, "SET @myrole='Slave 1'", MYSQLND_MS_SLAVE_SWITCH);
	run_query(3, $link, "SET @myrole='Slave 2'", MYSQLND_MS_SLAVE_SWITCH);
	run_query(4, $link, "SET @myrole='Master 1'");

	if (!$link->ping())
		printf("[005] [%d] %s\n", $link->errno, $link-error);

	/* although we kill the master connection, the slaves are still reachable */
	if (!$link->kill($link->thread_id))
		printf("[006] [%d] %s\n", $link->errno, $link->error);

	if ($link->ping())
		printf("[007] Master connection is still alive\n");

	if (run_query(8, $link, "SET @myrole='Master 1'"))
		printf("[008] Master connection can still run queries\n");

	$res = run_query(9, $link, "SELECT @myrole AS _role");

	if (!$res || !($row = $res->fetch_assoc()))
		printf("[010] Slave connections should be still usable, [%d] %s\n",
			$link->errno, $link->error);

	if ($row['_role'] != "Slave 1")
		printf("[011] Expecting 'Slave 1' got '%s'\n", $row['_role']);

	if (!$link->ping())
		printf("[012] [%d] %s\n", $link->errno, $link->error);

	if (!$link->kill($link->thread_id))
		printf("[013] [%d] %s\n", $link->errno, $link->error);

	if ($link->ping())
		printf("[014] Slave connection is still alive\n");


	$res = run_query(15, $link, "SELECT @myrole AS _role");

	if (!$res || !($row = $res->fetch_assoc()))
		printf("[016] Slave connections should be still usable, [%d] %s\n",
		  $link->errno, $link->error);

	if ($row['_role'] != "Slave 2")
		printf("[017] Expecting 'Slave 2' got '%s'\n", $row['_role']);

	if (!$link->ping())
		printf("[018] [%d] %s\n", $link->errno, $link->error);

	if (!$link->kill($link->thread_id))
		printf("[019] [%d] %s\n", $link->errno, $link->error);

	if ($link->ping())
		printf("[020] Slave connection is still alive\n");

	print "done!";

?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_ping.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_ini_force_config.ini'.\n");
?>
--EXPECTF--
[008] %s
[014] %s
[018] %s
done!