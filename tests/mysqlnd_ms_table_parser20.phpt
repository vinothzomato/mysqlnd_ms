--TEST--
parser: SELECT 'h\'ello'
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_check_feature(array("parser"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

$settings = array(
	"myapp" => array(
		'master' => array(
			 "master1" => array(
				'host' 		=> $master_host_only,
				'port' 		=> (int)$master_port,
				'socket' 	=> $master_socket,
			),
		),
		'slave' => array(
			 "slave1" => array(
				'host' 	=> $slave_host_only,
				'port' 	=> (int)$slave_port,
				'socket' 	=> $slave_socket,
			),
		),
		'lazy_connections' => 0,
		'filters' => array(
		),
	),
);

if (_skipif_have_feature("table_filter")) {
	$settings['myapp']['filters']['table'] = array(
		"rules" => array(
			 "%" => array(
				  "master" => array("master1"),
				  "slave" => array("slave1"),
			),
		),
	);
}

if ($error = mst_create_config("test_mysqlnd_ms_table_parser20.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

include_once("util.inc");
msg_mysqli_init_emulated_id_skip($slave_host, $user, $passwd, $db, $slave_port, $slave_socket, "slave");
msg_mysqli_init_emulated_id_skip($master_host, $user, $passwd, $db, $master_port, $master_socket, "master");
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_table_parser20.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");	

	mst_mysqli_create_test_table($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);
	$sql = "SELECT 'h\'ello' AS _id FROM DUAL";
	if (mst_mysqli_server_supports_query(1, $sql, $slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket)) {

		$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
		if (mysqli_connect_errno())
			printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

		mst_mysqli_query(3, $link, "SELECT 1 FROM test", MYSQLND_MS_SLAVE_SWITCH);
		$slave_id = mst_mysqli_get_emulated_id(4, $link);

		mst_mysqli_fetch_id(6, mst_mysqli_query(5, $link, $sql));
		$server_id = mst_mysqli_get_emulated_id(7, $link);
		if ($slave_id != $server_id)
			printf("[008] Statement has not been executed on the slave\n");

	} else {
		/* fake result */
		printf("[006] _id = 'h'ello'\n");
	}

	$sql = "SELECT '''hello''' AS _id FROM DUAL";
	if (mst_mysqli_server_supports_query(9, $sql, $slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket)) {

		$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
		if (mysqli_connect_errno())
			printf("[010] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

		mst_mysqli_fetch_id(12, mst_mysqli_query(11, $link, $sql));
	} else {
		/* fake result */
		printf("[012] _id = ''hello''\n");
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_table_parser20.ini"))
		printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_table_parser20.ini'.\n");
?>
--EXPECTF--
[001] Testing server support of 'SELECT 'h\'ello' AS _id FROM DUAL'
[006] _id = 'h'ello'
[009] Testing server support of 'SELECT '''hello''' AS _id FROM DUAL'
[012] _id = ''hello''
done!