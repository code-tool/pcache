--TEST--
Compare with apcu
--SKIPIF--
<?php if (!extension_loaded("pcache") || !extension_loaded("apcu") ) print "skip"; ?>
--FILE--
<?php 
$length = 100000;
$keyPrefix = 'keyasdasdasdksdfdsfswifhjuhasddxsfdasg';
$timePcache = 0;
$timeApcu = 0;

$start = microtime(true);
for ($i = 0; $i < $length; $i++) {
    pcache_set($keyPrefix. $i, "$i", $i);
}
$end = microtime(true);
$timePcache = $end - $start;

$start = microtime(true);
for ($i = 0; $i < $length; $i++) {
    apcu_add($keyPrefix. $i, "$i", $i);
}
$end = microtime(true);
$timeApcu = $end - $start;

echo sprintf("Pcache:%f, APCu:%f", $timePcache, $timeApcu);
?>
--EXPECTF--
