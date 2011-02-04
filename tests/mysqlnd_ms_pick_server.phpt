--TEST--
Pick Server
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.force_config_usage=0
mysqlnd_ms.ini_file=mysqlnd_ms.ini
--SKIPIF--
<?php
require_once("connect.inc");
require_once('skipif.inc');
file_put_contents("mysqlnd_ms.ini", implode("\n", array("[phpBB]", "master=$host", "slave[]=$host", "slave[]=$host")));
?>
--FILE--
<?php
	function pick_server($query, $m_list, $s_list) {
		var_dump($query, $m_list, $s_list);
		return $m_list[0];
	}
	mysqlnd_ms_set_user_pick_server("pick_server");

	require_once("connect.inc");
	$link = my_mysqli_connect("phpBB", $user, $passwd, "", $port, $socket);

	var_dump($link->thread_id);
	$res=$link->query("/*ms=slave*/SELECT user from mysql.user");
	$res2=$link->query("/*ms=master*/SELECT user from mysql.user");
	$res3=$link->query("/*ms=master*/SELECT user from mysql.user");


	print "done!";
?>
--CLEAN--
<?php
	unlink("mysqlnd_ms.ini");
?>
--EXPECTF--
int(%d)
string(39) "/*ms=slave*/SELECT user from mysql.user"
array(1) {
  [0]=>
  string(%d) "%s"
}
array(2) {
  [0]=>
  string(%d) "%s"
  [1]=>
  string(%d) "%s"
}
string(40) "/*ms=master*/SELECT user from mysql.user"
array(1) {
  [0]=>
  string(%d) "%s"
}
array(2) {
  [0]=>
  string(%d) "%s"
  [1]=>
  string(%d) "%s"
}
string(40) "/*ms=master*/SELECT user from mysql.user"
array(1) {
  [0]=>
  string(%d) "%s"
}
array(2) {
  [0]=>
  string(%d) "%s"
  [1]=>
  string(%d) "%s"
}
done!