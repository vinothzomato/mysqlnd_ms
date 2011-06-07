--TEST--
Per host credentials
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array(
			  array(
				'host' 	=> $master_host_only,
				'port' 	=> $master_port,
				'socket' 	=> $master_socket,
				'db'		=> $db,
				'user'	=> $user,
				'password'=> $passwd,
			  ),
		),
		'slave' => array(
			array(
			  'host' 	=> $slave_host_only,
			  'port' 	=> $slave_port,
			  'socket' 	=> $slave_socket,
			  'db'		=> $db,
			  'user'	=> $user,
			  'password'=> $passwd,
			),
		),
		'pick' => 'roundrobin',
		'lazy_connections' => 0,
	),
);
if ($error = create_config("test_mysqlnd_ms_settings_host_credentials.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_settings_host_credentials.ini
--FILE--
<?php
	require_once("connect.inc");

	function run_query($offset, $link, $query, $switch = NULL) {
		if ($switch)
			$query = sprintf("/*%s*/%s", $switch, $query);

		if (!$link->real_query($query))
			printf("[%03d + 01] [%d] %s\n", $offset, $link->errno, $link->error);

		if (!($res = $link->use_result()))
			printf("[%03d + 02] [%d] %s\n", $offset, $link->errno, $link->error);

		while ($row = $res->fetch_assoc())
			printf("[%03d + 03] '%s'\n", $offset, $row['_one']);
	}

	/* note that user etc are to be taken from the config! */
	if (!($link = my_mysqli_connect("myapp", NULL, NULL, NULL, NULL, NULL)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$threads = array();

	/* slave 1 */
	run_query(1, $link, "SELECT 1 AS _one FROM DUAL");
	$threads[$link->thread_id] = array('role' => 'Slave 1', 'stat' => $link->stat());

	/* master */
	run_query(2, $link, "SELECT 123 AS _one FROM DUAL", MYSQLND_MS_MASTER_SWITCH);
	$threads[$link->thread_id] = array('role' => 'Master', 'stat' => $link->stat());

	foreach ($threads as $thread_id => $details) {
		printf("%d - %s: '%s'\n", $thread_id, $details['role'], $details['stat']);
		if ('' == $details['stat'])
			printf("Server stat must not be empty!\n");
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_settings_host_credentials.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_settings_host_credentials.ini'.\n");
?>
--EXPECTF--
[001 + 03] '1'
[002 + 03] '123'
%d - Slave 1: '%s'
%d - Master: '%s'
done!