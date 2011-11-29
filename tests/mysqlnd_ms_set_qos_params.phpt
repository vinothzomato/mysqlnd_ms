--TEST--
mysqlnd_ms_set_qos(), params
--SKIPIF--
<?php
if (version_compare(PHP_VERSION, '5.3.99-dev', '<'))
	die(sprintf("SKIP Requires PHP >= 5.3.99, using " . PHP_VERSION));

require_once('skipif.inc');
require_once("connect.inc");

if (($master_host == $slave_host)) {
	die("SKIP master and slave seem to the the same, see tests/README");
}

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

include_once("util.inc");
$sql = mst_get_gtid_sql($db);
if ($error = mst_mysqli_setup_gtid_table($master_host_only, $user, $passwd, $db, $master_port, $master_socket))
  die(sprintf("SKIP Failed to setup GTID on master, %s\n", $error));


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

		'global_transaction_id_injection' => array(
			'on_commit'	 				=> $sql['update'],
			'fetch_last_gtid'			=> $sql['fetch_last_gtid'],
			'check_for_gtid'			=> $sql['check_for_gtid'],
			'report_error'				=> true,
		),

	),

);
if ($error = mst_create_config("test_mysqlnd_ms_set_qos_params.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

msg_mysqli_init_emulated_id_skip($slave_host, $user, $passwd, $db, $slave_port, $slave_socket, "slave1");
msg_mysqli_init_emulated_id_skip($master_host, $user, $passwd, $db, $master_port, $master_socket, "master1");
?>
--INI--
mysqlnd_ms.enable=1
  mysqlnd_ms.ini_file=test_mysqlnd_ms_set_qos_params.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	$link = null;
	if (NULL !== ($ret = @mysqlnd_ms_set_qos()))
		printf("[001] Expecting NULL got %s\n", var_export($ret, true));

	if (NULL !== ($ret = @mysqlnd_ms_set_qos($link)))
		printf("[002] Expecting NULL got %s\n", var_export($ret, true));

	if (NULL !== ($ret = @mysqlnd_ms_set_qos($link, $link, $link, $link, $link)))
		printf("[003] Expecting NULL got %s\n", var_export($ret, true));

	$link = mst_mysqli_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
	if (mysqli_connect_errno()) {
		printf("[004] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	if (false !== ($ret = mysqlnd_ms_set_qos($link, MYSQLND_MS_QOS_CONSISTENCY_STRONG)))
		printf("[005] Expecting false got %s\n", var_export($ret, true));

	$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[006] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	$valid_service_levels = array(
		MYSQLND_MS_QOS_CONSISTENCY_STRONG => true,
		MYSQLND_MS_QOS_CONSISTENCY_SESSION => true,
		MYSQLND_MS_QOS_CONSISTENCY_EVENTUAL => true,
	);
	do {
		$invalid_service_level = mt_rand(10, 100);
	} while (isset($valid_service_levels[$invalid_service_level]));

	if (false !== ($ret = mysqlnd_ms_set_qos($link, $invalid_service_level)))
		printf("[007] Expecting false got %s\n", var_export($ret, true));


	$valid_options = array(
		MYSQLND_MS_QOS_OPTION_GTID => MYSQLND_MS_QOS_CONSISTENCY_SESSION,
		MYSQLND_MS_QOS_OPTION_AGE => NULL,
	);
	do {
		$invalid_option = mt_rand(10, 100);
	} while (isset($valid_options[$invalid_option]));

	foreach ($valid_options as $option => $service_level) {
		if (is_null($service_level))
			continue;

		ob_start();
		$ret = mysqlnd_ms_set_qos($link, $service_level, $invalid_option);
		$tmp = ob_get_contents();
		ob_end_clean();

		if (!stristr($tmp, "Warning")) {
			printf("[008] Can't find warning about invalid option %d for service level %d\n",
				$invalid_option, $service_level);
		}

		if (false !== $ret)
			printf("[009] Expecting false got %s with invalid option %d for service level %d\n",
				var_export($ret, true), $invalid_option, $service_level);
	}

	/* GTID */
	if (false !== ($ret = mysqlnd_ms_set_qos($link, MYSQLND_MS_QOS_CONSISTENCY_SESSION, MYSQLND_MS_QOS_OPTION_GTID))) {
		printf("[010] Expecting false got %s\n", var_export($ret, true));
	}

	/* casted to 0 */
	if (true !== ($ret = mysqlnd_ms_set_qos($link, MYSQLND_MS_QOS_CONSISTENCY_SESSION, MYSQLND_MS_QOS_OPTION_GTID, array()))) {
		printf("[011] Expecting true got %s\n", var_export($ret, true));
	}

	if (false !== ($ret = mysqlnd_ms_set_qos($link, MYSQLND_MS_QOS_CONSISTENCY_SESSION, MYSQLND_MS_QOS_OPTION_GTID, (-1 * PHP_INT_MAX) + 1))) {
		printf("[012] Expecting false got %s\n", var_export($ret, true));
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_set_qos_params.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_set_qos_params.ini'.\n");

	require_once("connect.inc");
	require_once("util.inc");
	if ($error = mst_mysqli_drop_test_table($master_host_only, $user, $passwd, $db, $master_port, $master_socket))
		printf("[clean] %s\n");

	if ($error = mst_mysqli_drop_gtid_table($master_host_only, $user, $passwd, $db, $master_port, $master_socket))
		printf("[clean] %s\n", $error));
?>
--EXPECTF--
Warning: mysqlnd_ms_set_qos(): (mysqlnd_ms) No mysqlnd_ms connection in %s on line %d

Warning: mysqlnd_ms_set_qos(): Invalid service level in %s on line %d

Warning: mysqlnd_ms_set_qos(): Option value required in %s on line %d

Warning: mysqlnd_ms_set_qos(): GTID must have a positive value in %s on line %d
done!