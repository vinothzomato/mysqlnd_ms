--TEST--
Limits: all prepared statements go to the master
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host),
	),
);
if ($error = create_config("test_mysqlnd_prepared_statement.ini", $settings))
	die(sprintf("SKIP %d\n", $error));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_prepared_statement.ini
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

	run_query(3, $link, "SET @myrole='master'", MYSQLND_MS_MASTER_SWITCH);
	run_query(4, $link, "SET @myrole='slave'", MYSQLND_MS_SLAVE_SWITCH);

	if (!$stmt = $link->prepare("SELECT @myrole AS _role"))
		printf("[005] [%d] %s\n", $link->errno, $link->error);

	if (!$stmt->execute())
		printf("[006] [%d] %s\n", $stmt->errno, $stmt->error);

	$role = NULL;
	if (!$stmt->bind_result($role))
		printf("[007] [%d] %s\n", $stmt->errno, $stmt->error);

	while ($stmt->fetch())
		printf("Role = '%s'\n", $role);

	$stmt->close();

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_prepared_statement.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_prepared_statement.ini'.\n");
?>
--EXPECTF--
Role = 'master'
done!