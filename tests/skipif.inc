<?php
require_once("connect.inc");

function _skipif_check_extensions($extensions) {
	if (!is_array($extensions))
		$extensions = array($extensions);

	foreach ($extensions as $k => $extension) {
		if (!extension_loaded($extension))
			die(sprintf("SKIP ext/%s not available"));
	}

}

function _skipif_connect($host, $user, $passwd, $db, $port, $socket) {
	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket))
		die(sprintf("SKIP Cannot connect to MySQL on host = '%s' using user = '%s', [%d] %s\n",
			$host, $user, mysqli_connect_errno(), mysqli_connect_error()));
}

function _skipif_check_feature($features) {
	if (!is_array($features))
		$features = array($features);

	foreach ($features as $k => $feature) {
		  switch ($feature) {
			  case "table_filter":
				if (!defined("MYSQLND_MS_HAVE_FILTER_TABLE_PARTITION"))
					die(sprintf("SKIP Table partitioning filter support not available"));
				break;

			  case "parser":
				break;

			  default:
				die(sprintf("SKIP Fix test! SKIPIF tests for unknown feature '%s'", $feature));
				break;
		  }
	}
}

_skipif_check_extensions("mysqlnd_ms");
?>