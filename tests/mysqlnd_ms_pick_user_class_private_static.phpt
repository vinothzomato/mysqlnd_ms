--TEST--
pick = user, callback = private static method
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host),
		'pick' 	=> array('user' => array('callback' => array('picker', 'server_static'))),
	),
);
if ($error = create_config("test_mysqlnd_ms_pick_user_class_private_static", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_pick_user_class_private_static
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_pick_user.inc");

	class picker {
		private static function server_static($connected_host, $query, $master, $slaves, $last_used_connection, $in_transaction) {
			printf("%sserver_static('%s', '%s')\n", isset($this) ? 'picker->' : 'picker::', $connected_host, $query);
			return ($last_used_connection) ? $last_used_connection : $master;
		}
	}

	function run_query($offset, $link, $query) {
		$ret = $link->query($query);
		printf("[%03d + 01] [%d] '%s'\n", $offset, $link->errno, $link->error);
		return $ret;
	}

	if (!$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket))
		printf("[001] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n",
			$host, $user, $db, $port, $socket);

	run_query(2, $link, "SELECT 1 FROM DUAL");
	run_query(3, $link, "SET @my_role='master'");


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