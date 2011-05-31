--TEST--
mysqlnd_ms_set_user_pick_server()
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host),
		'pick' 	=> array('user'),
	),
);
if ($error = create_config("test_mysqlnd_ms_set_user_pick_server.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_set_user_pick_server.ini
--FILE--
<?php
	require_once("connect.inc");

	function grumble_catchable_fatal_grumble($errno, $error, $file, $line) {
		static $errcodes = array();
		if (empty($errcodes)) {
			$constants = get_defined_constants();
			foreach ($constants as $name => $value) {
				if (substr($name, 0, 2) == "E_")
					$errcodes[$value] = $name;
			}
		}
		printf("[%s] %s in %s on line %s\n",
			(isset($errcodes[$errno])) ? $errcodes[$errno] : $errno,
			 $error, $file, $line);

		return true;
	}

	set_error_handler('grumble_catchable_fatal_grumble');

	function pick_server($connected_host, $query, $master, $slaves, $last_used_connection, $in_transaction) {
		printf("pick_server('%s', '%s')\n", $connected_host, $query);
		return ($last_used_connection) ? $last_used_connection : $master;
	}

	class picker {
		public function server($connected_host, $query, $master, $slaves, $last_used_connection, $in_transaction) {
			printf("%spick_server('%s', '%s')\n", isset($this) ? 'picker->' : 'picker::', $connected_host, $query);
			return ($last_used_connection) ? $last_used_connection : $master;
		}
		public static function server_static($connected_host, $query, $master, $slaves, $last_used_connection, $in_transaction) {
			printf("%sserver_static('%s', '%s')\n", isset($this) ? 'picker->' : 'picker::', $connected_host, $query);
			return ($last_used_connection) ? $last_used_connection : $master;
		}
		private static function server_static_private($connected_host, $query, $master, $slaves, $last_used_connection, $in_transaction) {
			printf("%sserver_static_private('%s', '%s')\n", isset($this) ? 'picker->' : 'picker::', $connected_host, $query);
			return ($last_used_connection) ? $last_used_connection : $master;
		}
		private function server_private($connected_host, $query, $master, $slaves, $last_used_connection, $in_transaction) {
			printf("%sserver_private('%s', '%s')\n", isset($this) ? 'picker->' : 'picker::', $connected_host, $query);
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

	if (true !== mysqlnd_ms_set_user_pick_server("pick_server")) {
		printf("[004] Cannot install user callback for picking/selecting servers\n");
	}

	run_query(5, $link, "SELECT 1 FROM DUAL");
	run_query(6, $link, "SET @my_role='master'");

	if (false !== mysqlnd_ms_set_user_pick_server("unknown_function")) {
		printf("[007] Unknown function should be rejected\n");
	}

	run_query(8, $link, "SELECT 1 FROM DUAL");
	run_query(9, $link, "SET @my_role='master'");

	if (false !== mysqlnd_ms_set_user_pick_server(array("picker", "server"))) {
		printf("[010] Non static method should be rejected (Ok, although not nice, if not.)\n");
	}

	run_query(11, $link, "SELECT 1 FROM DUAL");
	run_query(12, $link, "SET @my_role='master'");

	if (true !== mysqlnd_ms_set_user_pick_server(array("picker", "server_static"))) {
		printf("[013] Static public method should be allowed\n");
	}

	run_query(14, $link, "SELECT 1 FROM DUAL");
	run_query(15, $link, "SET @my_role='master'");

	if (false !== mysqlnd_ms_set_user_pick_server(array("picker", "server_static_private"))) {
		printf("[016] Static private method should be rejected\n");
	}

	run_query(17, $link, "SELECT 1 FROM DUAL");
	run_query(18, $link, "SET @my_role='master'");

	$p = new picker();
	if (true !== mysqlnd_ms_set_user_pick_server(array($p, "server"))) {
		printf("[019] Public method should be allowed\n");
	}

	run_query(20, $link, "SELECT 1 FROM DUAL");
	run_query(21, $link, "SET @my_role='master'");

	if (false !== mysqlnd_ms_set_user_pick_server(array($p, "server_private"))) {
		printf("[022] Private method should be rejected\n");
	}

	run_query(23, $link, "SELECT 1 FROM DUAL");
	run_query(24, $link, "SET @my_role='master'");


	if (NULL !== ($ret = @mysqlnd_ms_set_user_pick_server())) {
		printf("[025] Expecting NULL got %s/%s\n", gettype($ret), $ret);
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_set_user_pick_server.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_set_user_pick_server.ini'.\n");
?>
--EXPECTF--
[002 + 01] [0] %s
[003 + 01] [0] %s
pick_server('myapp', 'SELECT 1 FROM DUAL')
[005 + 01] [0] %s
pick_server('myapp', 'SET @my_role='master'')
[006 + 01] [0] %s
[E_RECOVERABLE_ERROR] mysqlnd_ms_set_user_pick_server(): Argument is not a valid callback in %s on line %d
pick_server('myapp', 'SELECT 1 FROM DUAL')
[008 + 01] [0] %s
pick_server('myapp', 'SET @my_role='master'')
[009 + 01] [0] %s
[E_STRICT] Non-static method picker::server() should not be called statically in %s on line %d
[010] Non static method should be rejected (Ok, although not nice, if not.)
[E_STRICT] Non-static method picker::server() should not be called statically in %s on line %d
picker::pick_server('myapp', 'SELECT 1 FROM DUAL')
[011 + 01] [0] %s
[E_STRICT] Non-static method picker::server() should not be called statically in %s on line %d
picker::pick_server('myapp', 'SET @my_role='master'')
[012 + 01] [0] %s
picker::server_static('myapp', 'SELECT 1 FROM DUAL')
[014 + 01] [0] %s
picker::server_static('myapp', 'SET @my_role='master'')
[015 + 01] [0] %s
[E_RECOVERABLE_ERROR] mysqlnd_ms_set_user_pick_server(): Argument is not a valid callback in %s on line %d
picker::server_static('myapp', 'SELECT 1 FROM DUAL')
[017 + 01] [0] %s
picker::server_static('myapp', 'SET @my_role='master'')
[018 + 01] [0] %s
picker->pick_server('myapp', 'SELECT 1 FROM DUAL')
[020 + 01] [0] %s
picker->pick_server('myapp', 'SET @my_role='master'')
[021 + 01] [0] %s
[E_RECOVERABLE_ERROR] mysqlnd_ms_set_user_pick_server(): Argument is not a valid callback in %s on line %d
picker->pick_server('myapp', 'SELECT 1 FROM DUAL')
[023 + 01] [0] %s
picker->pick_server('myapp', 'SET @my_role='master'')
[024 + 01] [0] %s
[E_WARNING] %s
done!