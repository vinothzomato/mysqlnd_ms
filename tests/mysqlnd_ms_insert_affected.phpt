--TEST--
insert id, affected rows
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

if ($master_host == $slave_host) {
	die("SKIP master and slave seem to the the same, see tests/README");
}

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
	),
);
if ($error = create_config("test_mysqlnd_insert_affected.ini", $settings))
	die(sprintf("SKIP %d\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_insert_affected.ini
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

	function run_insert($offset, $link, $num_rows, $switch = NULL) {
		if (!$ret = run_query($offset, $link, "DROP TABLE IF EXISTS test", $switch))
			return $ret;

		if (!$ret = run_query($offset + 1, $link, "CREATE TABLE test(id INT AUTO_INCREMENT PRIMARY KEY, label CHAR(1))", MYSQLND_MS_LAST_USED_SWITCH))
			return $ret;

		for ($i = 0; $i < $num_rows; $i++) {
			if (!$ret = run_query($offset + 2, $link, "INSERT INTO test(label) VALUES ('a')", MYSQLND_MS_LAST_USED_SWITCH))
				return $ret;
		}

		return $ret;
	}

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	/* master, automatically */
	run_insert(10, $link, 1);
	if (1 !== $link->insert_id)
		printf("[17] Master insert id should be 1 got %d\n", $link->insert_id);
	run_query(18, $link, "UPDATE test SET label = 'b'", MYSQLND_MS_LAST_USED_SWITCH);
	if (1 !== $link->affected_rows)
		printf("[19] Master affected should be 1 got %d\n", $link->affected_rows);

	/* slave 1 */
	run_insert(20, $link, 5, MYSQLND_MS_SLAVE_SWITCH);
	$connections[$link->thread_id] = $link->insert_id;
	if (5 !== $link->insert_id)
		printf("[27] Slave 1 insert id should be 5 got %d\n", $link->insert_id);
	run_query(28, $link, "UPDATE test SET label = 'b'", MYSQLND_MS_LAST_USED_SWITCH);
	if (5 !== $link->affected_rows)
		printf("[29] Slave 1 affected should be 5 got %d\n", $link->affected_rows);

	/* slave 2 */
	run_insert(30, $link, 10, MYSQLND_MS_SLAVE_SWITCH);
	if (10 !== $link->insert_id)
		printf("[37] Slave 2 insert id should be 10 got %d\n", $link->insert_id);
	run_query(38, $link, "UPDATE test SET label = 'b'", MYSQLND_MS_LAST_USED_SWITCH);
	if (10 !== $link->affected_rows)
		printf("[39] Slave 2 affected should be 10 got %d\n", $link->affected_rows);


	print "done!";

?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_insert_affected.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_ini_force_config.ini'.\n");
?>
--EXPECTF--
done!