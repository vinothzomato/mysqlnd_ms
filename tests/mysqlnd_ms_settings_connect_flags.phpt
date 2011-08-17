--TEST--
Connection flags
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

$settings = array(
	"myapp" => array(
		'master' => array(
			  array(
				'host' 		=> $master_host_only,
				'port' 		=> (int)$master_port,
				'socket' 	=> $master_socket,
				'db'		=> (string)$db,
				'user'		=> $user,
				'password'	=> $passwd,
				'connect_flags' => 0,
			  ),
		),
		'slave' => array(
			array(
			  'host' 	=> $slave_host_only,
			  'port' 	=> (double)$slave_port,
			  'socket' 	=> $slave_socket,
			  'db'		=> $db,
			  'user'	=> $user,
			  'password'=> $passwd,
			  'connect_flags' => 0,
			),
			array(
			  'host' 	=> $slave_host_only,
			  'port' 	=> (double)$slave_port,
			  'socket' 	=> $slave_socket,
			  'db'		=> $db,
			  'user'	=> $user,
			  'password'=> $passwd,
			  'connect_flags' => MYSQLI_CLIENT_FOUND_ROWS
			),
		),
		'pick' => 'roundrobin',
		'lazy_connections' => 0,
	),
);
if ($error = create_config("test_mysqlnd_ms_settings_connect_flags.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_settings_connect_flags.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	create_test_table($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

	/* note that user etc are to be taken from the config! */
	if (!($link = my_mysqli_connect("myapp", NULL, NULL, NULL, NULL, NULL)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$threads = array();

	run_query(2, $link, "UPDATE test SET id = 1 WHERE id = 1", MYSQLND_MS_SLAVE_SWITCH);
	printf("Slave 1 - affected rows: %d\n", $link->affected_rows);
	$threads[$link->thread_id] = "Slave 1 . I";

	run_query(3, $link, "UPDATE test SET id = 1 WHERE id = 1", MYSQLND_MS_SLAVE_SWITCH);
	printf("Slave 2 - affected rows: %d\n", $link->affected_rows);
	$threads[$link->thread_id] = "Slave 2";

	run_query(4, $link, "DROP TABLE IF EXISTS test");
	$threads[$link->thread_id] = "Master";

	run_query(4, $link, "DROP TABLE IF EXISTS test", MYSQLND_MS_SLAVE_SWITCH);
	$threads[$link->thread_id] = "Slave 1 - II";

	foreach ($threads as $id => $role)
		printf("%s - %d\n", $role, $id);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_settings_connect_flags.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_settings_connect_flags.ini'.\n");
?>
--EXPECTF--
Slave 1 - affected rows: 0
Slave 2 - affected rows: 1
Slave 1 - II - %d
Slave 2 - %d
Master - %d
done!