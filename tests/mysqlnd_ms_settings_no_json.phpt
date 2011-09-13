--TEST--
Invalid config format
--SKIPIF--
<?php
require_once('skipif.inc');
require_once("connect.inc");

_skipif_check_extensions(array("mysqli"));
_skipif_connect($master_host_only, $user, $passwd, $db, $master_port, $master_socket);
_skipif_connect($slave_host_only, $user, $passwd, $db, $slave_port, $slave_socket);

if (FALSE === file_put_contents("test_mysqlnd_ms_settings_no_json.ini", "a\0gurken\0\nsalat\rli\t\n"))
	die(sprintf("SKIP Cannot write config file\n"));

?>
--INI--
mysqlnd_ms.enable=1
mysqlnd_ms.force_config=1
mysqlnd_ms.ini_file=test_mysqlnd_ms_settings_no_json.ini
mysqlnd_ms.in_regression_tests=1
--FILE--
<?php
	require_once("connect.inc");
	require_once("util.inc");

  $mst_ignore_errors = array(
		/* depends on test machine network configuration */
		'[E_WARNING] mysqli_real_connect(): [2002] Connection refused',
		'[E_WARNING] mysqli_real_connect(): (HY000/2002): Connection refused',
		'[E_WARNING] mysqli_real_connect(): php_network_getaddresses: getaddrinfo failed:',
		'[E_WARNING] mysqli_real_connect(): [2002] php_network_getaddresses: getaddrinfo failed:',
		'[E_WARNING] mysqli_real_connect(): (HY000/2002): php_network_getaddresses: getaddrinfo failed:',
	);
	set_error_handler('mst_error_handler');

	/* note that user etc are to be taken from the config! */
	if (!($link = mst_mysqli_connect("myapp", NULL, NULL, NULL, NULL, NULL)))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());

	print "done!";
?>
--CLEAN--
<?php
	if (!unlink("test_mysqlnd_ms_settings_no_json.ini"))
	  printf("[clean] Cannot unlink ini file 'test_mysqlnd_ms_settings_no_json.ini'.\n");
?>
--EXPECTF--
[001] [2002] %s
done!