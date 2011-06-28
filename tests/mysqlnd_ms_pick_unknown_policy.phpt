--TEST--
Load Balancing: wrong configuration, unknown policy
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'pick'		=> array('unknown'),
		'master' 	=> array($master_host),
		'slave' 	=> array($slave_host, $slave_host, $slave_host),
	),
);
if ($error = create_config("test_mysqlnd_pick_unknown_policy.ini", $settings))
	die(sprintf("SKIP %d\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_pick_unknown_policy.ini
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

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_pick_unknown_policy.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_pick_unknown_policy.ini'.\n");
?>
--XFAIL--
pick is about to be deprected. The test needs to be modified.
The basic idea remains: if config makes no sense, throw connect error,
warn about misconfiguration as early as possible
--EXPECTF--
Some error message about an invalid pick method and a connect error
[001] [2000] Error message about wrong pick method
done!