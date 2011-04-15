--TEST--
ReflectionExtension basics to check API
--SKIPIF--
<?php
require_once('skipif.inc');
?>
--FILE--
<?php
	$r = new ReflectionExtension("mysqlnd_ms");

	printf("Name: %s\n", $r->name);

	printf("Version: %s\n", $r->getVersion());
	if ($r->getVersion() != MYSQLND_MS_VERSION) {
		printf("[001] Expecting version '%s' got '%s'\n", MYSQLND_MS_VERSION, $r->getVersion());
	}

	$classes = $r->getClasses();
	if (!empty($classes)) {
		printf("[002] Expecting no class\n");
		var_dump($classes);
	}

	$dependencies = $r->getDependencies();
	printf("Dependencies:\n");
	foreach ($dependencies as $what => $how)
		printf("  %s - %s\n", $what, $how);

	$functions = $r->getFunctions();
	printf("Functions:\n");
	foreach ($functions as $func)
		printf("  %s\n", $func->name);

	print "done!";
?>
--EXPECTF--
Name: mysqlnd_ms
Version: 1.0.0-prototype
Dependencies:
  mysqlnd - Required
  standard - Required
Functions:
  mysqlnd_ms_set_user_pick_server
  mysqlnd_ms_query_is_select
  mysqlnd_ms_get_stats
done!