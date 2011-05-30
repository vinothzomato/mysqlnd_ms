<?php
	/*
	Default values are "localhost", "root",
	database "stest" and empty password.
	Change the MYSQL_TEST environment values
	if you want to use another configuration
	*/

	$driver    = new mysqli_driver;

	$host      = getenv("MYSQL_TEST_HOST")     ? getenv("MYSQL_TEST_HOST") : "localhost";
	$port      = getenv("MYSQL_TEST_PORT")     ? getenv("MYSQL_TEST_PORT") : 3306;
	$user      = getenv("MYSQL_TEST_USER")     ? getenv("MYSQL_TEST_USER") : "root";
	$passwd    = getenv("MYSQL_TEST_PASSWD")   ? getenv("MYSQL_TEST_PASSWD") : "root";
	$db        = getenv("MYSQL_TEST_DB")       ? getenv("MYSQL_TEST_DB") : "test";
	$engine    = getenv("MYSQL_TEST_ENGINE")   ? getenv("MYSQL_TEST_ENGINE") : "MyISAM";
	$socket    = getenv("MYSQL_TEST_SOCKET")   ? getenv("MYSQL_TEST_SOCKET") : null;
	$skip_on_connect_failure  = getenv("MYSQL_TEST_SKIP_CONNECT_FAILURE") ? getenv("MYSQL_TEST_SKIP_CONNECT_FAILURE") : true;
	$connect_flags = getenv("MYSQL_TEST_CONNECT_FLAGS") ? (int)getenv("MYSQL_TEST_CONNECT_FLAGS") : 0;
	if ($socket) {
		ini_set('mysqli.default_socket', $socket);
	}

	/* We assume that user/password are the same for all machines but alternative port/socket can be configured like host[;port|socket] */
	$master_host = $master_host_only = getenv("MYSQL_TEST_MASTER_HOST") ? getenv("MYSQL_TEST_MASTER_HOST") : $host;
	$master_socket = $socket;
	$master_port = $port;
	if (false !== ($pos = strpos($master_host, ':'))) {
		if ('/' == substr($master_host, $pos + 1, 1)) {
			$master_socket = substr($master_host, $pos + 1);
			$master_port = NULL;
		} else {
			$master_socket = NULL;
			$master_port = substr($master_host, $pos + 1);
		}
		$master_host_only = substr($master_host, 0, $pos);
	}

	$slave_host = $slave_host_only = getenv("MYSQL_TEST_SLAVE_HOST") ? getenv("MYSQL_TEST_SLAVE_HOST") : $host;
	$slave_socket = $socket;
	$slave_port = $port;
	if (false !== ($pos = strpos($slave_host, ':'))) {
		if ('/' == substr($slave_host, $pos + 1, 1)) {
			$slave_socket = substr($slave_host, $pos + 1);
			$slave_port = NULL;
		} else {
			$slave_socket = NULL;
			$slave_port = substr($slave_host, $pos + 1);
		}
		$slave_host_only = substr($slave_host, 0, $pos);
	}
	/* Development setting: test experimal features and/or feature requests that never worked before? */
	$TEST_EXPERIMENTAL = (in_array(getenv("MYSQL_TEST_EXPERIMENTAL"), array(0, 1))) ?
				((1 == getenv("MYSQL_TEST_EXPERIMENTAL")) ? true : false) :
				false;

	$IS_MYSQLND = stristr(mysqli_get_client_info(), "mysqlnd");
	if (!$IS_MYSQLND) {
		$MYSQLND_VERSION = NULL;
	} else {
		/*
		The formatting of the version reported by mysqli_get_client_info()
		has changed significantly in the past. To get tests working properly
		with PHP 5.3.0 and up, we set everything that looks like prior to
		PHP 5.3.0 to version 5.0.4 = 5 * 10000 + 0 * 100 + 4 = 50004.
		PHP 5.3.0	reports mysqlnd 5.0.5 dev (= 5 * 10000 + 0 * 100 + 5 = 50005.
		*/
		if (preg_match('@Revision:\s+(\d+)\s*\$@ism', mysqli_get_client_info(), $matches)) {
			/* something prior to PHP 5.3.0 */
			$MYSQLND_VERSION = 50004;
		} else if (preg_match('@^mysqlnd (\d+)\.(\d+)\.(\d+).*@ism', mysqli_get_client_info(), $matches)) {
			/* formatting schema used by PHP 5.3.0 */
			$MYSQLND_VERSION = (int)$matches[1] * 10000 + (int)$matches[2] * 100 + (int)$matches[3];
		} else if (preg_match('@^mysqlnd/PHP 6.0.0-dev@ism', mysqli_get_client_info(), $matches)) {
			/*
				PHP 6.0 at the time of the first PHP 5.3.0 release.
				HEAD and 5.3 have been in sync when 5.3.0 was released.
				It is at least 5.0.5-dev.
			*/
			$MYSQLND_VERSION = 50005;
		} else {
			/* unknown */
			$MYSQLND_VERSION = -1;
		}

	}

	if (!function_exists('sys_get_temp_dir')) {
		function sys_get_temp_dir() {

			if (!empty($_ENV['TMP']))
				return realpath( $_ENV['TMP'] );
			if (!empty($_ENV['TMPDIR']))
				return realpath( $_ENV['TMPDIR'] );
			if (!empty($_ENV['TEMP']))
				return realpath( $_ENV['TEMP'] );

			$temp_file = tempnam(md5(uniqid(rand(), TRUE)), '');
			if ($temp_file) {
				$temp_dir = realpath(dirname($temp_file));
				unlink($temp_file);
				return $temp_dir;
			}
			return FALSE;
		}
	}

	if (!function_exists('my_mysqli_connect')) {

		/**
		* Whenever possible, please use this wrapper to make testing ot MYSQLI_CLIENT_COMPRESS (and potentially SSL) possible
		*
		* @param enable_env_flags Enable setting of connection flags through 	env(MYSQL_TEST_CONNECT_FLAGS)?
		*/
		function my_mysqli_connect($host, $user, $passwd, $db, $port, $socket, $enable_env_flags = true) {
			global $connect_flags;

			$flags = ($enable_env_flags) ? $connect_flags : false;

			if ($flags !== false) {
				$link = mysqli_init();
				if (!mysqli_real_connect($link, $host, $user, $passwd, $db, $port, $socket, $flags))
					$link = false;
			} else {
				$link = mysqli_connect($host, $user, $passwd, $db, $port, $socket);
			}

			return $link;
		}

		/**
		* Whenever possible, please use this wrapper to make testing ot MYSQLI_CLIENT_COMPRESS (and potentially SSL) possible
		*
		* @param enable_env_flags Enable setting of connection flags through env(MYSQL_TEST_CONNECT_FLAGS)
		*/
		function my_mysqli_real_connect($link, $host, $user, $passwd, $db, $port, $socket, $flags = 0, $enable_env_flags = true) {
			global $connect_flags;

			if ($enable_env_flags)
				$flags & $connect_flags;

			return mysqli_real_connect($link, $host, $user, $passwd, $db, $port, $socket, $flags);
		}

		class my_mysqli extends mysqli {
			public function __construct($host, $user, $passwd, $db, $port, $socket, $enable_env_flags = true) {
				global $connect_flags;

				$flags = ($enable_env_flags) ? $connect_flags : false;

				if ($flags !== false) {
					parent::init();
					$this->real_connect($host, $user, $passwd, $db, $port, $socket, $flags);
				} else {
					parent::__construct($host, $user, $passwd, $db, $port, $socket);
				}
			}
		}

		function my_get_charsets($link) {

			/* Those tree are set by SET NAMES */
			$charsets = array(
				'client' 		=> NULL,
				'results'		=> NULL,
				'connection'	=> NULL,
			);

			if (!($res = mysqli_query($link, "SHOW VARIABLES LIKE '%character%'"))) {
				printf("[%d] %s\n", mysqli_errno($link), mysqli_error($link));
				return $charsets;
			}

			$names = array();
			while ($row = mysqli_fetch_assoc($res)) {
				$names[$row['Variable_name']] = $row['Value'];
			}
			mysqli_free_result($res);

			if (!($res = mysqli_query($link, sprintf("SHOW CHARACTER SET LIKE '%s'", $names['character_set_client']))) ||
				!($details = mysqli_fetch_assoc($res))) {
				printf("[%d] %s\n", mysqli_errno($link), mysqli_error($link));
				return $charsets;
			}
			mysqli_free_result($res);

			$charsets['client'] = array(
				'charset' 	=> $details['Charset'],
				'desc'		=> $details['Description'],
				'collation'	=> $details['Default collation'],
				'maxlen'	=> $details['Maxlen'],
				'nr'		=> NULL,
			);

			if (!($res = mysqli_query($link, sprintf("SHOW COLLATION LIKE '%s'", $details['Default collation']))) ||
				!($collation = mysqli_fetch_assoc($res))) {
				printf("[%d] %s\n", mysqli_errno($link), mysqli_error($link));
				return $charsets;
			}
			mysqli_free_result($res);
			$charsets['client']['nr'] = $collation['Id'];

			if (!($res = mysqli_query($link, sprintf("SHOW CHARACTER SET LIKE '%s'", $names['character_set_results']))) ||
				!($details = mysqli_fetch_assoc($res))) {
				printf("[%d] %s\n", mysqli_errno($link), mysqli_error($link));
				return $charsets;
			}
			mysqli_free_result($res);

			$charsets['results'] = array(
				'charset' 	=> $details['Charset'],
				'desc'		=> $details['Description'],
				'collation'	=> $details['Default collation'],
				'maxlen'	=> $details['Maxlen'],
				'nr'		=> NULL,
			);

			if (!($res = mysqli_query($link, sprintf("SHOW COLLATION LIKE '%s'", $details['Default collation']))) ||
				!($collation = mysqli_fetch_assoc($res))) {
				printf("[%d] %s\n", mysqli_errno($link), mysqli_error($link));
				return $charsets;
			}
			mysqli_free_result($res);
			$charsets['results']['nr'] = $collation['Id'];


			if (!($res = mysqli_query($link, sprintf("SHOW CHARACTER SET LIKE '%s'", $names['character_set_connection']))) ||
				!($details = mysqli_fetch_assoc($res))) {
				printf("[%d] %s\n", mysqli_errno($link), mysqli_error($link));
				return $charsets;
			}
			mysqli_free_result($res);

			$charsets['connection'] = array(
				'charset' 	=> $details['Charset'],
				'desc'		=> $details['Description'],
				'collation'	=> $details['Default collation'],
				'maxlen'	=> $details['Maxlen'],
				'nr'		=> NULL,
			);

			if (!($res = mysqli_query($link, sprintf("SHOW COLLATION LIKE '%s'", $details['Default collation']))) ||
				!($collation = mysqli_fetch_assoc($res))) {
				printf("[%d] %s\n", mysqli_errno($link), mysqli_error($link));
				return $charsets;
			}
			mysqli_free_result($res);
			$charsets['connection']['nr'] = $collation['Id'];

			return $charsets;
		}

		if (extension_loaded("pdo_mysql")) {
			function my_pdo_connect($host, $user, $passwd, $db, $port, $socket, $options = NULL) {

				if (is_null($options))
					$options = array(
					  PDO::ATTR_EMULATE_PREPARES => true,
					  PDO::MYSQL_ATTR_DIRECT_QUERY => true,
					);

				if ($port) {
				  $dsn = sprintf("mysql:host=%s;port=%d;dbname=%s", $host, $port, $db);
				} else {
				  $dsn = sprintf("mysql:host=%s;unix_socket=%s;dbname=%s", $host, $socket, $db);
				}
				return new PDO($dsn, $user, $passwd, $options);
			}
		}

	} else {
		printf("skip Eeeek/BUG/FIXME - connect.inc included twice! skipif bug?\n");
	}

	if (!function_exists("create_config")) {
		function create_config($file, $settings, $replace_file = true, $format = 'json') {

			switch ($format) {
			  case 'json':
				return create_config_json($file, $settings, $replace_file);
				break;
			  case 'ini':
			  default:
				return create_config_ini($file, $settings, $replace_file);
				break;
			}

		}

		function create_config_json($file, $settings, $replace_file) {
/*
{
	"myapp":
		{
			"lazy_connections":0,
			"master":
				[
					{
						"host"	: "127.0.0.1",
						"port"	: 3306,
						"socket": "/tmp/mysql.sock",
						"db"	: "test",
						"user"	: "root",
						"password": "root"
					}
				],
			"slave":
				{
					"slave1" :
						{
							"host":"127.0.0.1",
							"port":3307,
							"socket":"/tmp/mysql.sock",
							"db":"test",
							"user":"root",
							"password":"root",
							"table_filters":
								{
									"db1.%":{ "priority" : 1},
									"db2.t%":{ "priority" : 2}
								}
						},
					"slave2" :
						{
							"host" : "localhost",
							"socket":"/tmp/mysql3307.sock",
							"db":"test",
							"user":"root",
							"password":"root",
							"table_filters":
								{
									"db1.%":{ "priority" : 2},
									"db2.t%":{ "priority" : 1}
								}
						}
				}
		}
}
*/
			if ($replace_file && file_exists($file) && !@unlink($file))
				return sprintf("Cannot unlink existing file '%s'.", $file);

			if (!$fp = @fopen($file, "w"))
				return sprintf("Cannot open file '%s' for writing.", $file);

			$json = array();
			foreach ($settings as $application => $config) {

				$json[$application] = array();
				if (isset($config['master'])) {
					foreach ($config['master'] as $host) {
						$host_config = array();
						if (false !== stristr($host, ':')) {
							/* 1.0 legacy */
							list($real_host, $port) = explode(':', $host);
							$host_config['host'] = $real_host;
							if ($port) {
								if (!is_numeric($port)) {
									$host_config['socket'] = $port;
								} else {
									$host_config['port'] = $port;
								}
							}
						} else {
							$host_config['host'] = $host;
						}
						$json[$application]['master'][] = $host_config;
					}
				}
				if (isset($config['slave'])) {
					foreach ($config['slave'] as $host) {
						$host_config = array();
						/* 1.0 legacy */
						if (false !== stristr($host, ':')) {
							list($real_host, $port) = explode(':', $host);
							$host_config['host'] = $real_host;
							if ($port) {
								if (!is_numeric($port)) {
									$host_config['socket'] = $port;
								} else {
									$host_config['port'] = $port;
								}
							}
						} else {
							$host_config['host'] = $host;
						}
						$json[$application]['slave'][] = $host_config;
					}
				}
				if (isset($config['pick'])) {
					if (!is_array($config['pick']))
						$config['pick'] = array($config['pick']);

					foreach ($config['pick'] as $method) {
						$json[$application]['pick'][] = (string)$method;
					}
				}
				if (isset($config['lazy_connections'])) {
				  $json[$application]['lazy_connections'] = (string)$config['lazy_connections'];
				}
				if (isset($config['failover'])) {
				  $json[$application]['failover'] = (string)$config['failover'];
				}
				if (isset($config['master_on_write'])) {
				  $json[$application]['master_on_write'] = (string)$config['master_on_write'];
				}
				if (isset($config['trx_stickiness'])) {
				  $json[$application]['trx_stickiness'] = (string)$config['trx_stickiness'];
				}

			}

			fwrite($fp, json_encode($json));
			fclose($fp);
			return '';
		}

		function create_config_ini($file, $settings, $replace_file) {

			if ($replace_file && file_exists($file) && !@unlink($file))
				return sprintf("Cannot unlink existing file '%s'.", $file);

			if (!$fp = @fopen($file, "w"))
				return sprintf("Cannot open file '%s' for writing.", $file);

			foreach ($settings as $application => $config) {
				fwrite($fp, sprintf("[%s]\n", $application));
				if (isset($config['master'])) {
					foreach ($config['master'] as $host) {
						if (!fwrite($fp, sprintf("master[]=%s\n", $host)))
							return sprintf("Cannot write master settings to file.");
					}
				}
				if (isset($config['slave'])) {
					foreach ($config['slave'] as $host) {
						if (!fwrite($fp, sprintf("slave[]=%s\n", $host)))
							return sprintf("Cannot write slave settings to file.");
					}
				}

				if (isset($config['pick'])) {
					foreach ($config['pick'] as $method)
						if (!fwrite($fp, sprintf("pick[]=%s\n", $method)))
							return sprintf("Cannot write query redirection policy settings to file.");
				}

				if (isset($config['lazy_connections'])) {
					if (!fwrite($fp, sprintf("lazy_connections=%s\n", $config['lazy_connections'])))
							return sprintf("Cannot write query redirection/load balancing policy settings to file.");
				}

				if (isset($config['failover'])) {
					if (!fwrite($fp, sprintf("failover=%s\n", $config['failover'])))
							return sprintf("Cannot write failover policy settings to file.");
				}
				if (isset($config['master_on_write'])) {
					if (!fwrite($fp, sprintf("master_on_write=%s\n", $config['master_on_write'])))
							return sprintf("Cannot write master_on_write policy settings to file.");
				}
				if (isset($config['trx_stickiness'])) {
					if (!fwrite($fp, sprintf("trx_stickiness=%s\n", $config['trx_stickiness'])))
							return sprintf("Cannot write trx_stickiness policy settings to file.");
				}
			}
			fclose($fp);
			return '';
		}
	}
?>