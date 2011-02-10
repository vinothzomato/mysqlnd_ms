--TEST--
Config settings: pick server = user, fallback
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
		'slave' => array($slave_host),
		'pick' 	=> array('user'),
	),
);
if ($error = create_config("test_mysqlnd_ms_pick_server_fallback.ini", $settings))
  die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_pick_server_fallback.ini
--FILE--
<?php
	require_once("connect.inc");

	function pick_server($connected_host, $query, $master, $slaves, $last_used_connection) {
		printf("%s\n", $query);
		/* should default to build-in pick logic */
		return -1;
	}

	if (!$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket))
		printf("[001] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n",
			$host, $user, $db, $port, $socket);

	$query = sprintf("/*%s*/SELECT CONNECTION_ID() as _master FROM DUAL", MYSQLND_MS_MASTER_SWITCH);
	if (!$res = $link->query($query))
		printf("[002] [%d] %s\n", $link->errno, $link->error);
	$row = $res->fetch_assoc();
	var_dump($row);
	$master = $link->thread_id;

	$query = sprintf("/*%s*/SELECT CONNECTION_ID() as _slave FROM DUAL", MYSQLND_MS_SLAVE_SWITCH);
	if (!$res = $link->query($query))
		printf("[003] [%d] %s\n", $link->errno, $link->error);
	$row = $res->fetch_assoc();
	var_dump($row);
	$slave = $link->thread_id;
	if ($link->thread_id == $master) {
		printf("[004] Master and slave query seem to use the same connection\n");
	 }

	$query = sprintf("/*%s*/SELECT CONNECTION_ID() as _last_used FROM DUAL", MYSQLND_MS_LAST_USED_SWITCH);
	if (!$res = $link->query($query))
		printf("[005] [%d] %s\n", $link->errno, $link->error);
	$row = $res->fetch_assoc();
	var_dump($row);
	if ($link->thread_id != $slave) {
		printf("[006] Connection has been switched although last_used_switch SQL hint has been used\n");
	 }


	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_pick_server_fallback.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_pick_server_fallback.ini'.\n");
?>
--EXPECTF--
array(1) {
  ["_master"]=>
  string(%d) "%d"
}
array(1) {
  ["_slave"]=>
  string(%d) "%d"
}
array(1) {
  ["_last_used"]=>
  string(%d) "%d"
}
done!