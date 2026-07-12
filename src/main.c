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

    printf("==========================================\n");
    printf(" Cache Simulator Configuration\n");
    printf("==========================================\n");
    printf("  Cache Size:    %ld bytes\n", cache_size);
    printf("  Block Size:    %ld bytes\n", block_size);
    printf("  Associativity: %ld-way\n", associativity);
    printf("  Policy:        %s\n", policy_str);
    printf("  Trace File:    %s\n", tracefile);
    printf("==========================================\n");

    // Placeholder cache allocation verification
    Cache *cache = cache_create((size_t)cache_size, (size_t)block_size, (size_t)associativity, policy);
    if (!cache) {
        printf("Simulator skeleton initialized successfully (Returned stub NULL cache).\n");
    } else {
        cache_destroy(cache);
    }

    return EXIT_SUCCESS;
}
