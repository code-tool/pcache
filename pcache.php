<?php
$length = 500;
$keyPrefix = 'key';

for ($i = 0; $i < $length; $i++) {

    $key = $keyPrefix. $i;

    pcache_set($key, "$i");

    var_dump(pcache_get($key));
    //var_dump(pcache_del($key));
    //var_dump(pcache_get($key));
}

var_dump(pcache_keys('key5'));