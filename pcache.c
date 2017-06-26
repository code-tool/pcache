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
#include "trie_storage.h"

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
    long expire;
    char *val;
};

struct pcache_status {
    long trie_size;
};

/* True global resources - no need for thread safety here */
static trie *cache_trie;
static ncx_atomic_t *cache_lock;
static struct pcache_status *cache_status;
/* configure entries */
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
    int len;

    if (NEW_VALUE_LEN == 0) {
        return FAILURE;
    }

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

    /* alloc cache lock */
    cache_lock = ncx_slab_alloc_locked(cache_pool, sizeof(ncx_atomic_t));
    if (!cache_lock) {
        ncx_shm_free(&cache_shm);
        return FAILURE;
    }

    /* alloc cache status struct */
    cache_status = ncx_slab_alloc_locked(cache_pool, sizeof(struct pcache_status));
    if (!cache_status) {
        ncx_shm_free(&cache_shm);
        return FAILURE;
    }

    cache_status->trie_size  = trie_size(cache_trie);

    /* get cpu's core number */
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

PHP_FUNCTION (pcache_set) {
    if (!cache_enable) {
        RETURN_FALSE;
    }

    char *key = NULL, *val = NULL, *shared_val = NULL;
    size_t val_len, item_len;
    long expire = 0;
    pcache_cache_item *shared_item;

    zend_string *pkey, *pval;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "SS|l", &pkey, &pval, &expire) == FAILURE) {
        RETURN_FALSE;
    }

    key = ZSTR_VAL(pkey);
    val = ZSTR_VAL(pval);
    val_len = ZSTR_LEN(pval);

    if (expire > 0) {
        expire += (long) time(NULL); /* update expire time */
    }

    ncx_shmtx_lock(cache_lock);

    shared_val = storage_malloc(val_len + 1);
    if (!shared_val) {
        ncx_shmtx_unlock(cache_lock);

        RETURN_FALSE;
    }
    memcpy(shared_val, val, val_len);
    shared_val[val_len] = '\0';

    item_len = sizeof(pcache_cache_item);
    shared_item = storage_malloc(item_len);
    if (!shared_item) {
        ncx_shmtx_unlock(cache_lock);

        RETURN_FALSE;
    }
    memcpy(shared_item, shared_item, item_len);
    shared_item->expire = expire;
    shared_item->val = shared_val;

    bool r_val = 0 == trie_insert(cache_trie, key, shared_item);

    ncx_shmtx_unlock(cache_lock);

    RETURN_BOOL(r_val)
}

PHP_FUNCTION (pcache_get) {
    if (!cache_enable) {
        RETURN_FALSE;
    }

    char *key = NULL;
    size_t retlen = 0;
    char *retval = NULL;
    zend_string *pkey;
    pcache_cache_item *item;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S", &pkey) == FAILURE) {
        RETURN_FALSE;
    }
    key = ZSTR_VAL(pkey);

    ncx_shmtx_lock(cache_lock);

    item = trie_search(cache_trie, key);
    if (!item) {
        ncx_shmtx_unlock(cache_lock);

        RETURN_NULL()
    }

    retlen = strlen(item->val);
    retval = emalloc(retlen + 1);
    memcpy(retval, item->val, retlen);
    retval[retlen] = '\0';

    ncx_shmtx_unlock(cache_lock);

    _RETURN_STRINGL(retval, retlen);
}

PHP_FUNCTION (pcache_del) {
    if (!cache_enable) {
        RETURN_FALSE;
    }

    char *key = NULL;
    zend_string *pkey;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S", &pkey) == FAILURE) {
        RETURN_FALSE;
    }
    key = ZSTR_VAL(pkey);

    ncx_shmtx_lock(cache_lock);

    bool r_val = 0 == trie_remove(cache_trie, key);

    ncx_shmtx_unlock(cache_lock);

    RETURN_BOOL(r_val)
}

int visitor_search(const char *key, void *data, void *arg) {
    size_t retlen = 0;
    char *retval = NULL;
    pcache_cache_item *item = data;

    retlen = strlen(item->val);
    retval = emalloc(retlen + 1);
    memcpy(retval, item->val, retlen);
    retval[retlen] = '\0';

    add_assoc_stringl(arg, key, retval, retlen);

    return 0;
}

PHP_FUNCTION (pcache_search) {
    if (!cache_enable) {
        RETURN_FALSE;
    }
    char *key_pattern = NULL;
    zend_string *pkey_pattern;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S", &pkey_pattern) == FAILURE) {
        RETURN_FALSE;
    }
    key_pattern = ZSTR_VAL(pkey_pattern);

    array_init(return_value);

    ncx_shmtx_lock(cache_lock);

    trie_visit(cache_trie, key_pattern, visitor_search, return_value);

    ncx_shmtx_unlock(cache_lock);
}

PHP_FUNCTION (pcache_info) {
    if (!cache_enable) {
        RETURN_FALSE;
    }

    array_init(return_value);

    add_assoc_long(return_value, "trie_size", trie_size(cache_trie));
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
