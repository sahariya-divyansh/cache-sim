#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "cache.h"

/**
 * @brief Prints usage details for the command-line interface.
 * 
 * @param prog_name Name of the simulator executable.
 */
static void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s -s <cache_size> -b <block_size> -a <associativity> -p <policy> -t <tracefile>\n", prog_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s <cache_size>    Total cache size in bytes (must be a power of 2)\n");
    fprintf(stderr, "  -b <block_size>    Block size in bytes (must be a power of 2)\n");
    fprintf(stderr, "  -a <associativity> Associativity (1 for direct-mapped, power of 2, or total size for fully associative)\n");
    fprintf(stderr, "  -p <policy>        Replacement policy (lru, fifo, random)\n");
    fprintf(stderr, "  -t <tracefile>     Path to the memory access trace file\n");
}

int main(int argc, char *argv[]) {
    int opt;
    long cache_size = -1;
    long block_size = -1;
    long associativity = -1;
    char *policy_str = NULL;
    char *tracefile = NULL;

    // Parsing options using getopt
    while ((opt = getopt(argc, argv, "s:b:a:p:t:")) != -1) {
        switch (opt) {
            case 's':
                cache_size = atol(optarg);
                break;
            case 'b':
                block_size = atol(optarg);
                break;
            case 'a':
                associativity = atol(optarg);
                break;
            case 'p':
                policy_str = optarg;
                break;
            case 't':
                tracefile = optarg;
                break;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Validate parameters presence
    if (cache_size <= 0 || block_size <= 0 || associativity <= 0 || !policy_str || !tracefile) {
        fprintf(stderr, "Error: Missing or invalid required configuration arguments.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Parse replacement policy
    ReplacementPolicy policy;
    if (strcmp(policy_str, "lru") == 0) {
        policy = POLICY_LRU;
    } else if (strcmp(policy_str, "fifo") == 0) {
        policy = POLICY_FIFO;
    } else if (strcmp(policy_str, "random") == 0) {
        policy = POLICY_RANDOM;
    } else {
        fprintf(stderr, "Error: Invalid replacement policy '%s'. Choose from: lru, fifo, random\n", policy_str);
        return EXIT_FAILURE;
    }

    // Initialize cache simulator core structures
    Cache *cache = cache_init((int)cache_size, (int)block_size, (int)associativity);
    if (!cache) {
        fprintf(stderr, "Error: Failed to initialize cache structure.\n");
        return EXIT_FAILURE;
    }
    cache->policy = policy;

    printf("==========================================\n");
    printf(" Cache Simulator Configuration\n");
    printf("==========================================\n");
    printf("  Cache Size:    %d bytes\n", cache->cache_size);
    printf("  Block Size:    %d bytes\n", cache->block_size);
    printf("  Associativity: %d-way\n", cache->associativity);
    printf("  Policy:        %s\n", policy_str);
    printf("  Trace File:    %s\n", tracefile);
    printf("------------------------------------------\n");
    printf(" Computed Cache Parameters:\n");
    printf("  Num Sets:      %d\n", cache->num_sets);
    printf("  Offset Bits:   %d\n", cache->offset_bits);
    printf("  Index Bits:    %d\n", cache->index_bits);
    printf("  Tag Bits:      %d\n", cache->tag_bits);
    printf("==========================================\n");

    // Address decomposition logic verification
    unsigned long sample_addr = 0xABCD1234UL;
    unsigned long tag = 0, index = 0, offset = 0;
    extract_address_fields(cache, sample_addr, &tag, &index, &offset);
    printf("Address Decomposition Check (Addr: 0x%lX):\n", sample_addr);
    printf("  Tag:    0x%lX\n", tag);
    printf("  Index:  0x%lX\n", index);
    printf("  Offset: 0x%lX\n", offset);
    printf("==========================================\n");

    // Clean up memory before exiting
    cache_destroy(cache);

    return EXIT_SUCCESS;
}
