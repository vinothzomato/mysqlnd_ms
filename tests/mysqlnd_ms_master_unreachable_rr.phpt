--TEST--
Unreachable master, pick = round robin, lazy = 0
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array("unknown_i_really_hope"),
		'slave' => array($slave_host),
		'pick'		=> array('roundrobin'),
		'lazy_connections' => 0,
	),

);
if ($error = create_config("test_mysqlnd_ms_master_unreachable_rr.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
error_reporting=E_ALL
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_master_unreachable_rr.ini
mysqlnd_ms.collect_statistics=1
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	compare_stats();
	/* error messages (warnings can vary a bit, let's not bother about it */
	ob_start();
	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	$tmp = ob_get_contents();
	ob_end_clean();
	if ('' == $tmp)
		printf("[001] Expecting some warnings/errors but nothing has been printed, check manually");

	compare_stats();
	if (!$link) {
		if (0 == mysqli_connect_errno()) {
			printf("[002] Plugin has failed to connect but connect error is not set, [%d] '%s'\n",
				mysqli_connect_errno(), mysqli_connect_error());
		} else {
			die(sprintf("[003] Plugin reports connect error, [%d] '%s'\n",
				mysqli_connect_errno(), mysqli_connect_error()));
		}
	} else {
		printf("[004] Plugin returns valid handle, no API to fetch error codes, connect error: [%d] '%s', error: [%d] '%s'\n",
			mysqli_connect_errno(), mysqli_connect_error(),
			mysqli_errno($link), mysqli_error($link));
	}

	$link->close();

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_master_unreachable_rr.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_master_unreachable_rr.ini'.\n");
?>
--EXPECTF--
Stats non_lazy_connections_master_failure: 1
[003] Plugin reports connect error, [200%d] '%s'
