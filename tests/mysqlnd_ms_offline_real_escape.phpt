--TEST--
lazy + real escape
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

if (!function_exists("iconv"))
	die("SKIP needs iconv extension\n");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
		'pick' => array("roundrobin"),
		'server_charset' => 'utf8',
		'lazy_connections' => 1,
	),
);
if ($error = mst_create_config("test_mysqlnd_ms_offline_real_escape.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_offline_real_escape.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	if (!($link = mst_mysqli_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$link->set_charset("utf8");

	/* From a user perspective MS and non MS-Connection are now in the same state: connected */
	if (!($link_ms = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$string = "андрей\0'\"улф\"\'\13\10\7йоханес\0майескюел мастер слейв";
	$no_ms = $link->real_escape_string($string);
	$ms = $link_ms->real_escape_string($string);

	if ($no_ms !== $ms) {
		printf("[007] Encoded strings differ for charset 'utf8', MS = '%s', no MS = '%s'\n", $ms, $no_ms);
		printf("[008] [%d/%s] '%s'\n", $link->errno, $link->sqlstate, $link->error);
	}

	print "done!";

?>
--CLEAN--
<?php
if (!unlink("test_mysqlnd_ms_lazy_offline_escape.ini"))
	printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_lazy_offline_escape.ini'.\n");
?>
--EXPECTF--
done!