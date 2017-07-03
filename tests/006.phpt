--TEST--
Check for info
--SKIPIF--
<?php if (!extension_loaded("pcache")) print "skip"; ?>
--FILE--
<?php 
$key = "foo";
$value = "dummy";

var_dump(pcache_set($key, $value));
$cacheInfo = pcache_info();
var_dump(array_key_exists('mem_used',$cacheInfo));
var_dump(array_key_exists('trie_mem_used',$cacheInfo));
var_dump(array_key_exists('trie_count',$cacheInfo));
?>
--EXPECTF--
bool(true)
bool(true)
bool(true)
bool(true)