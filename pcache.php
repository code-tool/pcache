<?php
$length = 500;

for ($i = 0; $i < $length; $i++) {

    pcache_set('key' . $i, "$i");

    var_dump(pcache_get('key' . $i));
    var_dump(pcache_del('key' . $i));
    var_dump(pcache_get('key' . $i));
}

var_dump(pcache_keys('key*'));