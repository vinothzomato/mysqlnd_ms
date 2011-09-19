--TEST--
SQL hints to control query redirection
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

if ($master_host == $slave_host) {
	die("SKIP master and slave seem to the the same, see tests/README");
}

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host),
	),
);
if ($error = mst_create_config("test_mysqlnd_ms_sql_hints.ini", $settings))
  die(sprintf("SKIP %s\n", $error));

include_once("util.inc");
msg_mysqli_init_emulated_id_skip($slave_host, $user, $passwd, $db, $slave_port, $slave_socket, "slave");
msg_mysqli_init_emulated_id_skip($master_host, $user, $passwd, $db, $master_port, $master_socket, "master");

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_sql_hints.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	function my_mysqli_query($offset, $link, $query, $expected) {
		if (!$res = $link->query($query)) {
			printf("[%03d + 01] [%d] %s\n", $offset, $link->errno, $link->error);
			return false;
		}
		if ($expected) {
			$row = $res->fetch_assoc();
			$res->close();
			if (empty($row)) {
				printf("[%03d + 02] [%d] %s, empty result\n", $offset, $link->errno, $link->error);
				return false;
			}
			if ($row != $expected) {
				printf("[%03d + 03] Unexpected results, dumping data\n", $offset);
				var_dump($row);
				var_dump($expected);
				return false;
			}
		}
		return true;
	}

	if (!$link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket))
		printf("[001] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n",
			$host, $user, $db, $port, $socket);

	$expected = array();
	my_mysqli_query(10, $link, sprintf("/*%s*/DROP TABLE IF EXISTS test", MYSQLND_MS_MASTER_SWITCH), $expected);
	$master_thread_id = $link->thread_id;
	$master = mst_mysqli_get_emulated_id(11, $link);
	my_mysqli_query(20, $link, sprintf("/*%s*/DROP TABLE IF EXISTS test", MYSQLND_MS_SLAVE_SWITCH), $expected);
	$slave_thread_id = $link->thread_id;
	$slave = mst_mysqli_get_emulated_id(21, $link);
	my_mysqli_query(30, $link, sprintf("/*%s*/CREATE TABLE test(id INT)", MYSQLND_MS_MASTER_SWITCH), $expected);
	my_mysqli_query(40, $link, sprintf("/*%s*/CREATE TABLE test(id INT)", MYSQLND_MS_SLAVE_SWITCH), $expected);
	my_mysqli_query(50, $link, sprintf("/*%s*/INSERT INTO test(id) VALUES (CONNECTION_ID())", MYSQLND_MS_SLAVE_SWITCH), $expected);
	my_mysqli_query(60, $link, sprintf("/*%s*/INSERT INTO test(id) VALUES (CONNECTION_ID())", MYSQLND_MS_MASTER_SWITCH), $expected);
	my_mysqli_query(70, $link, sprintf("/*%s*/SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH), $expected);
	my_mysqli_query(80, $link, sprintf("/*%s*/SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH), $expected);

	/* slave, no hint */
	$expected = array('_role' => 'slave');
	my_mysqli_query(90, $link, "SELECT @myrole AS _role", $expected);
	$server_id = mst_mysqli_get_emulated_id(91, $link);
	if ($server_id != $slave)
		printf("[092] Query should have been run on the slave\n");

	/* master, no hint */
	$expected = array();
	my_mysqli_query(100, $link, "INSERT INTO test(id) VALUES (-2)", $expected);
	$server_id = mst_mysqli_get_emulated_id(101, $link);
	if ($server_id != $master)
		printf("[102] Query should have been run on the master\n");

	/* ... boring: slave */
	$expected = array("id" => $slave_thread_id);
	my_mysqli_query(110, $link, sprintf("/*%s*/SELECT id FROM test", MYSQLND_MS_SLAVE_SWITCH), $expected);
	$server_id = mst_mysqli_get_emulated_id(111, $link);
	if ($server_id != $slave)
		printf("[112] Query should have been run on the slave\n");

	/* master, no hint */
	$expected = array();
	my_mysqli_query(120, $link, "DELETE FROM test WHERE id = -2", $expected);
	$server_id = mst_mysqli_get_emulated_id(121, $link);
	if ($server_id != $master)
		printf("[122] Query should have been run on the master\n");

	/* master, forced */
	$expected = array("id" => $master_thread_id);
	my_mysqli_query(130, $link, sprintf("/*%s*/SELECT id FROM test", MYSQLND_MS_MASTER_SWITCH), $expected);
	$server_id = mst_mysqli_get_emulated_id(131, $link);
	if ($server_id != $master)
		printf("[132] Query should have been run on the master\n");

	/* master, forced */
	$expected = array("_role" => 'master');
	my_mysqli_query(140, $link, sprintf("/*%s*/SELECT @myrole AS _role", MYSQLND_MS_LAST_USED_SWITCH), $expected);
	$server_id = mst_mysqli_get_emulated_id(141, $link);
	if ($server_id != $master)
		printf("[142] Query should have been run on the master\n");

	/* slave, forced */
	$expected = array();
	my_mysqli_query(150, $link, sprintf("/*%s*/INSERT INTO test(id) VALUES (0)", MYSQLND_MS_SLAVE_SWITCH), $expected);
	$server_id = mst_mysqli_get_emulated_id(151, $link);
	if ($server_id != $slave)
		printf("[152] Query should have been run on the slave\n");

	$expected = array('_role' => 'slave');
	my_mysqli_query(160, $link, sprintf("/*%s*/SELECT @myrole AS _role", MYSQLND_MS_LAST_USED_SWITCH), $expected);
	$server_id = mst_mysqli_get_emulated_id(161, $link);
	if ($server_id != $slave)
		printf("[162] Query should have been run on the slave\n");

	$expected = array();
	my_mysqli_query(170, $link, sprintf("/*%s*/DROP TABLE IF EXISTS test", MYSQLND_MS_MASTER_SWITCH), $expected);
	my_mysqli_query(180, $link, sprintf("/*%s*/DROP TABLE IF EXISTS test", MYSQLND_MS_SLAVE_SWITCH), $expected);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_sql_hints.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_sql_hints.ini'.\n");
?>
--EXPECTF--
done!