<?php
function get_servers_from_config($config, $user, $passwd, $db, $port, $socket) {
	$ret = array();
	$servers = array();
	$json = file_get_contents($config);
	$config = json_decode($json, true);
	foreach ($config as $section => $details) {
		if (isset($details['master'])) {
			foreach ($details['master'] as $m_name => $m_details) {
				$servers[$m_name] = $m_details;
			}
		}
		if (isset($details['slave'])) {
			foreach ($details['slave'] as $m_name => $m_details) {
				$servers[$m_name] = $m_details;
			}
		}
	}

	foreach ($servers as $name => $details) {
		$ret[$name] = array(
		  'host'		=> $details['host'],
		  'port' 		=> (isset($details['port']) && $details['port']) ? $details['port'] : $port,
		  'socket'		=> (isset($details['socket']) && $details['socket']) ? $details['socket'] : $socket,
		  'db'			=> (isset($details['db']) && $details['db']) ? $details['db'] : $db,
		  'user'		=> (isset($details['user']) && $details['user']) ? $details['user'] : $user,
		  'password'	=> (isset($details['password'])) ? $details['password'] : $passwd,
		);
	}
	return $ret;
}

function clean_server_id($config, $user, $passwd, $db, $port, $socket) {
	$servers = get_servers_from_config($config, $user, $passwd, $db, $port, $socket);
	foreach ($servers as $name => $details) {
		if (!$link = mysqli_connect($details['host'], $details['user'], $details['password'], $details['db'], $details['port'], $details['socket']))
		  continue;

		if (!$link->query("DROP TABLE IF EXISTS _mysqlnd_ms_server_id"))
		  continue;
	}
}

function set_server_ids($config, $user, $passwd, $db, $port, $socket) {

	$servers = get_servers_from_config($config, $user, $passwd, $db, $port, $socket);
	foreach ($servers as $name => $details) {
		if (!$link = mysqli_connect($details['host'], $details['user'], $details['password'], $details['db'], $details['port'], $details['socket']))
		  return sprintf("[%d] %s", mysqli_connect_errno(), mysqli_connect_error());

		if (!$link->query(sprintf("DROP TABLE IF EXISTS %s._mysqlnd_ms_server_id", $db)) ||
			!$link->query(sprintf("CREATE TABLE %s._mysqlnd_ms_server_id(server VARCHAR(255))", $db)) ||
			!$link->query(sprintf("INSERT INTO %s._mysqlnd_ms_server_id(server) VALUES('%s')", $db, $link->real_escape_string($name))))
		  return sprintf("[%d] %s\n", $link->errno, $link->error);

	}

	return "";
}

function get_server_id($link, $db) {
	$sql = sprintf("/*%s*/SELECT server AS _server_id FROM %s._mysqlnd_ms_server_id", MYSQLND_MS_LAST_USED_SWITCH, $db);
	if (!($res = $link->query($sql))) {
		printf("Cannot fetch server id, [%d] %s\n", $link->errno, $link->error);
		return NULL;
	}
	$row = $res->fetch_assoc();
	return $row['_server_id'];
}

?>
