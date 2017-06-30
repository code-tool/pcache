--TEST--
Check for basic functions
--SKIPIF--
<?php if (!extension_loaded("pcache")) print "skip"; ?>
--FILE--
<?php 
$key = "foo";
$value = "dummy";

var_dump(pcache_set($key, $value));
var_dump(pcache_get($key));

$value = NULL;
var_dump(pcache_set($key, $value));
var_dump(pcache_get($key));

$value = TRUE;
var_dump(pcache_set($key, $value));
var_dump(pcache_get($key));

$value = FALSE;
var_dump(pcache_set($key, $value));
var_dump(pcache_get($key));

var_dump(pcache_del($key));
var_dump(pcache_get($key));

?>
--EXPECTF--
bool(true)
string(5) "dummy"
bool(true)
string(0) ""
bool(true)
string(1) "1"
bool(true)
string(0) ""
bool(true)
NULL