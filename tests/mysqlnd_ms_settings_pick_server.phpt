--TEST--
Config settings: pick server = user
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

if (version_compare(PHP_VERSION, '5.3.99-dev', '>='))
	die(sprintf("SKIP Requires PHP >= 5.3.99, using " . PHP_VERSION));

if (($master_host == $slave_host)) {
	die("SKIP master and slave seem to the the same, see tests/README");
}

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host),
		'pick' 	=> array('user'),
	),
);
if ($error = create_config("test_mysqlnd_ms_pick_server.ini", $settings))
  die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_pick_server.ini
--FILE--
<?php
	require_once("connect.inc");

	/*
	* Select/pick a server for running the query on.
	*
	*/
	function pick_server($connected_host, $query, $master, $slaves, $last_used_connection, $in_transaction) {
		global $queries, $host, $autocommit;
		static $pick_server_last_used = "";
		flush();

		$args = func_get_args();
		$num = func_num_args();
		if ($num != 6) {
			printf("[003] Number of arguments should be 6 got %d\n", $num);
			var_dump($args);
		}

		if ($in_transaction == $autocommit) {
			printf("[004] in_transaction should be %d\n", !$autocommit);
		}

		if ("" == $connected_host) {
			printf("[005] Currently connected host is empty\n");
		}
		if (!isset($queries[$query])) {
			printf("[006] We are asked to handle the query '%s' which has not been issued by us.\n", $query);
		} else {
			unset($queries[$query]);
		}

		if (!is_array($master)) {
			printf("[007] No list of master servers given.");
		} else {
			/* we can't do much better because we get string representations of the master/slave connections */
			if (1 != ($tmp = count($master))) {
				printf("[008] Expecting one entry in the list of masters, found %d. Dumping list.\n", $tmp);
				var_dump($master);
			}
		}

		if (!is_array($slaves)) {
			printf("[009] No list of slave servers given.");
		} else {
			if (1 != ($tmp = count($slaves))) {
				printf("[010] Expecting one entry in the list of slaves, found %d. Dumping list.\n", $tmp);
				var_dump($slaves);
			}
		}

		if ($last_used_connection != $pick_server_last_used) {
			printf("[011] Last used connection should be '%s' but got '%s'.\n", $pick_server_last_used, $last_used_connection);
		}

		$ret = "";
		$where = mysqlnd_ms_query_is_select($query);
		$server = '';
		switch ($where) {
			case MYSQLND_MS_QUERY_USE_LAST_USED:
			  $ret = $last_used_connection;
			  $server = 'last used';
			  break;
			case MYSQLND_MS_QUERY_USE_MASTER:
			  $ret = $master[0];
			  $server = 'master';
			  break;
			case MYSQLND_MS_QUERY_USE_SLAVE:
 			  $ret = $slaves[0];
			  $server = 'slave';
			  break;
			default:
			  printf("[012] Unknown return value from mysqlnd_ms_query_is_select, where = %s .\n", $where);
			  $ret = $master[0];
			  $server = 'unknown';
			  break;
		}

		printf("'%s' => %s\n", $query, $server);

		$pick_server_last_used = $ret;
		return $ret;
	}


	function run_query($offset, $link, $query, $expected) {
		global $queries;

		$queries[$query] = $query;
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

	function check_master_slave_threads($offset, $threads) {

		if (isset($threads["slave"]) && isset($threads["master"])) {
			foreach ($threads["slave"] as $thread_id => $num_queries) {
				if (isset($threads["master"][$thread_id])) {
					printf("[%03d + 01] Slave connection thread=%d is also a master connection!\n",
						$offset, $thread_id);
					unset($threads["slave"][$thread_id]);
				}
			}
			foreach ($threads["master"] as $thread_id => $num_queries) {
				if (isset($threads["slave"][$thread_id])) {
					printf("[%03d + 02] Master connection thread=%d is also a slave connection!\n",
						$offset, $thread_id);
				}
			}
		}
	}

	if (true !== mysqlnd_ms_set_user_pick_server("pick_server")) {
		printf("[001] Cannot install user callback for picking/selecting servers\n");
	}

	$threads = array();
	if (!$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket))
		printf("[002] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n",
			$host, $user, $db, $port, $socket);

	$threads["connect"] = $link->thread_id;
	$autocommit = true;
	$link->autocommit($autocommit);

	/* Should go to the first slave */
	$query = "SELECT 'Master Andrey has send this query to a slave.' AS _message FROM DUAL";
	$expected = array('_message' => 'Master Andrey has send this query to a slave.');
	run_query(20, $link, $query, $expected);
	if (!isset($threads["slave"][$link->thread_id])) {
		$threads["slave"][$link->thread_id] = 1;
	} else {
		$threads["slave"][$link->thread_id]++;
	}
	check_master_slave_threads(30, $threads);

	/* Should go to the first master */
	$query = sprintf("/*%s*/SELECT 'master' AS _message FROM DUAL", MYSQLND_MS_MASTER_SWITCH);
	$expected = array('_message' => 'master');
	run_query(40, $link, $query, $expected);
	if (!isset($threads["master"][$link->thread_id])) {
		$threads["master"][$link->thread_id] = 1;
	} else {
		$threads["master"][$link->thread_id]++;
	}
	check_master_slave_threads(50, $threads);

	/* Should go to the first master */
	$query = sprintf("/*%s*/SELECT 'master' AS _message FROM DUAL", MYSQLND_MS_MASTER_SWITCH);
	$expected = array('_message' => 'master');
	run_query(60, $link, $query, $expected);
	if (!isset($threads["master"][$link->thread_id])) {
		$threads["master"][$link->thread_id] = 1;
	} else {
		$threads["master"][$link->thread_id]++;
	}
	check_master_slave_threads(70, $threads);
	if ($threads["master"][$link->thread_id] != 2) {
		printf("[071] Master should have run 2 queries, records report %d\n", $threads["master"][$link->thread_id]);
	}

	/* Should go to the first slave */
	$query = sprintf("/*%s*/SELECT 'slave' AS _message FROM DUAL", MYSQLND_MS_SLAVE_SWITCH);
	$expected = array('_message' => 'slave');
	run_query(80, $link, $query, $expected);
	if (!isset($threads["slave"][$link->thread_id])) {
		$threads["slave"][$link->thread_id] = 1;
	} else {
		$threads["slave"][$link->thread_id]++;
	}
	check_master_slave_threads(90, $threads);
	if ($threads["slave"][$link->thread_id] != 2) {
		printf("[091] Slave should have run 2 queries, records report %d\n", $threads["slave"][$link->thread_id]);
	}

	$autocommit = false;
	$link->autocommit($autocommit);

	/* Should go to the first slave with in_transaction = true */
	$query = sprintf("/*%s*/SELECT 'slave' AS _message FROM DUAL", MYSQLND_MS_SLAVE_SWITCH);
	$expected = array('_message' => 'slave');
	run_query(100, $link, $query, $expected);
	if (!isset($threads["slave"][$link->thread_id])) {
		$threads["slave"][$link->thread_id] = 1;
	} else {
		$threads["slave"][$link->thread_id]++;
	}
	check_master_slave_threads(110, $threads);
	if ($threads["slave"][$link->thread_id] != 3) {
		printf("[111] Slave should have run 3 queries, records report %d\n", $threads["slave"][$link->thread_id]);
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_pick_server.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_pick_server.ini'.\n");
?>
--EXPECTF--
'SELECT 'Master Andrey has send this query to a slave.' AS _message FROM DUAL' => slave
'/*ms=master*/SELECT 'master' AS _message FROM DUAL' => master
'/*ms=master*/SELECT 'master' AS _message FROM DUAL' => master
'/*ms=slave*/SELECT 'slave' AS _message FROM DUAL' => slave
[004] in_transaction should be 1
[006] We are asked to handle the query 'SET AUTOCOMMIT=0' which has not been issued by us.
'SET AUTOCOMMIT=0' => master
'/*ms=slave*/SELECT 'slave' AS _message FROM DUAL' => slave
done!