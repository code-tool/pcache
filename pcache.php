<?php
$length = 100;
$keyPrefix = 'key';

for ($i = 0; $i < $length; $i++) {
    $key = $keyPrefix . $i;

    if (false === pcache_set($key, $i, mt_rand(1,10))) {
        throw new \RuntimeException(var_export([$i,pcache_info()], true));
    }
    var_dump(pcache_get($key));
}

sleep(10);
pcache_set('test', 'test');

var_dump(pcache_info());