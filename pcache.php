<?php

for ($i = 0; $i < 100; $i++) {
    pcache_set('key' . $i, "$i");
}

for ($i = 0; $i < 100; $i++) {
    var_dump(pcache_get('key' . $i));
    var_dump(pcache_del('key' . $i));
    var_dump(pcache_get('key' . $i));
}

var_dump(pcache_keys('key*'));