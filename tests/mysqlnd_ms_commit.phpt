--TEST--
commit()
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

if (version_compare(PHP_VERSION, '5.3.99-dev', '<'))
	die(sprintf("SKIP Requires PHP >= 5.3.99, using " . PHP_VERSION));

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
			"random" => array('sticky' => '1'),
		),
	),

);
if ($error = create_config("test_mysqlnd_ms_commit.ini", $settings))
	die(sprintf("SKIP %s\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_commit.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("mysqlnd_ms_lazy.inc");

	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[002] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	/* commit prior to any command... */
	if (!($wo_ms = $link->commit()))
		printf("[003] [%d] %s\n", $link->errno, $link->error);

	/* master */
	if (!run_query(4, $link, "DROP TABLE IF EXISTS test") ||
		!run_query(5, $link, "CREATE TABLE test(id INT) ENGINE=InnoDB") ||
		!run_query(6, $link, "INSERT INTO test(id) VALUES (1), (2), (3)"))
		printf("[007] [%d] %s\n", $link->errno, $link->error);

	if (!$link->autocommit(FALSE))
		printf("[008] [%d] %s\n", $link->errno, $link->error);

	if (!run_query(9, $link, "INSERT INTO test(id) VALUES (4)"))
		printf("[010] [%d] %s\n", $link->errno, $link->error);

	if (!$link->commit())
		printf("[011] [%d] %s\n", $link->errno, $link->error);

	$res = run_query(12, $link, "SELECT id FROM test ORDER BY id ASC", MYSQLND_MS_MASTER_SWITCH);
	while ($row = $res->fetch_assoc())
		printf("[013] %d\n", $row['id']);

	if (!run_query(14, $link, "DROP TABLE IF EXISTS test"))
		printf("[015] [%d] %s\n", $link->errno, $link->error);

	/* slave */
	if (!run_query(16, $link, "DROP TABLE IF EXISTS test", MYSQLND_MS_SLAVE_SWITCH) ||
		!run_query(17, $link, "CREATE TABLE test(id INT) ENGINE=InnoDB", MYSQLND_MS_SLAVE_SWITCH) ||
		!run_query(18, $link, "INSERT INTO test(id) VALUES (1), (2), (3)", MYSQLND_MS_SLAVE_SWITCH))
		printf("[019] [%d] %s\n", $link->errno, $link->error);

	if (!run_query(20, $link, "INSERT INTO test(id) VALUES (4)", MYSQLND_MS_SLAVE_SWITCH))
		printf("[021] [%d] %s\n", $link->errno, $link->error);

	if (!$link->commit())
		printf("[022] [%d] %s\n", $link->errno, $link->error);

	$res = run_query(23, $link, "SELECT id FROM test ORDER BY id ASC");
	while ($row = $res->fetch_assoc())
		printf("[024] %d\n", $row['id']);

	if (!run_query(25, $link, "DROP TABLE IF EXISTS test", MYSQLND_MS_SLAVE_SWITCH))
		printf("[026] [%d] %s\n", $link->errno, $link->error);

	print "done!";
?>
--CLEAN--
<?php
	require_once("connect.inc");

	if (!unlink("test_mysqlnd_ms_commit.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_commit.ini'.\n");
?>
--EXPECTF--
[013] 1
[013] 2
[013] 3
[013] 4
[024] 1
[024] 2
[024] 3
[024] 4
done!