<?php

for ($i = 0; $i < 100; $i++) {
    pcache_set2('key' . $i, "$i");

    var_dump(pcache_get2('key' . $i));
}
