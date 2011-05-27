<?php
if (!extension_loaded("mysqlnd_ms"))
  die("SKIP ext/mysqlnd_ms not available");

if (!extension_loaded("pdo_mysql"))
  die("SKIP ext/pdo_mysql not available");
?>