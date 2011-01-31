--TEST--
Exportet constants
--SKIPIF--
<?php
require_once('skipif.inc');
?>
--FILE--
<?php
	$expected = array(
		"MYSQLND_MS_MASTER_SWITCH" => true,
		"MYSQLND_MS_SLAVE_SWITCH" => true,
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
MYSQLND_MS_MASTER_SWITCH = 'ms=master'
MYSQLND_MS_SLAVE_SWITCH = 'ms=slave'
done!