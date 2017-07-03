<?php
$length = 10;
$keyPrefix = 'key';

for ($i = 0; $i < $length; $i++) {
    $key = $keyPrefix . $i;

    if (false === pcache_set($key, $i, $i)) {
        throw new \RuntimeException(var_export([$i,pcache_info()], true));
    }
    var_dump(pcache_get($key));
    pcache_del($key);
}
var_dump(pcache_info());