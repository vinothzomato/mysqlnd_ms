--TEST--
pick = user, callback = private method
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
		'pick' 	=> array('user' => array('callback' => array('picker', 'server'))),
	),
);
if ($error = create_config("test_mysqlnd_ms_pick_user_class_private.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_pick_user_class_private.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_pick_user.inc");

	class picker {
		private function server($connected_host, $query, $master, $slaves, $last_used_connection, $in_transaction) {
			printf("%sserver('%s', '%s')\n", isset($this) ? 'picker->' : 'picker::', $connected_host, $query);
			return ($last_used_connection) ? $last_used_connection : $master[0];
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
	if (!unlink("test_mysqlnd_ms_pick_user_class_private.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_pick_user_class_private.ini'.\n");
?>
--XFAIL--
Syntax not supported. Was supported with mysqlnd_ms_set_user_pick_server()
--EXPECTF--
Relevant error message
done!