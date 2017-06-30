#ifndef PCACHE_STORAGE_H
#define PCACHE_STORAGE_H

#include <stddef.h>
#include "ncx_slab.h"
#include "ncx_shm.h"

ncx_slab_pool_t *cache_pool;
ncx_shm_t cache_shm;

extern bool storage_init(ncx_uint_t cache_size);

extern void *storage_malloc(size_t size);

extern void *storage_realloc(void *ptr, size_t new_size);

extern void storage_free(void *ptr, size_t size);

#endif