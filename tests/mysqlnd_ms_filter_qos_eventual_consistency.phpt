--TEST--
Filter QOS, eventual consistency
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

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
				"eventual_consistency" => 1,
			),
		),
	),

);
if ($error = mst_create_config("test_mysqlnd_ms_filter_qos_eventual_consistency.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

include_once("util.inc");
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_filter_qos_eventual_consistency.ini
mysqlnd.debug=d:t:O,/tmp/mysqlnd.trace
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
	mst_mysqli_query(3, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);
	printf("[%d] %s\n", $link->errno, $link->error);

	/* slave access shall be allowed */
	if ($res = mst_mysqli_query(4, $link, "SELECT @myrole AS _msg")) {
		$row = $res->fetch_assoc();
		printf("Greetings from '%s'\n", $row['_msg']);
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
See test
done!