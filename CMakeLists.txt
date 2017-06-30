cmake_minimum_required(VERSION 3.8)
project(pcache)

set(CMAKE_INCLUDE_PATH /usr/include)
set(CMAKE_CXX_STANDARD 11)
include_directories(/usr/include/php/20151012)
include_directories(/usr/include/php/20151012/Zend)
include_directories(/usr/include/php/20151012/TSRM)
include_directories(/usr/include/php/20151012/main)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

set(SOURCE_FILES
        list.h
        ncx_core.h
        ncx_lock.c
        ncx_lock.h
        ncx_shm.c
        ncx_shm.h
        ncx_slab.c
        ncx_slab.h
        pcache.c
        php_pcache.h
        util.c
        util.h
        trie.c
        trie.h
        storage.c
        storage.h)

add_library(pcache SHARED ${SOURCE_FILES})