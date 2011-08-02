--TEST--
lazy connections and stmt init
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
		 ),

		'lazy_connections' => 1,
		'filters' => array(
			"random" => array('sticky' => '1'),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_lazy_stmt_init.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_lazy_stmt_init.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	if (!$link->dump_debug_info())
		printf("[003] [%d] %s\n", $link->errno, $link->error);

	if (!($stmt = $link->stmt_init()))
		printf("[004] [%d] %s\n", $link->errno, $link->error);

	if (!($stmt = $link->stmt_init()))
		printf("[005] [%d] %s\n", $link->errno, $link->error);

	/* line useable ?! */
	if (!$link->dump_debug_info())
		printf("[006] [%d] %s\n", $link->errno, $link->error);


	if (is_object($stmt)) {
		$one = NULL;
		if (!$stmt->prepare("SELECT 1 AS _one FROM DUAL") ||
			!$stmt->execute() ||
			!$stmt->bind_result($one))
			printf("[007] [%d] '%s'\n", $stmt->errno, $stmt->error);
		else
			printf("[008] _one = %s\n", $one);
	}

	if ($res = run_query(6, $link, "SELECT 1 FROM DUAL"))
		var_dump($res->fetch_assoc());

	print "done!";
?>
--CLEAN--
<?php
	require_once("connect.inc");

	if (!unlink("test_mysqlnd_ms_lazy_stmt_init.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_lazy_stmt_init.ini'.\n");
?>
--EXPECTF--
[003] %d %s
[004] %d %s
[005] %d %s
[006] %d %s
[007] %d '%s'
array(1) {
  [1]=>
  string(1) "1"
}
done!