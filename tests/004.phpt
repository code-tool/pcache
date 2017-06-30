--TEST--
Check for prefix search
--SKIPIF--
<?php if (!extension_loaded("pcache")) print "skip"; ?>
--FILE--
<?php 
$length = 10;
$keyPrefix = 'key';

for ($i = 0; $i < $length; $i++) {
    $key = $keyPrefix. $i;
    pcache_set($key, "$i");
}

var_dump(pcache_search('key'));
?>
--EXPECTF--
array(10) {
  ["key0"]=>
  string(1) "0"
  ["key1"]=>
  string(1) "1"
  ["key2"]=>
  string(1) "2"
  ["key3"]=>
  string(1) "3"
  ["key4"]=>
  string(1) "4"
  ["key5"]=>
  string(1) "5"
  ["key6"]=>
  string(1) "6"
  ["key7"]=>
  string(1) "7"
  ["key8"]=>
  string(1) "8"
  ["key9"]=>
  string(1) "9"
}