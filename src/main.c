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
    fprintf(stderr, "Usage: %s -c <cache_size> -b <block_size> -a <associativity> -p <policy> -t <tracefile> [-s <seed>]\n", prog_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -c <cache_size>    Total cache size in bytes (must be a power of 2)\n");
    fprintf(stderr, "  -b <block_size>    Block size in bytes (must be a power of 2)\n");
    fprintf(stderr, "  -a <associativity> Associativity (1 for direct-mapped, power of 2, or total size for fully associative)\n");
    fprintf(stderr, "  -p <policy>        Replacement policy (lru, fifo, random)\n");
    fprintf(stderr, "  -t <tracefile>     Path to the memory access trace file\n");
    fprintf(stderr, "  -s <seed>          Random seed state for RANDOM replacement policy (default: 42)\n");
}

int main(int argc, char *argv[]) {
    int opt;
    long cache_size = -1;
    long block_size = -1;
    long associativity = -1;
    char *policy_str = NULL;
    char *tracefile = NULL;
    long seed = 42;

    // Parsing options using getopt (supporting -c and -s)
    while ((opt = getopt(argc, argv, "c:b:a:p:t:s:")) != -1) {
        switch (opt) {
            case 'c':
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
            case 's':
                seed = atol(optarg);
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
    cache->rng_state = (unsigned int)seed;

    printf("==========================================\n");
    printf(" Cache Simulator Configuration\n");
    printf("==========================================\n");
    printf("  Cache Size:    %d bytes\n", cache->cache_size);
    printf("  Block Size:    %d bytes\n", cache->block_size);
    printf("  Associativity: %d-way\n", cache->associativity);
    printf("  Policy:        %s\n", policy_str);
    if (policy == POLICY_RANDOM) {
        printf("  Random Seed:   %ld\n", seed);
    }
    printf("  Trace File:    %s\n", tracefile);
    printf("------------------------------------------\n");
    printf(" Computed Cache Parameters:\n");
    printf("  Num Sets:      %d\n", cache->num_sets);
    printf("  Offset Bits:   %d\n", cache->offset_bits);
    printf("  Index Bits:    %d\n", cache->index_bits);
    printf("  Tag Bits:      %d\n", cache->tag_bits);
    printf("==========================================\n");

    // Open and parse trace file
    FILE *tf = fopen(tracefile, "r");
    if (!tf) {
        fprintf(stderr, "Error: Could not open trace file '%s'.\n", tracefile);
        cache_destroy(cache);
        return EXIT_FAILURE;
    }

    unsigned long addr;
    long long hits = 0;
    long long misses = 0;
    long long line_num = 0;

    char line_buf[256];
    while (fgets(line_buf, sizeof(line_buf), tf)) {
        line_num++;

        // Trim leading whitespace
        char *ptr = line_buf;
        while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r') {
            ptr++;
        }

        // Skip comments and empty lines
        if (*ptr == '\0' || *ptr == '#') {
            continue;
        }

        int parsed = 0;
        char op = 'l'; // Default operation

        // 1. Try to parse operation-address format (e.g., 'l 0x7fff1234' or 's 1000')
        if (sscanf(ptr, " %c 0x%lx", &op, &addr) == 2 || sscanf(ptr, " %c %lx", &op, &addr) == 2) {
            if (op == 'l' || op == 's' || op == 'I' || op == 'D' || op == 'M') {
                parsed = 1;
            }
        }

        // 2. Try to parse address-only format (e.g., '0x7fff1234' or '7fff1234')
        if (!parsed) {
            if (sscanf(ptr, "0x%lx", &addr) == 1 || sscanf(ptr, "%lx", &addr) == 1) {
                parsed = 1;
            }
        }

        if (parsed) {
            int outcome = cache_access(cache, addr);
            if (outcome == 1) {
                hits++;
            } else {
                misses++;
            }
        } else {
            // Warn on malformed lines instead of crashing
            // Remove trailing newline for print
            size_t len = strlen(line_buf);
            if (len > 0 && line_buf[len - 1] == '\n') {
                line_buf[len - 1] = '\0';
            }
            fprintf(stderr, "Warning: Skipping malformed trace line at line %lld: \"%s\"\n", line_num, line_buf);
        }
    }
    fclose(tf);

    long long total_accesses = hits + misses;
    double hit_rate = total_accesses > 0 ? (double)hits / total_accesses * 100.0 : 0.0;
    double miss_rate = total_accesses > 0 ? (double)misses / total_accesses * 100.0 : 0.0;

    printf("\n");
    printf("==========================================\n");
    printf(" Simulation Results Summary\n");
    printf("==========================================\n");
    printf("  Total Accesses: %lld\n", total_accesses);
    printf("  Hits:           %lld\n", hits);
    printf("  Misses:         %lld\n", misses);
    printf("  Hit Rate:       %.2f%%\n", hit_rate);
    printf("  Miss Rate:      %.2f%%\n", miss_rate);
    printf("==========================================\n");

    // Clean up memory before exiting
    cache_destroy(cache);

    return EXIT_SUCCESS;
}
