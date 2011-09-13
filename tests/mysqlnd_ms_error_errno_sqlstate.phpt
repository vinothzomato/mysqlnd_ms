--TEST--
error, errno, sqlstate
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);


if (!function_exists("iconv"))
	die("SKIP needs iconv extension\n");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
		'pick' => array("roundrobin"),
	),
);
if ($error = mst_create_config("test_mysqlnd_ms_error_errno_sqlstate.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_error_errno_sqlstate.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	if (!($link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$threads = array();

	if (mst_mysqli_query(10, $link, "role=master I_HOPE_THIS_IS_INVALID_SQL")) {
		printf("[011] Query should have failed\n");
	} else {
		$threads[$link->thread_id] = array(
			"role"		=> "master",
			"errno" 	=> $link->errno,
			"error"		=> $link->error,
			"sqlstate"	=> $link->sqlstate,
		);
	}

	if (mst_mysqli_query(20, $link, "role=slave1 I_HOPE_THIS_IS_INVALID_SQL", MYSQLND_MS_SLAVE_SWITCH)) {
		printf("[021] Query should have failed\n");
	} else {
		$threads[$link->thread_id] = array(
			"role"		=> "slave1",
			"errno" 	=> $link->errno,
			"error"		=> $link->error,
			"sqlstate"	=> $link->sqlstate,
		);
	}

	if (mst_mysqli_query(30, $link, "role=slave2 I_HOPE_THIS_IS_INVALID_SQL", MYSQLND_MS_SLAVE_SWITCH)) {
		printf("[031] Query should have failed\n");
	} else {
		$threads[$link->thread_id] = array(
			"role"		=> "slave2",
			"errno" 	=> $link->errno,
			"error"		=> $link->error,
			"sqlstate"	=> $link->sqlstate,
		);
	}

	foreach ($threads as $thread_id => $details) {
		printf("%d - %s\n", $thread_id, $details['role']);
		if (!$details['errno'])
			printf("[032] Errno missing\n");

		if (!$details['error'])
			printf("[033] Error message missing\n");

		if (false === stristr($details['error'], "role=" . $details['role']))
			printf("[034] Suspicious error message '%s'. Check manually, update test, if need be,", $details['error']);

		if ('00000' == $details['sqlstate'] || !$details['sqlstate'])
			printf("[034] Suspicious sqlstate '%s'. Check manually, update test, if need be,", $details['sqlstate']);

	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_error_errno_sqlstate.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_error_errno_sqlstate.ini'.\n");
?>
--EXPECTF--
[010] [1064] You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near 'role=master I_HOPE_THIS_IS_INVALID_SQL' at line 1
[020] [1064] You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near 'role=slave1 I_HOPE_THIS_IS_INVALID_SQL' at line 1
[030] [1064] You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near 'role=slave2 I_HOPE_THIS_IS_INVALID_SQL' at line 1
%d - master
%d - slave1
%d - slave2
done!