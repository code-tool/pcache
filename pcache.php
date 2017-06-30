<?php
$length = 10;
$keyPrefix = 'key';


//var_dump(pcache_info());

for ($i = 0; $i < $length; $i++) {

    $key = $keyPrefix . $i;

    if (false === pcache_set($key, $i, $i)) {
        throw new \RuntimeException(var_export(pcache_info(), true));
    }
    var_dump(pcache_get($key));
    //pcache_del($key);
}

sleep(9);

var_dump(pcache_search('key'));

var_dump(pcache_info());
