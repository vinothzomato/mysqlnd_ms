--TEST--
mysqli_set_charset() - commands out of sync
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");


$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
		'pick' => 'roundrobin',
		'lazy_connections' => 1,
	),
);
if ($error = create_config("test_mysqlnd_ms_set_charset_out_of_sync.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_set_charset_out_of_sync.ini
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

	run_query(2, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH);
	run_query(3, $link, "SET @myrole='slave 1'", MYSQLND_MS_SLAVE_SWITCH);
	run_query(4, $link, "SET @myrole='slave 2'", MYSQLND_MS_SLAVE_SWITCH);

	/* slave 1 */
	if (!$res = run_query(4, $link, "SELECT @@character_set_connection AS charset", MYSQLND_MS_SLAVE_SWITCH))
		printf("[010] [%d] %s\n", $link->errno, $link->error);

	if (!$row = $res->fetch_assoc())
		printf("[011] [%d] %s\n", $link->errno, $link->error);

	$current_charset = $row['charset'];
	if ('latin1' == $current_charset) {
		$new_charset = 'latin2';
	} else {
		$new_charset = 'latin1';
	}

	/* shall be run on *all* configured machines - all masters, all slaves */
	$link->set_charset($new_charset);

	print "done!";

?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_set_charset_out_of_sync.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_set_charset_out_of_sync.ini'.\n");
?>
--EXPECTF--
done!