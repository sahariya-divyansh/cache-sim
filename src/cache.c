#include "cache.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Helper function to verify if an integer is a positive power of 2.
 */
static int is_power_of_two(int val) {
    return (val > 0) && ((val & (val - 1)) == 0);
}

/**
 * @brief Helper function to compute log2 of an integer (assumes positive input).
 */
static int log2_int(int val) {
    int bits = 0;
    while (val > 1) {
        val >>= 1;
        bits++;
    }
    return bits;
}

/**
 * @brief A thread-safe, cross-platform deterministic Linear Congruential Generator (LCG).
 * 
 * Uses standard POSIX LCG multiplier and increment constants.
 * 
 * @param state Pointer to the unsigned int state to be updated.
 * @return The updated pseudorandom number.
 */
static unsigned int lcg_rand(unsigned int *state) {
    *state = (*state * 1103515245U + 12345U) & 0x7fffffffU;
    return *state;
}

/**
 * @brief Scans a cache set to find the Least Recently Used (LRU) victim line.
 * 
 * Runs in O(associativity) time complexity.
 * 
 * @param c Pointer to the Cache parameters.
 * @param set_index Index of the set to scan.
 * @return The index of the victim line.
 */
static int find_victim_lru(Cache *c, int set_index) {
    CacheSet *set = &c->sets[set_index];
    int victim = 0;
    unsigned long min_access = set->lines[0].last_used;

    for (int i = 1; i < c->associativity; i++) {
        if (set->lines[i].last_used < min_access) {
            min_access = set->lines[i].last_used;
            victim = i;
        }
    }
    return victim;
}

/**
 * @brief Scans a cache set to find the First-In, First-Out (FIFO) victim line.
 * 
 * Runs in O(associativity) time complexity.
 * 
 * @param c Pointer to the Cache parameters.
 * @param set_index Index of the set to scan.
 * @return The index of the victim line.
 */
static int find_victim_fifo(Cache *c, int set_index) {
    CacheSet *set = &c->sets[set_index];
    int victim = 0;
    unsigned long min_insertion = set->lines[0].insertion_time;

    for (int i = 1; i < c->associativity; i++) {
        if (set->lines[i].insertion_time < min_insertion) {
            min_insertion = set->lines[i].insertion_time;
            victim = i;
        }
    }
    return victim;
}

/**
 * @brief Selects a random cache line index as the eviction victim.
 * 
 * Runs in O(1) time complexity.
 * 
 * @param c Pointer to the Cache parameters.
 * @return The index of the victim line.
 */
static int find_victim_random(Cache *c) {
    return (int)(lcg_rand(&c->rng_state) % (unsigned int)c->associativity);
}

Cache* cache_init(int cache_size, int block_size, int associativity) {
    // 1. Defensive parameter validation
    if (cache_size <= 0 || block_size <= 0 || associativity <= 0) {
        fprintf(stderr, "Error: Cache parameters must be positive integers.\n");
        return NULL;
    }

    if (!is_power_of_two(cache_size)) {
        fprintf(stderr, "Error: Cache size (%d) must be a power of 2.\n", cache_size);
        return NULL;
    }

    if (!is_power_of_two(block_size)) {
        fprintf(stderr, "Error: Block size (%d) must be a power of 2.\n", block_size);
        return NULL;
    }

    if (!is_power_of_two(associativity)) {
        fprintf(stderr, "Error: Associativity (%d) must be a power of 2.\n", associativity);
        return NULL;
    }

    // Ensure cache capacity can accommodate at least one block per set
    long long total_block_capacity = (long long)block_size * associativity;
    if (cache_size < total_block_capacity) {
        fprintf(stderr, "Error: Cache size (%d) is too small to accommodate block size (%d) * associativity (%d).\n",
                cache_size, block_size, associativity);
        return NULL;
    }

    // 2. Compute address bit widths
    int num_sets = cache_size / (block_size * associativity);
    int offset_bits = log2_int(block_size);
    int index_bits = log2_int(num_sets);
    int tag_bits = (int)(sizeof(unsigned long) * 8) - index_bits - offset_bits;

    // 3. Dynamic allocation with robust NULL checking and zero-initialization
    Cache *c = (Cache *)calloc(1, sizeof(Cache));
    if (!c) {
        fprintf(stderr, "Error: Out of memory allocating Cache descriptor.\n");
        return NULL;
    }

    c->cache_size = cache_size;
    c->block_size = block_size;
    c->associativity = associativity;
    c->num_sets = num_sets;
    c->offset_bits = offset_bits;
    c->index_bits = index_bits;
    c->tag_bits = tag_bits;
    c->policy = POLICY_LRU; // Default policy
    c->access_counter = 0;
    c->rng_state = 42;      // Default seed state

    c->sets = (CacheSet *)calloc((size_t)num_sets, sizeof(CacheSet));
    if (!c->sets) {
        fprintf(stderr, "Error: Out of memory allocating Cache sets.\n");
        free(c);
        return NULL;
    }

    // Allocate cache lines per set, implementing clean rollback on failure to prevent memory leaks
    for (int i = 0; i < num_sets; i++) {
        c->sets[i].lines = (CacheLine *)calloc((size_t)associativity, sizeof(CacheLine));
        if (!c->sets[i].lines) {
            fprintf(stderr, "Error: Out of memory allocating Cache lines for set %d.\n", i);
            // Rollback previously allocated lines
            for (int j = 0; j < i; j++) {
                free(c->sets[j].lines);
            }
            free(c->sets);
            free(c);
            return NULL;
        }
    }

    return c;
}

void extract_address_fields(Cache *c, unsigned long address, unsigned long *tag, unsigned long *index, unsigned long *offset) {
    if (!c) {
        return;
    }

    int word_bits = (int)(sizeof(unsigned long) * 8);

    /*
     * Memory Address Decomposition:
     * +---------------------------+-----------------------+----------------------+
     * |       Tag (tag_bits)      |   Index (index_bits)  |  Offset (offset_bits)|
     * +---------------------------+-----------------------+----------------------+
     */

    // 1. Extract Offset (lower 'offset_bits' bits)
    if (offset) {
        if (c->offset_bits >= word_bits) {
            *offset = address;
        } else {
            unsigned long offset_mask = (1UL << c->offset_bits) - 1;
            *offset = address & offset_mask;
        }
    }

    // 2. Extract Set Index (next 'index_bits' bits, shifted right by 'offset_bits')
    if (index) {
        if (c->index_bits >= word_bits || c->offset_bits >= word_bits) {
            *index = 0;
        } else {
            unsigned long index_mask = (1UL << c->index_bits) - 1;
            *index = (address >> c->offset_bits) & index_mask;
        }
    }

    // 3. Extract Tag (remaining upper bits, shifted right by 'offset_bits' + 'index_bits')
    if (tag) {
        int shift = c->offset_bits + c->index_bits;
        if (shift >= word_bits) {
            *tag = 0;
        } else {
            *tag = address >> shift;
        }
    }
}

void cache_destroy(Cache *c) {
    if (!c) {
        return;
    }

    // Safely free individual cache line arrays per set
    if (c->sets) {
        for (int i = 0; i < c->num_sets; i++) {
            if (c->sets[i].lines) {
                free(c->sets[i].lines);
            }
        }
        free(c->sets);
    }

    // Free the top level cache descriptor
    free(c);
}

int cache_access(Cache *c, unsigned long address) {
    if (!c) {
        return 0;
    }

    unsigned long tag = 0;
    unsigned long index = 0;
    unsigned long offset = 0;

    // Decompose the address into Tag, Set Index, and Offset
    extract_address_fields(c, address, &tag, &index, &offset);

    CacheSet *set = &c->sets[index];

    // Increment global access counter on every read/write transaction
    c->access_counter++;

    // 1. Search for a Cache Hit
    for (int i = 0; i < c->associativity; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            // Hit! Update access order counter for LRU tracking
            set->lines[i].last_used = c->access_counter;
            return 1;
        }
    }

    // Miss! We must allocate or evict a block
    // 2. Search for an available empty (invalid) line in the target set
    for (int i = 0; i < c->associativity; i++) {
        if (!set->lines[i].valid) {
            // Empty block slot found. Populate it
            set->lines[i].valid = true;
            set->lines[i].tag = tag;
            set->lines[i].last_used = c->access_counter;
            set->lines[i].insertion_time = c->access_counter;
            return 0;
        }
    }

    // 3. Cache set is completely full! Perform eviction select
    int victim = -1;
    switch (c->policy) {
        case POLICY_LRU:
            victim = find_victim_lru(c, (int)index);
            break;
        case POLICY_FIFO:
            victim = find_victim_fifo(c, (int)index);
            break;
        case POLICY_RANDOM:
            victim = find_victim_random(c);
            break;
        default:
            victim = find_victim_lru(c, (int)index);
            break;
    }

    // Evict victim and install new block tag and tracking metadata
    set->lines[victim].tag = tag;
    set->lines[victim].last_used = c->access_counter;
    set->lines[victim].insertion_time = c->access_counter;

    return 0;
}
