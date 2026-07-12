#include "cache.h"
#include <stdlib.h>

Cache* cache_create(size_t cache_size, size_t block_size, size_t associativity, ReplacementPolicy policy) {
    // Stub implementation to be filled in later
    (void)cache_size;
    (void)block_size;
    (void)associativity;
    (void)policy;
    return NULL;
}

void cache_destroy(Cache *cache) {
    // Stub implementation to be filled in later
    (void)cache;
}

bool cache_access(Cache *cache, uint64_t address, char mode) {
    // Stub implementation to be filled in later
    (void)cache;
    (void)address;
    (void)mode;
    return false;
}
