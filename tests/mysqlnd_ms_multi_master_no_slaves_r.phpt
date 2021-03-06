--TEST--
Multi master, no slaves, random
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($emulated_master_host_only, $user, $passwd, $db, $emulated_master_port, $emulated_master_socket);
_skipif_connect($emulated_slave_host_only, $user, $passwd, $db, $emulated_slave_port, $emulated_slave_socket);

$settings = array(
	"myapp" => array(
		 /* NOTE: second master will be ignored! */
		'master' => array($emulated_master_host, $emulated_master_host),
		'slave' => array(),
		'pick' => array('random'),
	),
);
if ($error = mst_create_config("test_mysqlnd_ms_multi_master_no_slaves_r.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

include_once("util.inc");
msg_mysqli_init_emulated_id_skip($emulated_slave_host_only, $user, $passwd, $db, $emulated_slave_port, $emulated_slave_socket, "slave");
msg_mysqli_init_emulated_id_skip($emulated_master_host_only, $user, $passwd, $db, $emulated_master_port, $emulated_master_socket, "master[1,2]");
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.config_file=test_mysqlnd_ms_multi_master_no_slaves_r.ini
mysqlnd_ms.multi_master=1
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	if (!($link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	/* masters - writes */
	$servers = array();
	for ($i = 0; $i <= 100; $i++) {
		mst_mysqli_query(2, $link, "SET @myrole='Master'");
		$server_id = mst_mysqli_get_emulated_id(3, $link);
		if (isset($servers[$server_id]))
			$servers[$server_id] = $servers[$server_id] + 1;
		else
			$servers[$server_id] = 1;

		if (count($servers) > 1)
			break;
	}

	if (100 <= $i) {
		printf("[004] Random has choosen the same master for 100 subsequent writes\n");
	}

	/* slaves - reads */
	$servers = array();
	for ($i = 0; $i <= 100; $i++) {
		/* ignore warning */
		if ((!($res = @$link->query("SELECT 1 FROM DUAL"))) && (2000 != $link->errno)) {
			printf("[005] Wrong connection error. User should always get same error for same issue, [%d] %s\n", $link->errno, $link->error);
			/* breaking to keep trace short */
			break;
		}

		$server_id = mst_mysqli_get_emulated_id(6, $link);
		if (isset($servers[$server_id]))
			$servers[$server_id] = $servers[$server_id] + 1;
		else
			$servers[$server_id] = 1;

		if (count($servers) > 1)
			break;
	}

	if (100 <= $i) {
		printf("[007] Random has choosen the same slave for 100 subsequent reads\n");
	}

	if (!$link->query("SELECT 1 FROM DUAL"))
		printf("[008] [%d] %s\n", $link->errno, $link->error);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_multi_master_no_slaves_r.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_multi_master_no_slaves_r.ini'.\n");
?>
--EXPECTF--
[007] Random has choosen the same slave for 100 subsequent reads

Warning: mysqli::query(): (mysqlnd_ms) Couldn't find the appropriate slave connection. 0 slaves to choose from. Something is wrong in %s on line %d

Warning: mysqli::query(): (mysqlnd_ms) No connection selected by the last filter in %s on line %d
[008] [2000] (mysqlnd_ms) No connection selected by the last filter
done!
