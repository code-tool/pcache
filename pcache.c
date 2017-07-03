/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2014 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:   Liexusong (c) Liexusong@qq.com                             |
  +----------------------------------------------------------------------+
*/

/* $Id: header,v 1.16.2.1.2.1.2.1 2008/02/07 19:39:50 iliaa Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_pcache.h"
#include "ncx_slab.h"
#include "ncx_shm.h"
#include "util.h"
#include "trie.h"
#include "storage.h"
#include "list.h"

#if PHP_VERSION_ID >= 70000

#define NEW_VALUE_LEN ZSTR_LEN(new_value)
#define NEW_VALUE ZSTR_VAL(new_value)
#define _RETURN_STRINGL(k, l) RETURN_STRINGL(k,l)

#else

#define NEW_VALUE_LEN new_value_length
#define NEW_VALUE new_value
#define _RETURN_STRINGL(k,l) RETURN_STRINGL(k,l,0)

#endif

typedef struct pcache_cache_item pcache_cache_item;
struct pcache_cache_item {
    struct list_head hash;
    long expire;
    size_t data_len;
    void *data_ptr;
};

/* True global resources - no need for thread safety here */
static trie *cache_trie;
static ncx_atomic_t *cache_lock;
static struct list_head *cache_expire;
static ncx_uint_t cache_size = 10485760; /* 10MB */
static int cache_enable = 1;

int pcache_ncpu;

/* {{{ pcache_functions[]
 *
 * Every user visible function must have an entry in pcache_functions[].
 */
const zend_function_entry pcache_functions[] = {
        PHP_FE(pcache_set, NULL)
        PHP_FE(pcache_get, NULL)
        PHP_FE(pcache_del, NULL)
        PHP_FE(pcache_search, NULL)
        PHP_FE(pcache_info, NULL)
        {NULL, NULL, NULL}    /* Must be the last line in pcache_functions[] */
};
/* }}} */

/* {{{ pcache_module_entry
 */
zend_module_entry pcache_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
        STANDARD_MODULE_HEADER,
#endif
        "pcache",
        pcache_functions,
        PHP_MINIT(pcache),
        PHP_MSHUTDOWN(pcache),
        PHP_RINIT(pcache),
        PHP_RSHUTDOWN(pcache),
        PHP_MINFO(pcache),
#if ZEND_MODULE_API_NO >= 20010901
        "0.3", /* Replace with version number for your extension */
#endif
        STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_PCACHE
ZEND_GET_MODULE(pcache)
#endif

ZEND_INI_MH(pcache_set_enable) {
    if (NEW_VALUE_LEN == 0) {
        return FAILURE;
    }

    if (!strcasecmp(NEW_VALUE, "on") || !strcmp(NEW_VALUE, "1")) {
        cache_enable = 1;
    } else {
        cache_enable = 0;
    }

    return SUCCESS;
}


ZEND_INI_MH(pcache_set_cache_size) {
    if (NEW_VALUE_LEN == 0) {
        return FAILURE;
    }

    int len;

    pcache_atoi((const char *) NEW_VALUE, (int *) &cache_size, &len);

    if (len > 0 && len < NEW_VALUE_LEN) { /* have unit */
        switch (NEW_VALUE[len]) {
            case 'k':
            case 'K':
                cache_size *= 1024;
                break;
            case 'm':
            case 'M':
                cache_size *= 1024 * 1024;
                break;
            case 'g':
            case 'G':
                cache_size *= 1024 * 1024 * 1024;
                break;
            default:
                return FAILURE;
        }

    } else if (len == 0) {
        return FAILURE;
    }

    return SUCCESS;
}

PHP_INI_BEGIN()
                PHP_INI_ENTRY("pcache.cache_size", "10485760", PHP_INI_SYSTEM,
                              pcache_set_cache_size)
                PHP_INI_ENTRY("pcache.enable", "1", PHP_INI_SYSTEM,
                              pcache_set_enable)
PHP_INI_END()


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION (pcache) {

    REGISTER_INI_ENTRIES();

    if (!cache_enable) {
        return SUCCESS;
    }

    if (!storage_init(cache_size)) {
        return FAILURE;
    }

    cache_trie = trie_create();
    if (!cache_trie) {
        ncx_shm_free(&cache_shm);
        return FAILURE;
    }

    cache_lock = storage_malloc(sizeof(ncx_atomic_t));
    if (!cache_lock) {
        ncx_shm_free(&cache_shm);
        return FAILURE;
    }

    cache_expire = storage_malloc(sizeof(struct list_head));
    if (!cache_expire) {
        ncx_shm_free(&cache_shm);
        return FAILURE;
    }
    INIT_LIST_HEAD(cache_expire);

    pcache_ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    if (pcache_ncpu <= 0) {
        pcache_ncpu = 1;
    }

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION (pcache) {
    UNREGISTER_INI_ENTRIES();

    ncx_shm_free(&cache_shm);

    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION (pcache) {
    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION (pcache) {
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION (pcache) {
    php_info_print_table_start();
    php_info_print_table_header(2, "pcache support", "enabled");
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
}

/* }}} */

void pcache_try_run_gc() {
    pcache_cache_item *item;
    struct list_head *curr;
    long now = (long) time(NULL);

    list_for_each(curr, cache_expire) {
        item = list_entry(curr, pcache_cache_item, hash);

        if (item->expire > 0 && item->expire <= now) {
            list_del(&item->hash);
            storage_free(item->data_ptr, item->data_len);
            storage_free(item, sizeof(struct pcache_cache_item));
        }
    }
}

PHP_FUNCTION (pcache_set) {
    if (!cache_enable) {
        RETURN_FALSE;
    }

    char *key = NULL, *val = NULL, *shared_val = NULL;
    size_t val_len = 0, key_len = 0, item_len = 0;
    long expire = 0;
    pcache_cache_item *item = NULL;

#if PHP_VERSION_ID >= 70000

    zend_string *pkey, *pval;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "SS|l", &pkey, &pval, &expire) == FAILURE) {
        RETURN_FALSE;
    }

    key = ZSTR_VAL(pkey);
    key_len = ZSTR_LEN(pkey);

    val = ZSTR_VAL(pval);
    val_len = ZSTR_LEN(pval);

#else

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|l", &key, &key_len, &data_ptr, &val_len, &expire) == FAILURE)
    {
        RETURN_FALSE;
    }

#endif

    if (expire > 0) {
        expire += (long) time(NULL); /* update expire time */
    }

    ncx_shmtx_lock(cache_lock);

    //pcache_try_run_gc();

    shared_val = storage_malloc(val_len);
    if (!shared_val) {
        ncx_shmtx_unlock(cache_lock);

        RETURN_FALSE;
    }
    memcpy(shared_val, val, val_len);

    item = storage_malloc(sizeof(pcache_cache_item));
    if (!item) {
        storage_free(shared_val, val_len);
        ncx_shmtx_unlock(cache_lock);

        RETURN_FALSE;
    }
    item->expire = expire;
    item->data_len = val_len;
    item->data_ptr = shared_val;

    bool r_val = 0 == trie_insert(cache_trie, key, item);
    if (r_val == false) {
        storage_free(item->data_ptr, item->data_len);
        storage_free(item, sizeof(struct pcache_cache_item));

        ncx_shmtx_unlock(cache_lock);

        RETURN_FALSE;
    }

//    if (expire > 0) {
//        list_add(&item->hash, cache_expire);
//    }

    ncx_shmtx_unlock(cache_lock);

    RETURN_TRUE
}

PHP_FUNCTION (pcache_get) {
    if (!cache_enable) {
        RETURN_FALSE;
    }

    char *key = NULL;
    size_t key_len = 0, retlen = 0;
    char *retval = NULL;
    pcache_cache_item *item = NULL;
    long now = (long) time(NULL);

#if PHP_VERSION_ID >= 70000

    zend_string *pkey;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S", &pkey) == FAILURE) {
        RETURN_FALSE;
    }
    key = ZSTR_VAL(pkey);
    key_len = ZSTR_LEN(pkey);

#else

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }

#endif

    ncx_shmtx_lock(cache_lock);

    item = trie_search(cache_trie, key);
    if (!item) {
        ncx_shmtx_unlock(cache_lock);

        RETURN_NULL()
    }

    /* expire */
    if (item->expire > 0 && item->expire <= now) {
        storage_free(item->data_ptr, item->data_len);
        storage_free(item, sizeof(struct pcache_cache_item));
        trie_insert(cache_trie, key, NULL);
        ncx_shmtx_unlock(cache_lock);

        RETURN_NULL()
    }

    retval = emalloc(item->data_len);
    memcpy(retval, item->data_ptr, item->data_len);

    ncx_shmtx_unlock(cache_lock);

    _RETURN_STRINGL(retval, item->data_len);
}

PHP_FUNCTION (pcache_del) {
    if (!cache_enable) {
        RETURN_FALSE;
    }

    char *key = NULL;
    size_t key_len = 0;
    pcache_cache_item *item = NULL;

#if PHP_VERSION_ID >= 70000

    zend_string *pkey;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S", &pkey) == FAILURE) {
        RETURN_FALSE;
    }
    key = ZSTR_VAL(pkey);
    key_len = ZSTR_LEN(pkey);

#else

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }

#endif

    ncx_shmtx_lock(cache_lock);

    item = trie_search(cache_trie, key);
    if (!item) {
        ncx_shmtx_unlock(cache_lock);

        RETURN_FALSE;
    }

    storage_free(item->data_ptr, item->data_len);
    storage_free(item, sizeof(struct pcache_cache_item));

    bool r_val = 0 == trie_insert(cache_trie, key, NULL);

    ncx_shmtx_unlock(cache_lock);

    RETURN_BOOL(r_val);
}

int visitor_search(const char *key, void *data, void *arg) {
    size_t retlen = 0;
    char *retval = NULL;
    pcache_cache_item *item = data;
    long now = (long) time(NULL);

    /* expire */
    if (item->expire > 0 && item->expire <= now) {
        storage_free(item->data_ptr, item->data_len);
        storage_free(item, sizeof(struct pcache_cache_item));
        trie_insert(cache_trie, key, NULL);

        return 0;
    }

    retlen = item->data_len;
    retval = emalloc(item->data_len);
    memcpy(retval, item->data_ptr, item->data_len);

    add_assoc_stringl(arg, key, retval, retlen);

    return 0;
}

PHP_FUNCTION (pcache_search) {
    if (!cache_enable) {
        RETURN_FALSE;
    }
    char *key_prefix = NULL;
    size_t key_prefix_len = 0;

#if PHP_VERSION_ID >= 70000

    zend_string *pkey_prefix;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S", &pkey_prefix) == FAILURE) {
        RETURN_FALSE;
    }
    key_prefix = ZSTR_VAL(pkey_prefix);
    key_prefix_len = ZSTR_LEN(pkey_prefix);

#else

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key_prefix, &key_prefix_len) == FAILURE)
    {
        RETURN_FALSE;
    }

#endif

    array_init(return_value);

    ncx_shmtx_lock(cache_lock);

    trie_visit(cache_trie, key_prefix, visitor_search, return_value);

    ncx_shmtx_unlock(cache_lock);
}

PHP_FUNCTION (pcache_info) {
    if (!cache_enable) {
        RETURN_FALSE;
    }

    array_init(return_value);

    add_assoc_long(return_value, "mem_used", (zend_long) cache_pool->total_size);
    add_assoc_long(return_value, "trie_mem_used", (zend_long) trie_size(cache_trie));
    add_assoc_long(return_value, "trie_count", (zend_long) trie_count(cache_trie, ""));
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
