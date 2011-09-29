<?php
	/*
	For whatever reasons run-tests.php sets new environment variables
	and gives no acceess to original environment :(. If it would at least
	preserve and overwrite only selected ones ...
	*/

	putenv("MYSQL_TEST_HOST=127.0.0.1");
	putenv("MYSQL_TEST_PORT=3306");
	putenv("MYSQL_TEST_USER=root");
	putenv("MYSQL_TEST_PASSWD=root");
	putenv("MYSQL_TEST_DB=test");
	putenv("MYSQL_TEST_ENGINE=MyISAM");
	putenv("MYSQL_TEST_SOCKET=");

	putenv("MYSQL_TEST_SKIP_CONNECT_FAILURE=1");
	putenv("MYSQL_TEST_CONNECT_FLAGS=0");
	putenv("MYSQL_TEST_EXPERIMENTAL=0");

	putenv("MYSQL_TEST_MASTER_HOST=". getenv("MYSQL_TEST_HOST"));
?>
