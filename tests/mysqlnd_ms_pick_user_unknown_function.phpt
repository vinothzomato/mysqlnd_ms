--TEST--
pick = user, callback = non existant
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host),
		'pick' 	=> array('user' => array('callback' => 'unknown function')),
	),
);
if ($error = create_config("test_mysqlnd_ms_pick_user_unknown_function.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_pick_user_unknown_function.ini
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

	function run_query($offset, $link, $query) {
		$ret = $link->query($query);
		printf("[%03d + 01] [%d] '%s'\n", $offset, $link->errno, $link->error);
		return $ret;
	}

	if (!$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket))
		printf("[001] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n",
			$host, $user, $db, $port, $socket);

	run_query(2, $link, "SELECT 1 FROM DUAL");

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_pick_user_unknown_function.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_pick_user_unknown_function.ini'.\n");
?>
--EXPECTF--
[E_WARNING] mysqli::query(): (mysqlnd_ms) Specified callback (unknown function) is not a valid callback in %s on line %d
[002 + 01] [2014] %s
done!
