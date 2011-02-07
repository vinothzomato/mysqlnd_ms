--TEST--
Pick Server
--SKIPIF--
<?php
require_once("connect.inc");
require_once('skipif.inc');

$settings = array(
	"myapp" => array(
		'master' => array($host),
		'slave' => array($host, $host),
	),
);
if ($error = create_config("mysqlnd_ms_pick_server.ini", $settings))
  die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=mysqlnd_ms_pick_server.ini
mysqlnd.debug=d:t:O,/tmp/pick_server
--FILE--
<?php
  require_once("connect.inc");
	function pick_server($connect_host, $query, $m_list, $s_list, $last_used_conn) {
		var_dump($connect_host, $query, $m_list, $s_list, $last_used_conn);
		return $m_list[0];
	}
	mysqlnd_ms_set_user_pick_server("pick_server");

	$threads = array();
	if (!$link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket))
		printf("[001] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n",
			$host, $user, $db, $port, $socket);

	$threads["connect"] = $link->thread_id;


	if (!$res = $link->query("SELECT 'Master Andrey has send this query to a slave.' FROM DUAL"))
	  printf("[002] [%d] %s\n", $link->errno, $link->error);
	$rows = $res->fetch_all();
	var_dump($rows);
	$res->close();
	$threads["slaves"][$link->thread_id] = 1;

	// var_dump($threads);

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("mysqlnd_ms_pick_server.ini"))
	  printf("[clean] Cannot unlink ini file 'mysqlnd_ms_pick_server.ini'.\n");
?>
--EXPECTF--
done!