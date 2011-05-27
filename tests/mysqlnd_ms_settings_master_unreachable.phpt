--TEST--
Settings: unreachable master
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array("unknown_i_really_hope"),
		'slave' => array($slave_host, $slave_host),
		'pick' => array("roundrobin"),
		'lazy_connections' => 0,
	),

);
if ($error = create_config("test_mysqlnd_ms_master_unreachable.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
error_reporting=E_ALL
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_master_unreachable.ini
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

	/* error messages (warnings can vary a bit, let's not bother about it */
	ob_start();
	$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket);
	$tmp = ob_get_contents();
	ob_end_clean();
	if ('' == $tmp)
		printf("[001] Expecting some warnings/errors but nothing has been printed, check manually");

	if (!$link) {
		if (0 == mysqli_connect_errno()) {
			printf("[002] Plugin has failed to connect but connect error is not set, [%d] '%s'\n",
				mysqli_connect_errno(), mysqli_connect_error());
		} else {
			die(sprintf("[003] Plugin reports connect error, [%d] '%s'\n",
				mysqli_connect_errno(), mysqli_connect_error()));
		}
	} else {
		printf("[004] Plugin returns valid handle, no API to fetch error codes, connect error: [%d] '%s', error: [%d] '%s'\n",
			mysqli_connect_errno(), mysqli_connect_error(),
			mysqli_errno($link), mysqli_error($link));
	}

	$slaves = array();
	/* slave 1 */
	$res = run_query(5, $link, "SELECT CONNECTION_ID() AS _conn_id");
	if (!$row = $res->fetch_assoc()) {
		printf("[006] [%d] %s\n", $link->errno, $link->error);
	}
	$slaves[$row['_conn_id']] = (isset($slaves[$row['_conn_id']])) ? ++$slaves[$row['_conn_id']] : 1;

	/* slave 2 */
	$res = run_query(7, $link, "SELECT CONNECTION_ID() AS _conn_id");
	if (!$row = $res->fetch_assoc()) {
		printf("[008] [%d] %s\n", $link->errno, $link->error);
	}
	$slaves[$row['_conn_id']] = (isset($slaves[$row['_conn_id']])) ? ++$slaves[$row['_conn_id']] : 1;

	/* slave 1  - there should be no slave 3 */
	$res = run_query(9, $link, "SELECT CONNECTION_ID() AS _conn_id");
	if (!$row = $res->fetch_assoc()) {
		printf("[010] [%d] %s\n", $link->errno, $link->error);
	}
	$slaves[$row['_conn_id']] = (isset($slaves[$row['_conn_id']])) ? ++$slaves[$row['_conn_id']] : 1;

	foreach ($slaves as $thread => $num_queries) {
		printf("Slave %d has run %d queries.\n", $thread, $num_queries);
	}

	/* brute force query running to ensure the plugin does no magic and adds slaves... */
	for ($i = 11; $i < 222; $i += 2) {
		$res = run_query($i, $link, "SELECT CONNECTION_ID() AS _conn_id");
		if (!$row = $res->fetch_assoc()) {
			printf("[%03d] [%d] %s\n", $i + 1, $link->errno, $link->error);
		}
		$slaves[$row['_conn_id']] = (isset($slaves[$row['_conn_id']])) ? ++$slaves[$row['_conn_id']] : 1;
	}
	if (count($slaves) != 2) {
		printf("[300] There are more than two slave connections, there are %d.\n", count($slaves));
	}

	$link->close();

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_master_unreachable.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_master_unreachable.ini'.\n");
?>
--EXPECTF--
[003] Plugin reports connect error, [200%d] '%s'
