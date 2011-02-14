--TEST--
Simple
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.force_config_usage=0
mysqlnd_ms.ini_file=test_mysqlnd_ms_simple.ini
--SKIPIF--
<?php
require_once("connect.inc");
require_once('skipif.inc');
file_put_contents("test_mysqlnd_ms_simple.ini", implode("\n", array("[phpBB]", "master=$host", "slave[]=$host", "slave[]=$host")));
?>
--FILE--
<?php
	require_once("connect.inc");

	$link = my_mysqli_connect("phpBB", $user, $passwd, "", $port, $socket);

	var_dump($link->thread_id);
	var_dump($res=$link->query("/*ms=slave*/SELECT user from mysql.user"));
	var_dump($res2=$link->query("/*ms=master*/SELECT user from mysql.user"));
	var_dump($res3=$link->query("/*ms=master*/SELECT user from mysql.user"));


	print "done!";
?>
--CLEAN--
<?php
	unlink("test_mysqlnd_ms_simple.ini");
?>
--EXPECTF--
int(%d)
object(mysqli_result)#%d (5) {
  ["current_field"]=>
  int(0)
  ["field_count"]=>
  int(1)
  ["lengths"]=>
  NULL
  ["num_rows"]=>
  int(%d)
  ["type"]=>
  int(0)
}
object(mysqli_result)#%d (5) {
  ["current_field"]=>
  int(0)
  ["field_count"]=>
  int(1)
  ["lengths"]=>
  NULL
  ["num_rows"]=>
  int(%d)
  ["type"]=>
  int(0)
}
object(mysqli_result)#%d (5) {
  ["current_field"]=>
  int(0)
  ["field_count"]=>
  int(1)
  ["lengths"]=>
  NULL
  ["num_rows"]=>
  int(%d)
  ["type"]=>
  int(0)
}
done!