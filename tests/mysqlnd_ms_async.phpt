--TEST--
Asynchronous queries
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
		'pick' => array("roundrobin"),
	),
);
if ($error = create_config("test_mysqlnd_ms_async.ini", $settings))
  die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_async.ini
--FILE--
<?php
	require_once("connect.inc");
	$threads = array();

	$link1 = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (0 !== mysqli_connect_errno())
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$link1->query("SELECT 'test'", MYSQLI_ASYNC);
	$all_links = array($link1);
	$processed = 0;
	do {
		$links = $errors = $reject = array();
		foreach ($all_links as $link) {
			$links[] = $errors[] = $reject[] = $link;
		}
		if (!mysqli_poll($links, $errors, $reject, 1)) {
			usleep(100);
			continue;
		}
		foreach ($links as $link) {
			if ($result = $link->reap_async_query()) {
				print_r($result->fetch_row());
				mysqli_free_result($result);
				$processed++;
			}
		}
	} while ($processed < count($all_links));

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_async.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_async.ini'.\n");
?>
--EXPECTF--
Something meaningful
done!
