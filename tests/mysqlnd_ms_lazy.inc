<?php
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

	function run_query($offset, $link, $query, $switch = NULL, $quiet = false) {
		global $connect_errno_codes;
		if ($switch)
			$query = sprintf("/*%s*/%s", $switch, $query);

		if ($quiet)
			$ret = @$link->query($query);
		else
			$ret = $link->query($query);

		if (!$ret && !$quiet) {
			if (isset($connect_errno_codes[$link->errno]))
				printf("Expected error, ");
			printf("[%03d] [%d] %s\n", $offset, $link->errno, $link->error);
		}

		return $ret;
	}

	function schnattertante($res) {
		if (!$res)
		  return false;
		$row = $res->fetch_assoc();
		$res->close();
		printf("This is '%s' speaking\n", $row['_role']);
		return true;
	}
?>