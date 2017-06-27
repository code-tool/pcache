<?php
$length = 100000;
$keyPrefix = 'key';

for ($i = 0; $i < $length; $i++) {

    $key = $keyPrefix. $i;

    pcache_set($key, "$i");

    var_dump(pcache_get($key));
    var_dump(pcache_del($key));
    //var_dump(pcache_get($key));
}

sleep(1);

var_dump(pcache_search('key5'));

var_dump(pcache_info());