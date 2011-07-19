--TEST--
Many masters; syntax exists but unsupported!
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		 /* NOTE: second master will be ignored! */
		'master' => array($master_host, "unreachable"),
		'slave' => array($slave_host),
	),
);
if ($error = create_config("test_mysqlnd_ms_multi_master.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_multi_master.ini
--FILE--
<?php
	require_once("connect.inc");

	function run_query($offset, $link, $query, $switch = NULL) {
		if ($switch)
			$query = sprintf("/*%s*/%s", $switch, $query);

		if (!($ret = $link->query($query)))
			printf("[%03d] [%d] %s\n", $offset, $link->errno, $link->error);

		return $ret;
	}

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	run_query(3, $link, "SET @myrole='master1'", MYSQLND_MS_MASTER_SWITCH);
	run_query(4, $link, "SET @myrole='master2'", MYSQLND_MS_MASTER_SWITCH);

	$master = array();

	$res = run_query(5, $link, "SELECT CONNECTION_ID() AS _conn_id", MYSQLND_MS_MASTER_SWITCH);
	if (!$res)
		printf("[006] [%d] %s\n", $link->errno, $link->error);

	$row = $res->fetch_assoc();
	if ($link->thread_id != $row['_conn_id'])
		printf("[007] Expecting thread_id = %d got %d\n", $link->thread_id, $row['_conn_id']);

	$master[$row['_conn_id']] = (isset($master[$row['_conn_id']])) ? ++$master[$row['_conn_id']] : 1;

	run_query(8, $link, "DROP TABLE IF EXISTS test");
	run_query(9, $link, "CREATE TABLE test(id INT)", MYSQLND_MS_LAST_USED_SWITCH);
	run_query(10, $link, "INSERT INTO test(id) VALUES(1)", MYSQLND_MS_LAST_USED_SWITCH);
	$res = run_query(11, $link, "SELECT CONNECTION_ID() AS _conn_id", MYSQLND_MS_LAST_USED_SWITCH);
	if (!$res)
		printf("[012] [%d] %s\n", $link->errno, $link->error);

	$row = $res->fetch_assoc();
	if ($link->thread_id != $row['_conn_id'])
		printf("[013] Expecting thread_id = %d got %d\n", $link->thread_id, $row['_conn_id']);

	$master[$row['_conn_id']] = (isset($master[$row['_conn_id']])) ? ++$master[$row['_conn_id']] : 1;

	$res = run_query(14, $link, "SELECT CONNECTION_ID() AS _conn_id", MYSQLND_MS_MASTER_SWITCH);
	if (!$res)
		printf("[015] [%d] %s\n", $link->errno, $link->error);

	$row = $res->fetch_assoc();
	if ($link->thread_id != $row['_conn_id'])
		printf("[016] Expecting thread_id = %d got %d\n", $link->thread_id, $row['_conn_id']);

	$master[$row['_conn_id']] = (isset($master[$row['_conn_id']])) ? ++$master[$row['_conn_id']] : 1;

	foreach ($master as $id => $num_queries) {
		printf("Master %d has run %d queries\n", $id, $num_queries);
	}

	print "done!";

?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_multi_master.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_multi_master.ini'.\n");
?>
--EXPECTF--
Master %d has run %d queries
done!