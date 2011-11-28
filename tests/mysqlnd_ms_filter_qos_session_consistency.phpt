--TEST--
Filter QOS, session consistency
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

if (($master_host == $slave_host)) {
	die("SKIP master and slave seem to the the same, see tests/README");
}

_skipif_check_extensions(array("mysqli"));
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
				'socket' => $slave_socket,
			),
		 ),

		'lazy_connections' => 0,

		'filters' => array(
			"quality_of_service" => array(
				"session_consistency" => 1,
			),
			"random" => array("sticky" => 1),
		),
	),

);
if ($error = mst_create_config("test_mysqlnd_ms_filter_qos_session_consistency.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

include_once("util.inc");
msg_mysqli_init_emulated_id_skip($slave_host, $user, $passwd, $db, $slave_port, $slave_socket, "slave1");
msg_mysqli_init_emulated_id_skip($master_host, $user, $passwd, $db, $master_port, $master_socket, "master1");

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_filter_qos_session_consistency.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	mst_mysqli_query(2, $link, "SET @myrole='master'");
	$master_id = mst_mysqli_get_emulated_id(3, $link);

	/* session consistency --- master only if no other consistency criteria is set */
	mst_mysqli_query(4, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);
	$server_id = mst_mysqli_get_emulated_id(5, $link);
	if ($server_id != $master_id) {
		printf("[006] Expecting master use, found %s used\n", $server_id);
	}

	/* By default slave shall not be used but last set has overwritten previous one */
	if ($res = mst_mysqli_query(7, $link, "SELECT @myrole AS _msg")) {
		$row = $res->fetch_assoc();
		printf("Greetings from '%s'\n", $row['_msg']);
	} else {
		printf("[%d] %s\n", $link->errno, $link->error);
	}

	$server_id = mst_mysqli_get_emulated_id(8, $link);
	if ($server_id != $master_id) {
		printf("[009] Expecting master use, found %s used\n", $server_id);
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_filter_qos_session_consistency.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_filter_qos_session_consistency.ini'.\n");
?>
--EXPECTF--
Greetings from 'slave'
done!