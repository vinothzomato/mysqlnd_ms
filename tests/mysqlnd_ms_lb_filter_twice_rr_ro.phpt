--TEST--
Stacking LB filter: rr | random_once - second ignored
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

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
			"slave2" => array(
				'host' 	=> $slave_host_only,
				'port' 	=> (int)$slave_port,
				'socket' => $slave_socket,
			),
		 ),
		'lazy_connections' => 0,
		'filters' => array(
			"roundrobin" => array(),
			"random" => array("sticky" => 1),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_lb_filter_twice_rr_ro.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_lb_filter_twice_rr_ro.ini
mysqlnd.debug=d:t:O,/tmp/mysqlnd.trace
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	$threads = array();
	run_query(2, $link, "SET @myrole='master'");
	$threads[$link->thread_id] = 'master';
	run_query(3, $link, "SET @myrole='slave1'", MYSQLND_MS_SLAVE_SWITCH);
	$threads[$link->thread_id] = 'slave1';
	run_query(4, $link, "SET @myrole='slave2'", MYSQLND_MS_SLAVE_SWITCH);
	$threads[$link->thread_id] = 'slave2';

	$res = run_query(5, $link, "SELECT @myrole AS _role");
	$row = $res->fetch_assoc();
	printf("[006] Hi folks, %s speaking.\n", $row['_role']);


	foreach ($threads as $id => $role)
		printf("%d => %s\n", $id, $role);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_lb_filter_twice_rr_ro.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_lb_filter_twice_rr_ro.ini'.\n");
?>
--EXPECTF--
[006] Hi folks, slave1 speaking.
%d => master
%d => slave1
%d => slave2
done!