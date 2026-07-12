#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Enum representing cache replacement policies.
 */
typedef enum {
    POLICY_LRU,
    POLICY_FIFO,
    POLICY_RANDOM
} ReplacementPolicy;

/**
 * @brief Represents a single cache line.
 */
typedef struct CacheLine {
    bool valid;
    uint64_t tag;
    uint64_t last_accessed; // For LRU tracking
    uint64_t load_time;     // For FIFO tracking
} CacheLine;

/**
 * @brief Represents a single cache set, which contains multiple lines.
 */
typedef struct CacheSet {
    CacheLine *lines;
} CacheSet;

/**
 * @brief Represents the top-level cache simulator system structure.
 */
typedef struct Cache {
    CacheSet *sets;
    size_t num_sets;
    size_t block_size;
    size_t associativity;
    ReplacementPolicy policy;
} Cache;

/**
 * @brief Creates and initializes a new cache structure.
 * 
 * @param cache_size Total size of the cache in bytes.
 * @param block_size Block size in bytes.
 * @param associativity Associativity level (1 for direct-mapped, N for N-way, etc.).
 * @param policy Cache replacement policy.
 * @return Cache* Pointer to the allocated Cache structure, or NULL on failure.
 */
Cache* cache_create(size_t cache_size, size_t block_size, size_t associativity, ReplacementPolicy policy);

/**
 * @brief Safely frees all memory allocated for the cache simulator structure.
 * 
 * @param cache Pointer to the Cache structure to be freed.
 */
void cache_destroy(Cache *cache);

/**
 * @brief Simulates a memory access to the cache.
 * 
 * @param cache Pointer to the Cache structure.
 * @param address Memory address being accessed.
 * @param mode Access mode ('l' for load/read, 's' for store/write).
 * @return true if access resulted in a hit, false if it was a miss.
 */
bool cache_access(Cache *cache, uint64_t address, char mode);

#endif // CACHE_H
