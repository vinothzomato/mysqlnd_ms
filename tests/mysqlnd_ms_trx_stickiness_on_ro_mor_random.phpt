--TEST--
trx_stickiness=on, RO trx, master on write, pick = random
--SKIPIF--
<?php
if (version_compare(PHP_VERSION, '5.3.99-dev', '<'))
	die(sprintf("SKIP Requires PHP 5.3.99 or newer, using " . PHP_VERSION));

require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host, $slave_host),
		'trx_stickiness' => 'on',
		'master_on_write' => 1,
		'pick' => array("random"),
	),
);
if ($error = mst_create_config("test_mysqlnd_ms_trx_stickiness_on_ro_mor_random.ini", $settings))
	die(sprintf("SKIP %s\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.config_file=test_mysqlnd_ms_trx_stickiness_on_ro_mor_random.ini
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

	if (!($link = mst_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	mst_mysqli_query(2, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH);

	$slaves = array();
	$i = 0;
	do {
		mst_mysqli_query(3, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);
		$slaves[$link->thread_id] = true;
		$i++;
	} while (($i <= 10) && (count($slaves) < 2));
	if (10 == $i) {
		die("[004] Two connections happen to have the same thread id, ignore and run again!");
	}

	$link->begin_transaction(MYSQLI_TRANS_START_READ_ONLY);
	/* ro trx but master on write set, must use masterr! */
	$last = NULL;
	for ($i = 0; $i < 10; $i++) {
		if (!($res = mst_mysqli_query(5, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role"))) {
			printf("[006] [%d] %s\n", $link->errno, $link->error);
		}
		$row = $res->fetch_assoc();
		if (is_null($last)) {
			$last = $link->thread_id;
		} else {
		    if ($last != $link->thread_id) {
				printf("[007] Connection switch during transaction must not happen!\n");
				break;
		    }
		}
		/* must always use master! */
		if (!($res = mst_mysqli_query(8, $link, sprintf("SET @msg='_%d'", $i)))) {
			printf("[009] [%d] %s\n", $link->errno, $link->error);
		}
		if ($last != $link->thread_id) {
			printf("[010] Connection switch during transaction must not happen!\n");
			break;
		}
	}
	$link->commit();

	/* must use master */
	if (!($res = mst_mysqli_query(11, $link, "SELECT CONCAT(@myrole, ' ', CONNECTION_ID()) AS _role"))) {
		printf("[012] [%d] %s\n", $link->errno, $link->error);
	} else {
		$row = $res->fetch_assoc();
		printf("%s\n", $row['_role']);
	}
	$last =  $link->thread_id;

	/* no select... master, please! */
	if (!($res = mst_mysqli_query(13, $link, sprintf("SET @msg='_%d'", ++$i)))) {
			printf("[014] [%d] %s\n", $link->errno, $link->error);
	}

	if ($last != $link->thread_id) {
		printf("[015] Switching must not happen!\n");
	}
	if (!($res = mst_mysqli_query(16, $link, "SELECT @msg AS _msg"))) {
		printf("[017] [%d] %s\n", $link->errno, $link->error);
	} else {
		$row = $res->fetch_assoc();
		printf("%s\n", $row['_msg']);
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_trx_stickiness_on_ro_mor_random.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_trx_stickiness_on_ro_mor_random.ini'.\n");
?>
--EXPECTF--
master %d
_11
done!