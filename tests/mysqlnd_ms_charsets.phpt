--TEST--
Charsets - covered by plugin prototype
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");


$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host),
	),
);
if ($error = create_config("test_mysqlnd_ms_charsets.ini", $settings))
	die(sprintf("SKIP %d\n", $error));

function test_for_charset($host, $user, $passwd, $db, $port, $socket) {
	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket))
		die(sprintf("skip Cannot connect, [%d] %s", mysqli_connect_errno(), mysqli_connect_error()));

	if (!($res = mysqli_query($link, 'SELECT version() AS server_version')) ||
			!($tmp = mysqli_fetch_assoc($res))) {
		mysqli_close($link);
		die(sprintf("skip Cannot check server version, [%d] %s\n",
		mysqli_errno($link), mysqli_error($link)));
	}
	mysqli_free_result($res);
	$version = explode('.', $tmp['server_version']);
	if (empty($version)) {
		mysqli_close($link);
		die(sprintf("skip Cannot check server version, based on '%s'",
			$tmp['server_version']));
	}

	if ($version[0] <= 4 && $version[1] < 1) {
		mysqli_close($link);
		die(sprintf("skip Requires MySQL Server 4.1+\n"));
	}

	if ((($res = mysqli_query($link, 'SHOW CHARACTER SET LIKE "latin1"', MYSQLI_STORE_RESULT)) &&
			(mysqli_num_rows($res) == 1)) ||
			(($res = mysqli_query($link, 'SHOW CHARACTER SET LIKE "latin2"', MYSQLI_STORE_RESULT)) &&
			(mysqli_num_rows($res) == 1))
			) {
		// ok, required latin1 or latin2 are available
	} else {
		die(sprintf("skip Requires character set latin1 or latin2\n"));
	}

	if (!$res = mysqli_query($link, 'SELECT @@character_set_connection AS charset'))
		die(sprintf("skip Cannot select current charset, [%d] %s\n", $link->errno, $link->error));

	if (!$row = mysqli_fetch_assoc($res))
		die(sprintf("skip Cannot detect current charset, [%d] %s\n", $link->errno, $link->error));

	return $row['charset'];
}

$master_charset = test_for_charset($master_host, $user, $passwd, $db, $master_port, $master_socket);
$slave_charset = test_for_charset($slave_host, $user, $passwd, $db, $slave_port, $slave_socket);

if ($master_charset != $slave_charset) {
	die(sprintf("skip Master (%s) and slave (%s) must use the same default charset.", $master_charset, $slave_charset));
}
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_charsets.ini
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

	/*
	Charset changes made through the user API are applied to all
	connections. This is important and a security must because string
	escaping depends on the current charset. If the plugin would
	not take care of it, the following sequence would bear a security risk.

	INSERT - master, charset latin1
    $somestring = escape_string(link, "somestring") - would use latin1 from last used connection
    SELECT * FROM test WHERE column = $somestring - slave, slave may be using latin2, $somestring should have been escaped using latin2

	*/


	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	run_query(2, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH);
	run_query(3, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);

	/* slave */
	if (!$res = run_query(4, $link, "SELECT @@character_set_connection AS charset", MYSQLND_MS_LAST_USED_SWITCH))
		printf("[005] [%d] %s\n", $link->errno, $link->error);

	if (!$row = $res->fetch_assoc())
		printf("[006] [%d] %s\n", $link->errno, $link->error);

	$current_charset = $row['charset'];
	$new_charset = ('latin1' == $current_charset) ? 'latin2' : 'latin1';

	/* shall be run on *all* configured machines - all masters, all slaves */
	$link->set_charset($new_charset);

	/* slave */
	if (!$res = run_query(7, $link, "SELECT @@character_set_connection AS charset", MYSQLND_MS_LAST_USED_SWITCH))
		printf("[008] [%d] %s\n", $link->errno, $link->error);

	if (!$row = $res->fetch_assoc())
		printf("[009] [%d] %s\n", $link->errno, $link->error);

	$current_charset = $row['charset'];
	if ($current_charset != $new_charset)
		printf("[010] Expecting charset '%s' got '%s'\n", $new_charset, $current_charset);


	if (!$res = run_query(11, $link, "SELECT @@character_set_connection AS charset", MYSQLND_MS_MASTER_SWITCH))
		printf("[012] [%d] %s\n", $link->errno, $link->error);

	if (!$row = $res->fetch_assoc())
		printf("[013] [%d] %s\n", $link->errno, $link->error);

	$current_charset = $row['charset'];
	if ($current_charset != $new_charset)
		printf("[014] Expecting charset '%s' got '%s'\n", $new_charset, $current_charset);

	print "done!";

?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_charsets.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_charsets.ini'.\n");
?>
--EXPECTF--
done!