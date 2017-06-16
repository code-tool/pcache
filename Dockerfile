FROM php:7.0-apache

RUN bash -l -c 'echo export EXTENSION_DIR="$(php-config --extension-dir)"'
RUN echo 'extension=pcache.so' > ${PHP_INI_DIR=}/conf.d/pcache.ini
COPY modules/pcache.so $EXTENSION_DIR/
COPY modules/pcache.so /usr/local/lib/php/extensions/no-debug-non-zts-20151012/
COPY pcache.php /var/www/html/index.php
