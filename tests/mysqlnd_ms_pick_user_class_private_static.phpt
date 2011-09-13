--TEST--
pick = user, callback = private static method
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host),
		'pick' 	=> array('user' => array('callback' => array('picker', 'server_static'))),
	),
);
if ($error = mst_create_config("test_mysqlnd_ms_pick_user_class_private_static", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_pick_user_class_private_static
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");
	set_error_handler('mst_error_handler');

	class picker {
		private static function server_static($connected_host, $query, $master, $slaves, $last_used_connection, $in_transaction) {
			printf("%sserver_static('%s', '%s')\n", isset($this) ? 'picker->' : 'picker::', $connected_host, $query);
			return ($last_used_connection) ? $last_used_connection : $master;
		}
	}

	if (!$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket))
		printf("[001] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n",
			$host, $user, $db, $port, $socket);

	mst_mysqli_query(2, $link, "SELECT 1 FROM DUAL");
	mst_mysqli_query(3, $link, "SET @my_role='master'");


	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_pick_user_class_private_static"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_pick_user_class_private_static'.\n");
?>
--XFAIL--
Syntax not supported. Was supported with mysqlnd_ms_set_user_pick_server()
--EXPECTF--
Some relevant error message
done!