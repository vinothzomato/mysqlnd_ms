<?php
	require_once("connect.inc");

	/*
	Error: 2002 (CR_CONNECTION_ERROR)
	Message: Can't connect to local MySQL server through socket '%s' (%d)
	Error: 2003 (CR_CONN_HOST_ERROR)
	Message: Can't connect to MySQL server on '%s' (%d)
	Error: 2005 (CR_UNKNOWN_HOST)
	Message: Unknown MySQL server host '%s' (%d)
	*/
	$connect_errno_codes = array(
		2002 => true,
		2003 => true,
		2005 => true,
	);

	function verbose_run_query($offset, $link, $query, $switch = NULL, $quiet = false, $expect_connect_warning = false, $ignore_error = false) {
		printf("[%03d + 01] Query '%s'\n", $offset, $query);
		$ret = run_query($offset, $link, $query, $switch = NULL, $quiet = false, $expect_connect_warning = false, $ignore_error = false);
		printf("[%03d + 02] Thread '%d'\n", $offset, $link->thread_id);
		return $ret;
	}

	function run_query($offset, $link, $query, $switch = NULL, $quiet = false, $expect_connect_warning = false, $ignore_error = false) {
		global $connect_errno_codes;

		$query = "/*$offset*/$query";

		if ($switch)
			$query = sprintf("/*%s*/%s", $switch, $query);

		if ($quiet) {
			/* Stupid PHP streams sometimes shout without caller being able to suppress */

			ob_start();
			$ret = $link->query($query);
			$haystack = ob_get_contents();
			ob_end_clean();

			if ($expect_connect_warning) {
				$found = false;
				foreach ($connect_errno_codes as $code => $v) {
					$needle = "Warning: mysqli::query(): [" . $code . "]";
					if (false !== ($found = strpos($haystack, $needle))) {
						break;
					}
				}
				if (!$found) {
					printf("[%03d] Cannot find expect connect warning, got '%s'\n", $offset, $haystack);
				}
			}

		} else {
			$ret = $link->query($query);
		}

		if (!$ret && !$ignore_error && $link->errno) {
			if (isset($connect_errno_codes[$link->errno]))
				printf("Connect error, ");
			printf("[%03d] [%d] %s\n", $offset, $link->errno, $link->error);
		}

		return $ret;
	}


	function run_real_query($offset, $link, $query, $switch = NULL, $quiet = false, $expect_connect_warning = false, $ignore_error = false) {
		global $connect_errno_codes;

		$query = "/*$offset*/$query";

		if ($switch)
			$query = sprintf("/*%s*/%s", $switch, $query);

		if ($quiet) {
			/* Stupid PHP streams sometimes shout without caller being able to suppress */

			ob_start();
			$ret = $link->query($query);
			$haystack = ob_get_contents();
			ob_end_clean();

			if ($expect_connect_warning) {
				$found = false;
				foreach ($connect_errno_codes as $code => $v) {
					$needle = "Warning: mysqli::query(): [" . $code . "]";
					if (false !== ($found = strpos($haystack, $needle))) {
						break;
					}
				}
				if (!$found) {
					printf("[%03d] Cannot find expect connect warning, got '%s'\n", $offset, $haystack);
				}
			}

		} else {
			$ret = $link->real_query($query);
		}

		if (!$ret && !$ignore_error && $link->errno) {
			if (isset($connect_errno_codes[$link->errno]))
				printf("Connect error, ");
			printf("[%03d] [%d] %s\n", $offset, $link->errno, $link->error);
		}

		return $ret;
	}

	function schnattertante($res) {
		if (!is_object($res))
		  return false;

		$row = $res->fetch_assoc();
		$res->close();
		printf("This is '%s' speaking\n", $row['_role']);
		return true;
	}

	function compare_stats() {
		static $last_stats = NULL;
		if (is_null($last_stats)) {
			$last_stats = mysqlnd_ms_get_stats();
			return;
		}
		$stats = mysqlnd_ms_get_stats();
		foreach ($stats as $k => $v) {
			if ($last_stats[$k] != $v) {
				printf("Stats %s: %d\n", $k, $v);
			}
		}
		$last_stats = $stats;
	}

	function create_test_table($host, $user, $passwd, $db, $port, $socket, $tablename = "test") {
		$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
		if (mysqli_connect_errno()) {
			return sprintf("[%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
		}

		if (!$link->query(sprintf("DROP TABLE IF EXISTS %s", $link->real_escape_string($tablename))) ||
			!$link->query(sprintf("CREATE TABLE %s(id INT)", $link->real_escape_string($tablename))) ||
			!$link->query(sprintf("INSERT INTO %s(id) VALUES (1), (2), (3), (4), (5)", $link->real_escape_string($tablename))))
			return sprintf("[%d] %s\n", $link->errno, $link->error);

		return '';
	}

	function drop_test_table($host, $user, $passwd, $db, $port, $socket, $tablename = "test") {
		$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
		if (mysqli_connect_errno()) {
			return sprintf("[%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
		}
		if (!$link->query(sprintf("DROP TABLE IF EXISTS %s", $link->real_escape_string($tablename))))
			return sprintf("[%d] %s\n", $link->errno, $link->error);

		return '';
	}
?>