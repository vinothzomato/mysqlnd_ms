--TEST--
Unknown filter continue (round robin)
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
			"ulf" => array( "is" => "amazed"),
			"roundrobin" => array(),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_unknown_filter_cont_r.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_unknown_filter_cont_r.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	set_error_handler('my_error_handler');

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	/* valid config or not? */
	run_query(2, $link, "DROP TABLE IF EXISTS test");
	if (0 == $link->thread_id)
		printf("[003] Not connected to any server.\n");

	if ($res = run_query(4, $link, "SELECT 1")) {
		printf("[005] Who has run this?\n");
		var_dump($res->fetch_assoc());
	}

	if (0 == $link->thread_id)
		printf("[006] Not connected to any server.\n");

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_unknown_filter_cont_r.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_unknown_filter_cont_r.ini'.\n");
?>
--EXPECTF--
[E_RECOVERABLE_ERROR] mysqli_real_connect(): (mysqlnd_ms) Unknown filter 'ulf' . Stopping in %s on line %d
[001] [2000] (mysqlnd_ms) Unknown filter 'ulf' . Stopping
[002] [2000] (mysqlnd_ms) Unknown filter 'ulf' . Stopping
[004] [2000] (mysqlnd_ms) Unknown filter 'ulf' . Stopping
done!