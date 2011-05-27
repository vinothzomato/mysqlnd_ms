--TEST--
Limits: connection options not handled
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host),
	),
);
if ($error = create_config("test_mysqlnd_ms_connection_options.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_connection_options.ini
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

	$link = mysqli_init();
	if (!$link->options(MYSQLI_INIT_COMMAND, "SET @myinitcommand = 'something'"))
		printf("[001] Cannot set init command, [%d] %s\n", $link->errno, $link->error);

	if (!my_mysqli_real_connect($link, "myapp", $user, $passwd, $db, $port, $socket))
		printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	/* master may have it because MS prototype used to connect to the master first */
	if (!($res = run_query(3, $link, "SELECT @myinitcommand AS _myinit", MYSQLND_MS_MASTER_SWITCH)))
		printf("[004] [%d] %s\n", $link->errno, $link->error);

	if (!$row = $res->fetch_assoc())
		printf("[005] [%d] %s\n", $link->errno, $link->error);

	if ('something' != $row['_myinit'])
		printf("[006] Expecting 'something' got '%s'\n", $row['_myinit']);

	if (!($res = run_query(7, $link, "SELECT @myinitcommand AS _myinit", MYSQLND_MS_SLAVE_SWITCH)))
		printf("[008] [%d] %s\n", $link->errno, $link->error);

	if (!$row = $res->fetch_assoc())
		printf("[009] [%d] %s\n", $link->errno, $link->error);

	if ('something' != $row['_myinit'])
		printf("[010] Expecting 'something' got '%s'\n", $row['_myinit']);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_connection_options.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_connection_options.ini'.\n");
?>
--EXPECTF--
[010] Expecting 'something' got ''
done!