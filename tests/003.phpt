--TEST--
Check for ttl
--SKIPIF--
<?php if (!extension_loaded("pcache")) print "skip"; ?>
--FILE--
<?php 
$key = "foo";
$value = "dummy";

var_dump(pcache_set($key, $value, 1));
var_dump(pcache_get($key));
sleep(1);
var_dump(pcache_get($key));
?>
--EXPECTF--
bool(true)
string(5) "dummy"
NULL