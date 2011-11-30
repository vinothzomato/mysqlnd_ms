--TEST--
Filter QOS, eventual consistency
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

if (($emulated_master_host == $emulated_slave_host)) {
	die("SKIP master and slave seem to the the same, see tests/README");
}

_skipif_check_extensions(array("mysqli"));
_skipif_connect($emulated_master_host_only, $user, $passwd, $db, $emulated_master_port, $emulated_master_socket);
_skipif_connect($emulated_slave_host_only, $user, $passwd, $db, $emulated_slave_port, $emulated_slave_socket);

/* Emulated ID does not work with replication */
include_once("util.inc");
$ret = mst_is_slave_of($emulated_slave_host_only, $emulated_slave_port, $emulated_slave_socket, $emulated_master_host_only, $emulated_master_port, $emulated_master_socket, $user, $passwd, $db);
if (is_string($ret))
	die(sprintf("SKIP Failed to check relation of configured master and slave, %s\n", $ret));

if (true == $ret)
	die("SKIP Configured emulated master and emulated slave could be part of a replication cluster\n");


$settings = array(
	"myapp" => array(
		'master' => array(
			"master1" => array(
				'host' 		=> $emulated_master_host_only,
				'port' 		=> (int)$emulated_master_port,
				'socket' 	=> $emulated_master_socket,
			),
		),
		'slave' => array(
			"slave1" => array(
				'host' 	=> $emulated_slave_host_only,
				'port' 	=> (int)$emulated_slave_port,
				'socket' => $emulated_slave_socket,
			),
		 ),

		'lazy_connections' => 0,

		'filters' => array(
			"quality_of_service" => array(
				"eventual_consistency" => 1,
			),
			"random" => array(),
		),
	),

);
if ($error = mst_create_config("test_mysqlnd_ms_filter_qos_eventual_consistency.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

msg_mysqli_init_emulated_id_skip($emulated_slave_host, $user, $passwd, $db, $emulated_slave_port, $emulated_slave_socket, "slave1");
msg_mysqli_init_emulated_id_skip($emulated_master_host, $user, $passwd, $db, $emulated_master_port, $emulated_master_socket, "master1");
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_filter_qos_eventual_consistency.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	/*
	TODO: mysqlnd_ms_section_filters_add_filter() cannot handle situations when
	only one multi filter is set by the user.

	Bug/TODO 1:

	mysqlnd_ms_section_filters_add_filter() inserts default pick filter only if
	no filter is set. However, in this case there is one filter (qos) given.
	Thus, default random once pick filter is not inserted.

	Bug/TODO 2:

	When the script is run, the qos filter picks a list of master and slave connections.
	Then, MS does not know which server to pick. MS does not detect the misconfiguration.
	MS must forbid a multi filter as last filter in the chain.
	*/

	mst_mysqli_query(2, $link, "SET @myrole='master'");
	$master_id = mst_mysqli_get_emulated_id(3, $link);

	mst_mysqli_query(4, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);
	$slave_id = mst_mysqli_get_emulated_id(5, $link);

	/* slave access shall be allowed */
	if ($res = mst_mysqli_query(6, $link, "SELECT @myrole AS _msg")) {
		$row = $res->fetch_assoc();
		printf("Greetings from '%s'\n", $row['_msg']);

		$server_id = mst_mysqli_get_emulated_id(7, $link);
		if ($server_id != $slave_id) {
			printf("[008] Wrong server %s, slave = %s, master = %s\n",
			  $server_id, $slave_id, $master_id);
		}

	} else {
		printf("[%d] %s\n", $link->errno, $link->error);
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_filter_qos_eventual_consistency.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_filter_qos_eventual_consistency.ini'.\n");
?>
--EXPECTF--
Greetings from 'slave'
done!