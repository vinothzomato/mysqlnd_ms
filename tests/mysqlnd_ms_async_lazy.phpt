--TEST--
Asynchronous queries and lazy connections
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
		'pick' => array("roundrobin"),
		'lazy_connections' => 1,
	),
);
if ($error = create_config("test_mysqlnd_ms_async_lazy.ini", $settings))
  die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_async_lazy.ini
--FILE--
<?php
	require_once("connect.inc");
	$threads = array();

	$link1 = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (0 !== mysqli_connect_errno())
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	if (!$link1->query("SELECT 'hello' AS _msg", MYSQLI_ASYNC))
		printf("[002] [%d] %s\n", $link1->errno, $link1->error);

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
				$processed++;

				$row = $result->fetch_assoc();
				printf("fetch %d - thread %d - '%s'\n", $processed, $link->thread_id, $row['_msg']);
				mysqli_free_result($result);
			}
		}
	} while ($processed < count($all_links));

	if (!($res = $link1->query("SELECT 'hello' AS _msg")))
		printf("[003] [%d] %s\n", $link1->errno, $link1->error);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_async_lazy.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_async_lazy.ini'.\n");
?>
--EXPECTF--
fetch 1 - thread %s - %s
done!