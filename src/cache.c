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
    c->policy = POLICY_LRU; // Default policy placeholder

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
            // Create a mask with 1s in the lower 'offset_bits' positions
            unsigned long offset_mask = (1UL << c->offset_bits) - 1;
            *offset = address & offset_mask;
        }
    }

    // 2. Extract Set Index (next 'index_bits' bits, shifted right by 'offset_bits')
    if (index) {
        if (c->index_bits >= word_bits || c->offset_bits >= word_bits) {
            *index = 0;
        } else {
            // Create a mask with 1s in the lower 'index_bits' positions
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

bool cache_access(Cache *c, uint64_t address, char mode) {
    // Keep placeholder implementation for compiling
    (void)c;
    (void)address;
    (void)mode;
    return false;
}
