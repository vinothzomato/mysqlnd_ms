--TEST--
Per host credentials
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
			'master1' => array(
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

			array(
			  'host' 	=> $slave_host,
			),
		),
		'pick' => 'roundrobin',
		'lazy_connections' => 1,
	),
);
if ($error = create_config("test_mysqlnd_ms_settings_host_credentials_inherit.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_settings_host_credentials_inherit.ini
--FILE--
<?php
	require_once("connect.inc");

	function run_query($offset, $link, $query, $switch = NULL) {
		if ($switch)
			$query = sprintf("/*%s*/%s", $switch, $query);

		if (!($ret = $link->query($query)))
			printf("[%03d + 01] [%d] %s\n", $offset, $link->errno, $link->error);

		return $ret;
	}

	/* note that user etc are to be taken from the config! */
	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, NULL, NULL)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$threads = array();

	/* slave 1 */
	run_query(1, $link, "SELECT 1 AS _one FROM DUAL");
	$threads[$link->thread_id] = array('role' => 'Slave 1', 'stat' => $link->stat());

	/* master */
	run_query(2, $link, "SELECT 123 AS _one FROM DUAL", MYSQLND_MS_MASTER_SWITCH);
	$threads[$link->thread_id] = array('role' => 'Master', 'stat' => $link->stat());

	/* slave 2 */
	run_query(3, $link, "SELECT 2 AS _one FROM DUAL");
	$threads[$link->thread_id] = array('role' => 'Slave 2', 'stat' => $link->stat());

	$res = run_query(4, $link, "SELECT DATABASE() AS _db FROM DUAL", MYSQLND_MS_LAST_USED_SWITCH);
	$row = $res->fetch_assoc();
	if ($db != $row['_db'])
		printf("[005] Expecting database '%s' got '%s'\n", $ddb, $row['_db']);

	foreach ($threads as $thread_id => $details) {
		printf("%d - %s: '%s'\n", $thread_id, $details['role'], $details['stat']);
		if ('' == $details['stat'])
			printf("Server stat must not be empty!\n");
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_settings_host_credentials_inherit.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_settings_host_credentials_inherit.ini'.\n");
?>
--EXPECTF--
%d - Slave 1: '%s'
%d - Master: '%s'
%d - Slave 2: '%s'
done!