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
	$mst_connect_errno_codes = array(
		2002 => true,
		2003 => true,
		2005 => true,
	);

	function mst_mysqli_verbose_query($offset, $link, $query, $switch = NULL, $quiet = false, $expect_connect_warning = false, $ignore_error = false) {
		printf("[%03d + 01] Query '%s'\n", $offset, $query);
			$ret = mst_mysqli_query($offset, $link, $query, $switch = NULL, $quiet = false, $expect_connect_warning = false, $ignore_error = false);
		printf("[%03d + 02] Thread '%d'\n", $offset, $link->thread_id);
		return $ret;
	}

	function mst_mysqli_query($offset, $link, $query, $switch = NULL, $quiet = false, $expect_connect_warning = false, $ignore_error = false) {
		global $mst_connect_errno_codes;

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
				foreach ($mst_connect_errno_codes as $code => $v) {
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
			if (isset($mst_connect_errno_codes[$link->errno]))
				printf("Connect error, ");
			printf("[%03d] [%d] %s\n", $offset, $link->errno, $link->error);
		}

		return $ret;
	}


	function mst_mysqli_real_query($offset, $link, $query, $switch = NULL, $quiet = false, $expect_connect_warning = false, $ignore_error = false) {
		global $mst_connect_errno_codes;

		$query = "/*$offset*/$query";

		if ($switch)
			$query = sprintf("/*%s*/%s", $switch, $query);

		if ($quiet) {
			/* Stupid PHP streams sometimes shouts without caller being able to suppress */

			ob_start();
			$ret = $link->real_query($query);
			$haystack = ob_get_contents();
			ob_end_clean();

			if ($expect_connect_warning) {
				$found = false;
				foreach ($mst_connect_errno_codes as $code => $v) {
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
			if (isset($mst_connect_errno_codes[$link->errno]))
				printf("Connect error, ");
			printf("[%03d] [%d] %s\n", $offset, $link->errno, $link->error);
		}

		return $ret;
	}

	function mst_mysqli_fech_role($res) {
		if (!is_object($res))
			return false;

		$row = $res->fetch_assoc();
		$res->close();
		printf("This is '%s' speaking\n", $row['_role']);
		return true;
	}

	function mst_compare_stats() {
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

	function mst_mysqli_create_test_table($host, $user, $passwd, $db, $port, $socket, $tablename = "test") {
		$link = mst_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
		if (mysqli_connect_errno()) {
			return sprintf("[%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
		}

		if (!$link->query(sprintf("DROP TABLE IF EXISTS %s", $link->real_escape_string($tablename))) ||
			!$link->query(sprintf("CREATE TABLE %s(id INT)", $link->real_escape_string($tablename))) ||
			!$link->query(sprintf("INSERT INTO %s(id) VALUES (1), (2), (3), (4), (5)", $link->real_escape_string($tablename))))
			return sprintf("[%d] %s\n", $link->errno, $link->error);

		return '';
	}

	function mst_mysqli_drop_test_table($host, $user, $passwd, $db, $port, $socket, $tablename = "test") {
		$link = mst_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
		if (mysqli_connect_errno()) {
			return sprintf("[%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
		}
		if (!$link->query(sprintf("DROP TABLE IF EXISTS %s", $link->real_escape_string($tablename))))
			return sprintf("[%d] %s\n", $link->errno, $link->error);

		return '';
	}

	function mst_mysqli_fetch_id($offset, $res) {
		if (!$res) {
			printf("[%03d] No result\n", $offset);
			return;
		}
		$row = $res->fetch_assoc();
		printf("[%03d] _id = '%s'\n", $offset, $row['_id']);
	}

	function mst_mysqli_server_supports_query($offset, $sql, $host, $user, $passwd, $db, $port, $socket) {
		$link = mst_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
		if (mysqli_connect_errno()) {
			printf("[%03d] [%d] %s\n", $offset, mysqli_connect_errno(), mysqli_connect_error());
		}
		printf("[%03d] Testing server support of '%s'\n", $offset, $sql);
		return $link->query($sql);
	}

	function msg_mysqli_init_emulated_id_skip($host, $user, $passwd, $db, $port, $socket, $role) {
		if (true !== ($msg = _mst_mysqli_init_emulated_id(1, $host, $user, $passwd, $db, $port, $socket, $role)))
			die(sprintf("SKIP Cannot setup emulated server id, %s", $msg));
	}

	function mst_mysqli_init_emulated_id($offset, $host, $user, $passwd, $db, $port, $socket, $role) {
		if (true !== ($msg = _mst_mysqli_init_emulated_id($offset, $host, $user, $passwd, $db, $port, $socket, $role)))
			echo $msg;

		return true;
	}

 	function _mst_mysqli_init_emulated_id($offset, $host, $user, $passwd, $db, $port, $socket, $role) {
		$link = mst_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
		if (mysqli_connect_errno()) {
			return sprintf("[%03d] [%d] %s\n", $offset, mysqli_connect_errno(), mysqli_connect_error());
		}

		if (!$link->query("DROP TABLE IF EXISTS _mysqlnd_ms_roles") ||
			!$link->query("CREATE TABLE _mysqlnd_ms_roles(role VARCHAR(255) NOT NULL)") ||
			!$link->query(sprintf("INSERT INTO _mysqlnd_ms_roles(role) values ('%s')", $link->real_escape_string($role)))) {
			return sprintf("[%03d] [%d] %s\n", $offset, $link->errno, $link->error);
		}

		return true;
	}

	function mst_mysqli_get_emulated_id($offset, $link) {

		$thread_id = $link->thread_id;
		if (0 == $thread_id)
			return NULL;

		$query = sprintf("/*%s*//*%d*//*util.inc*/SELECT role FROM _mysqlnd_ms_roles", MYSQLND_MS_LAST_USED_SWITCH, $offset);
		if (!($res = $link->query($query))) {
			printf("[%03d] [%d] %s\n", $offset, $link->errno, $link->error);
			return NULL;
		}

		if ($res->num_rows > 1) {
			printf("[%03d] _mysqlnd_ms_roles holds more than one row\n", $offset);
			return NULL;
		}

		if ($thread_id != $link->thread_id) {
			printf("[%03] Unexpected connection switch\n");
			return NULL;
		}

		$row = $res->fetch_assoc();
		return sprintf("%s-%d", $row['role'], $link->thread_id);
	}
?>