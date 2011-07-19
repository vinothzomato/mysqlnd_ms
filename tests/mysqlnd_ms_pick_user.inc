<?php
	function grumble_catchable_fatal_grumble($errno, $error, $file, $line) {
		static $errcodes = array();
		if (empty($errcodes)) {
			$constants = get_defined_constants();
			foreach ($constants as $name => $value) {
				if (substr($name, 0, 2) == "E_")
					$errcodes[$value] = $name;
			}
		}
		printf("[%s] %s in %s on line %s\n",
			(isset($errcodes[$errno])) ? $errcodes[$errno] : $errno,
			 $error, $file, $line);

		return true;
	}

	set_error_handler('grumble_catchable_fatal_grumble');
?>