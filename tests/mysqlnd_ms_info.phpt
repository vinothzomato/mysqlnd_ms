--TEST--
Thread id
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
		'pick' => array("roundrobin"),
	),
);
if ($error = create_config("test_mysqlnd_ms_info.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_info.ini
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

	$threads = array();

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (0 !== mysqli_connect_errno())
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	run_query(2, $link, "DROP TABLE IF EXISTS test", MYSQLND_MS_MASTER_SWITCH);
	run_query(3, $link, "CREATE TABLE test(id INT)", MYSQLND_MS_LAST_USED_SWITCH);
	run_query(4, $link, "INSERT INTO test(id) VALUES (1), (2), (3)", MYSQLND_MS_LAST_USED_SWITCH);
	$threads[$link->thread_id] = array('role' => 'master', 'info' => mysqli_info($link));
	run_query(5, $link, "DROP TABLE IF EXISTS test", MYSQLND_MS_LAST_USED_SWITCH);

	run_query(6, $link, "DROP TABLE IF EXISTS test", MYSQLND_MS_SLAVE_SWITCH);
	run_query(7, $link, "CREATE TABLE test(id INT)", MYSQLND_MS_LAST_USED_SWITCH);
	run_query(8, $link, "INSERT INTO test(id) VALUES (1), (2), (3)", MYSQLND_MS_LAST_USED_SWITCH);
	$threads[$link->thread_id] = array('role' => 'slave 1', 'info' => mysqli_info($link));
	run_query(9, $link, "DROP TABLE IF EXISTS test", MYSQLND_MS_LAST_USED_SWITCH);

	run_query(10, $link, "DROP TABLE IF EXISTS test", MYSQLND_MS_SLAVE_SWITCH);
	run_query(11, $link, "CREATE TABLE test(id INT)", MYSQLND_MS_LAST_USED_SWITCH);
	run_query(12, $link, "INSERT INTO test(id) VALUES (1), (2), (3)", MYSQLND_MS_LAST_USED_SWITCH);
	$threads[$link->thread_id] = array('role' => 'slave 2', 'info' => mysqli_info($link));
	run_query(13, $link, "DROP TABLE IF EXISTS test", MYSQLND_MS_LAST_USED_SWITCH);

	foreach ($threads as $thread_id => $info) {
		printf("%d - %s - '%s'\n", $thread_id, $info['role'], $info['info']);
		if ('' == $info['info'])
			printf("info should not be empty. Check manually.\n");
	}


	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_info.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_info.ini'.\n");
?>
--EXPECTF--
%d - master - '%s'
%d - slave 1 - '%s'
%d - slave 2 - '%s'
done!
