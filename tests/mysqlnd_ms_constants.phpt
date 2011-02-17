--TEST--
Constants
--SKIPIF--
<?php
require_once('skipif.inc');
?>
--FILE--
<?php
	$expected = array(
		"MYSQLND_MS_VERSION" => true,
		"MYSQLND_MS_VERSION_ID" => true,

		/* SQL hints */
		"MYSQLND_MS_MASTER_SWITCH" => true,
		"MYSQLND_MS_SLAVE_SWITCH" => true,
		"MYSQLND_MS_LAST_USED_SWITCH" => true,

		/* Return values of mysqlnd_ms_is_select() */
		"MYSQLND_MS_QUERY_USE_LAST_USED" => true,
		"MYSQLND_MS_QUERY_USE_MASTER" => true,
		"MYSQLND_MS_QUERY_USE_SLAVE" => true,
	);

	$constants = get_defined_constants(true);
	$constants = (isset($constants['mysqlnd_ms'])) ? $constants['mysqlnd_ms'] : array();
	ksort($constants);
	foreach ($constants as $name => $value) {
		if (!isset($expected[$name])) {
			printf("[001] Unexpected constants: %s/%s\n", $name, $value);
		} else {
			printf("%s = '%s'\n", $name, $value);
			unset($expected[$name]);
		}
	}
	if (!empty($expected)) {
		printf("[002] Dumping list of missing constants\n");
		var_dump($expected);
	}

	print "done!";
?>
--EXPECTF--
MYSQLND_MS_LAST_USED_SWITCH = 'ms=last_used'
MYSQLND_MS_MASTER_SWITCH = 'ms=master'
MYSQLND_MS_QUERY_USE_LAST_USED = '2'
MYSQLND_MS_QUERY_USE_MASTER = '0'
MYSQLND_MS_QUERY_USE_SLAVE = '1'
MYSQLND_MS_SLAVE_SWITCH = 'ms=slave'
MYSQLND_MS_VERSION = '1.0.0-prototype'
MYSQLND_MS_VERSION_ID = '10000'
done!