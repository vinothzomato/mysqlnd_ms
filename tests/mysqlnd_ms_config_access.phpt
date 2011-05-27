--TEST--
Concurrent config file access
--SKIPIF--
<?php
require_once('skipif_mysqli.inc');
require_once("connect.inc");

if (!function_exists('pcntl_fork'))
	die("skip Process Control Functions not available");

if (!function_exists('posix_getpid'))
	die("skip POSIX functions not available");

$settings = array(
	"myapp" => array(
		'master' => array($master_host),
		'slave' => array($slave_host),
	),

);
if ($error = create_config("test_mysqlnd_ms_config_access.ini", $settings))
	die(sprintf("SKIP %d\n", $error));
?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_config_access.ini
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

	/* something easy to start with... */

	if (!($link = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	/* This shall work because we have two slaves */
	$pid = pcntl_fork();
	switch ($pid) {
		case -1:
			printf("[002] Cannot fork child");
			break;

		case 0:
			/* child */
			for ($i = 0; $i < 1; $i++) {
				if (!($clink = my_mysqli_connect("myapp", $user, $passwd, $db, $port, $socket))) {
					printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
					continue;
				}

				$res = mysqli_query($clink, "SELECT 'child' AS _message");
				if (!$res)
					printf("[003] Child cannot store results, [%d] %s\n", $clink->errno, $clink->error);
				else {
					if (!$row = $res->fetch_assoc())
						printf("[004] Child cannot fetch results\n");
					if ($row['_message'] != 'child')
						printf("[005] Expecting 'child' got '%s'\n", $row['_message']);
					$res->free();
				}
				$clink->close();
			}
			exit(0);
			break;

		default:
			/* parent */
			$status = null;
			$wait_id = pcntl_waitpid($pid, $status);
			if (pcntl_wifexited($status) && (0 != ($tmp = pcntl_wexitstatus($status)))) {
				printf("Exit code: %s\n", (pcntl_wifexited($status)) ? pcntl_wexitstatus($status) : 'n/a');
				printf("Signal: %s\n", (pcntl_wifsignaled($status)) ? pcntl_wtermsig($status) : 'n/a');
				printf("Stopped: %d\n", (pcntl_wifstopped($status)) ? pcntl_wstopsig($status) : 'n/a');
			}
			break;
	}

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_config_access.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_config_access.ini'.\n");
?>
--EXPECTF--
done!