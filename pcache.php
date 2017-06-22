<?php

$key = 'test';
$value = 'val2';
for ($i = 0; $i < 100; $i++) {
    pcache_set2('key' . $i, "$i");
}

var_dump(pcache_keys('key859*'));
/*
$value = pcache_get($key);
var_dump(pcache_keys('key*'));
var_dump($value, pcache_del($key));
var_dump(pcache_get($key));
sleep(100);*/