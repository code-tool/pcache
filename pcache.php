<?php
$length = 100000;
$keyPrefix = 'keyveryveryveryveryveryveryveryveryveryveryveryveryverylong';
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
