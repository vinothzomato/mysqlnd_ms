<?php
require_once("connect.inc");

function fetch_result($offset, $res) {
	if (!$res) {
		printf("[%03d] No result\n", $offset);
		return;
	}
	$row = $res->fetch_assoc();
	printf("[%03d] _id = '%s'\n", $offset, $row['_id']);
}

function server_supports_query($offset, $sql, $host, $user, $passwd, $db, $port, $socket) {
	$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[%03d] [%d] %s\n", $offset, mysqli_connect_errno(), mysqli_connect_error());
	}
	printf("[%03d] Testing server support of '%s'\n", $offset, $sql);
	return $link->query($sql);
}