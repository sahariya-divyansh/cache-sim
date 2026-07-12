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
    unsigned long tag;
    unsigned long last_used;       // Counter/timestamp for LRU tracking
    unsigned long insertion_time;  // Counter/timestamp for FIFO tracking
} CacheLine;

/**
 * @brief Represents a single cache set containing multiple cache lines.
 */
typedef struct CacheSet {
    CacheLine *lines;
} CacheSet;

/**
 * @brief Represents the top-level Cache configuration and hierarchy state.
 */
typedef struct Cache {
    CacheSet *sets;
    int cache_size;
    int block_size;
    int associativity;
    int num_sets;
    int offset_bits;
    int index_bits;
    int tag_bits;
    ReplacementPolicy policy;
    unsigned long access_counter;  // Global access counter for LRU/FIFO ordering
    unsigned int rng_state;        // Current state for thread-safe pseudo-random generator
} Cache;

/**
 * @brief Initializes the cache simulator data structures.
 * 
 * @param cache_size Total cache size in bytes (must be power of 2).
 * @param block_size Cache block size in bytes (must be power of 2).
 * @param associativity Level of associativity (1 for direct, power of 2, etc.).
 * @return Cache* Pointer to the allocated Cache structure, or NULL on error.
 */
Cache* cache_init(int cache_size, int block_size, int associativity);

/**
 * @brief Safely deallocates all allocated memory for the cache.
 * 
 * @param c Pointer to the Cache structure to destroy.
 */
void cache_destroy(Cache *c);

/**
 * @brief Extracts the tag, set index, and offset fields from a memory address.
 * 
 * @param c Pointer to the Cache simulator parameters.
 * @param address The incoming memory access address.
 * @param tag Output pointer for the extracted tag bits.
 * @param index Output pointer for the extracted index bits.
 * @param offset Output pointer for the extracted block offset bits.
 */
void extract_address_fields(Cache *c, unsigned long address, unsigned long *tag, unsigned long *index, unsigned long *offset);

/**
 * @brief Simulates a memory access to the cache.
 * 
 * @param c Pointer to the Cache structure.
 * @param address Memory address being accessed.
 * @return 1 if access resulted in a hit, 0 if it was a miss.
 */
int cache_access(Cache *c, unsigned long address);

#endif // CACHE_H
