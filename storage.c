#include "storage.h"

extern bool storage_init(ncx_uint_t cache_size) {
    void *space;

    cache_shm.size = cache_size;

    /* alloc share memory */
    if (ncx_shm_alloc(&cache_shm) == -1) {
        return false;
    }

    space = (void *) cache_shm.addr;

    cache_pool = (ncx_slab_pool_t *) space;
    cache_pool->addr = space;
    cache_pool->min_shift = 3;
    cache_pool->end = space + cache_size;
    cache_pool->total_size = 0;

    /* init slab */
    ncx_slab_init(cache_pool);

    return true;
}

extern void *storage_malloc(size_t size) {
    cache_pool->total_size += size;

    return ncx_slab_alloc_locked(cache_pool, size);
}

extern void *storage_realloc(void *ptr, size_t new_size) {
    size_t old_size = ncx_slab_size(cache_pool, ptr);
    cache_pool->total_size -= old_size;
    cache_pool->total_size += new_size;

    return ncx_slab_realloc_locked(cache_pool, ptr, old_size, new_size);
}

extern void storage_free(void *ptr) {
    size_t size = ncx_slab_size(cache_pool, ptr);
    cache_pool->total_size -= size;
    ncx_slab_free_locked(cache_pool, ptr);
}