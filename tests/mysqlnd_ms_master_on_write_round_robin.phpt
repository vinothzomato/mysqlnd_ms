--TEST--
Use master on write (pick = random)
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	$host => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
		'master_on_write' => 1,
		'pick' => array('roundrobin'),
		'lazy_connection' => 0
	),
);
if ($error = create_config("test_mysqlnd_ms_master_on_write_random.ini", $settings))
	die(sprintf("SKIP %d\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_master_on_write_random.ini
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

	if (!($link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	$slaves = array();
	$idx = 0;
	do {
		run_query(2, $link, sprintf("SET @myrole='Slave %d'", ++$idx), MYSQLND_MS_SLAVE_SWITCH);
		$slaves[$link->thread_id] = true;
	} while (count($slaves) < 2);

	$res = run_query(3, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role");

	$row = $res->fetch_assoc();
	$res->close();
	printf("This is '%s' speaking\n", $row['_role']);

	/* not a select -> master query */
	run_query(4, $link, "SET @myrole='Master'");


	/* master on write is active, master should reply */
	$res = run_query(5, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role");
	$row = $res->fetch_assoc();
	$res->close();
	printf("This is '%s' speaking\n", $row['_role']);

	/* SQL hint wins */
	$res = run_query(6, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role",  MYSQLND_MS_SLAVE_SWITCH);
	$row = $res->fetch_assoc();
	$res->close();
	printf("This is '%s' speaking\n", $row['_role']);

	/* master on write is active, master should reply */
	$res = run_query(7, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role");
	$row = $res->fetch_assoc();
	$res->close();
	printf("This is '%s' speaking\n", $row['_role']);

	/* SQL hint wins */
	$res = run_query(8, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role",  MYSQLND_MS_SLAVE_SWITCH);
	$row = $res->fetch_assoc();
	$res->close();
	printf("This is '%s' speaking\n", $row['_role']);

	/* SQL hint wins */
	$res = run_query(9, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role",  MYSQLND_MS_LAST_USED_SWITCH);
	$row = $res->fetch_assoc();
	$res->close();
	printf("This is '%s' speaking\n", $row['_role']);

	/* master on write is active, master should reply */
	$res = run_query(10, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role");
	$row = $res->fetch_assoc();
	$res->close();
	printf("This is '%s' speaking\n", $row['_role']);

	/* SQL hint wins */
	$res = run_query(11, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role",  MYSQLND_MS_SLAVE_SWITCH);
	$row = $res->fetch_assoc();
	$res->close();
	printf("This is '%s' speaking\n", $row['_role']);


	print "done!";

?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_master_on_write_random.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_master_on_write_random.ini'.\n");
?>
--EXPECTF--
This is 'Slave 1 %d' speaking
This is 'Master %d' speaking
This is 'Slave 2 %d' speaking
This is 'Master %d' speaking
This is 'Slave 1 %d' speaking
This is 'Slave 1 %d' speaking
This is 'Master %d' speaking
This is 'Slave 2 %d' speaking
done!