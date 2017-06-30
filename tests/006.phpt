--TEST--
Check for info
--SKIPIF--
<?php if (!extension_loaded("pcache")) print "skip"; ?>
--FILE--
<?php 
$key = "foo";
$value = "dummy";

var_dump(pcache_set($key, $value));

var_dump(pcache_info());
?>
--EXPECTF--
bool(true)
array(1) {
  ["used"]=>
  int(4218)
}