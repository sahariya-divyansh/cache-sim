#!/bin/bash
# Bash benchmark script for Unix/WSL environments

# Ensure execution halts on errors
set -e

# Verify the executable exists
if [ ! -f "bin/cachesim" ] && [ ! -f "bin/cachesim.exe" ]; then
    echo "Error: Executable not found under bin/. Please build using 'make' first." >&2
    exit 1
fi

# Detect actual binary name (cachesim or cachesim.exe)
if [ -f "bin/cachesim" ]; then
    EXE="bin/cachesim"
else
    EXE="bin/cachesim.exe"
fi

TRACES=(
    "traces/sequential_pattern.txt"
    "traces/loop_pattern.txt"
    "traces/random_pattern.txt"
)

POLICIES=("lru" "fifo" "random")
ASSOCIATIVITIES=(2 4)
CACHE_SIZE=1024
BLOCK_SIZE=32

RESULTS_FILE="results.csv"

# Initialize header
echo "trace,cache_size,block_size,associativity,policy,hit_rate" > "$RESULTS_FILE"

echo "Starting Unix batch benchmarks..."

for trace in "${TRACES[@]}"; do
    for assoc in "${ASSOCIATIVITIES[@]}"; do
        for policy in "${POLICIES[@]}"; do
            echo "Running: Trace=$trace, Size=$CACHE_SIZE, Block=$BLOCK_SIZE, Assoc=$assoc, Policy=$policy"

            # Execute simulation
            output=$("$EXE" -c "$CACHE_SIZE" -b "$BLOCK_SIZE" -a "$assoc" -p "$policy" -t "$trace" -s 42)

            # Parse Hit Rate using grep and sed
            hit_rate=$(echo "$output" | grep "Hit Rate:" | sed -E 's/[[:space:]]*Hit Rate:[[:space:]]*([0-9.]+)%/\1/')

            if [ -z "$hit_rate" ]; then
                hit_rate="0.00"
            fi

            trace_name=$(basename "$trace")
            echo "$trace_name,$CACHE_SIZE,$BLOCK_SIZE,$assoc,$policy,$hit_rate" >> "$RESULTS_FILE"
        done
    done
done

echo "Benchmarks finished! CSV written to $RESULTS_FILE"
