--TEST--
Check for prefix search with ttl
--SKIPIF--
<?php if (!extension_loaded("pcache")) print "skip"; ?>
--FILE--
<?php 
$length = 10;
$keyPrefix = 'key';
$ttl = 1;

for ($i = 0; $i < $length; $i++) {
    $key = $keyPrefix. $i;
    pcache_set($key, "$i", $ttl);
}

sleep(1);

var_dump(pcache_search('key'));
?>
--EXPECTF--
array(0) {
}