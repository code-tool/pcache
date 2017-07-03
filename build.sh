#!/usr/bin/env bash

/usr/bin/phpize7.0
CFLAGS="-g -O0" ./configure --with-php-config=/usr/bin/php-config7.0
/usr/bin/make clean
TEST_PHP_ARGS="-q" /usr/bin/make test
/usr/bin/make