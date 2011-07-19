--TEST--
error, errno, sqlstate
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

if (!function_exists("iconv"))
	die("SKIP needs iconv extension\n");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
		'pick' => array("roundrobin"),
	),
);
if ($error = create_config("test_mysqlnd_error_errno_sqlstate.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_error_errno_sqlstate.ini
--FILE--
<?php
	require_once("connect.inc");

	function run_query($offset, $link, $query, $switch = NULL) {
		if ($switch)
			$query = sprintf("/*%s*/%s", $switch, $query);

		return $link->query($query);
	}

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$threads = array();

	if (run_query(10, $link, "role=master I_HOPE_THIS_IS_INVALID_SQL")) {
		printf("[011] Query should have failed\n");
	} else {
		$threads[$link->thread_id] = array(
			"role"		=> "master",
			"errno" 	=> $link->errno,
			"error"		=> $link->error,
			"sqlstate"	=> $link->sqlstate,
		);
	}

	if (run_query(20, $link, "role=slave1 I_HOPE_THIS_IS_INVALID_SQL", MYSQLND_MS_SLAVE_SWITCH)) {
		printf("[021] Query should have failed\n");
	} else {
		$threads[$link->thread_id] = array(
			"role"		=> "slave1",
			"errno" 	=> $link->errno,
			"error"		=> $link->error,
			"sqlstate"	=> $link->sqlstate,
		);
	}

	if (run_query(30, $link, "role=slave2 I_HOPE_THIS_IS_INVALID_SQL", MYSQLND_MS_SLAVE_SWITCH)) {
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
	if (!unlink("test_mysqlnd_error_errno_sqlstate.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_error_errno_sqlstate.ini'.\n");
?>
--EXPECTF--
%d - master
%d - slave1
%d - slave2
done!