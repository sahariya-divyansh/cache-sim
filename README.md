# Configurable Cache Simulator (cache-sim)

A high-performance, configurable memory cache simulator written in C to evaluate hit/miss rates across different cache design configurations (LRU, FIFO, and RANDOM replacement policies).

---

## Architectural Overview & Cache Concepts

In modern architectures, the cache is divided into sets, each containing one or more cache lines. A memory address is split into three main components: **Tag**, **Set Index**, and **Block Offset**:

```
+------------------------------------+----------------------------------+------------------------------+
|          Tag (tag_bits)            |       Index (index_bits)         |     Offset (offset_bits)     |
+------------------------------------+----------------------------------+------------------------------+
```

1. **Block Offset**: Determines the specific byte location within a cache block.
   $$\text{offset\\_bits} = \log_2(\text{block\\_size})$$
2. **Set Index**: Selects the target set within the cache structure.
   $$\text{num\\_sets} = \frac{\text{cache\\_size}}{\text{block\\_size} \times \text{associativity}}$$
   $$\text{index\\_bits} = \log_2(\text{num\\_sets})$$
3. **Tag**: Uniquely identifies the memory block stored in a cache line.
   $$\text{tag\\_bits} = \text{word\\_size} - \text{index\\_bits} - \text{offset\\_bits}$$

This simulator uses high-speed bit masking and bit-shifts in the critical access loop to resolve set locations and tags with zero pointer dereferences or branching overhead.

---

## Directory Structure

```
cache-sim/
├── include/
│   └── cache.h              # Data structures and prototypes
├── src/
│   ├── cache.c              # Eviction logic (LRU, FIFO, LCG RANDOM)
│   └── main.c               # Entry point and multi-format trace parser
├── scripts/
│   ├── run_benchmarks.ps1   # PowerShell batch benchmarking runner
│   ├── run_benchmarks.sh    # Bash batch benchmarking runner
│   └── plot_results.py      # Python script to plot cache metrics
├── traces/                  # Pre-configured access patterns (.txt)
│   ├── sequential_pattern.txt
│   ├── loop_pattern.txt
│   └── random_pattern.txt
├── Makefile                 # C11 GNU Make file
└── README.md                # This documentation
```

---

## Getting Started

### Prerequisites

You need a C compiler (`gcc`), `make`, and optionally `python` (with `matplotlib`) to generate benchmark plots.
- On Windows: Install **MSYS2 / MinGW-w64** via winget (`winget install -e --id MSYS2.MSYS2`).
- Make sure standard binaries are added to your system `PATH`.

### Compilation

Build the simulator executable:
```bash
make
```

To clean the compiled object files and binaries:
```bash
make clean
```

---

## Usage

Run the cache simulator with the following options:
```bash
./bin/cachesim -c <cache_size> -b <block_size> -a <associativity> -p <policy> -t <tracefile> [-s <seed>]
```

### Parameter Description:
- `-c <cache_size>`: Total cache capacity in bytes (must be a power of 2).
- `-b <block_size>`: Size of a cache line block in bytes (must be a power of 2).
- `-a <associativity>`: Level of associativity (1 for direct-mapped, $2^N$ for set-associative, or capacity size for fully-associative).
- `-p <policy>`: Eviction/replacement policy (`lru`, `fifo`, `random`).
- `-t <tracefile>`: File path to the memory access trace.
- `-s <seed>`: (Optional) Initial seed for RANDOM replacement (default: `42`).

### Example Run:
```bash
./bin/cachesim -c 1024 -b 32 -a 4 -p lru -t traces/sequential_pattern.txt
```

---

## Trace File Format

The simulator's trace parser is robust and accepts two common text formats:
1. **Address-only format** (one hexadecimal address per line):
   ```
   0x7fff1234
   0x7fff1238
   ```
2. **Operation-Address format** (character operation followed by address):
   ```
   l 0x7fff1234
   s 0x7fff1238
   ```
Empty lines and lines starting with `#` comments are safely ignored. Malformed lines will display warnings on `stderr` without causing crashes.

---

## Benchmark Results

Running the automated script (`scripts/run_benchmarks.ps1` or `scripts/run_benchmarks.sh`) evaluates the three replacement policies under a 1024-byte cache and 32-byte block size configuration:

| Access Pattern | Cache Associativity | LRU Hit Rate | FIFO Hit Rate | RANDOM Hit Rate |
| :--- | :--- | :--- | :--- | :--- |
| **Sequential** | 2-way | 75.00% | 75.00% | 75.00% |
| **Sequential** | 4-way | 75.00% | 75.00% | 75.00% |
| **Loop Pattern** | 2-way | 49.80% | 25.00% | 24.60% |
| **Loop Pattern** | 4-way | 49.80% | 37.50% | 44.44% |
| **Random** | 2-way | 0.40% | 0.40% | 0.40% |
| **Random** | 4-way | 0.40% | 0.40% | 0.20% |

### Hot-Cold Interleaved Trace Design Insight
The `loop_pattern.txt` trace is designed as a "hot-cold" interleaved access pattern mapping entirely to Set 0 (`0x0`, `0x200`, `0x0`, `0x400`, `0x0`, `0x600`, ...). Block `0x0` acts as a hot line accessed every second transaction, while other blocks act as cold lines that thrash the set.

- **LRU (Least Recently Used)**: Recency-aware. Because `0x0` is accessed frequently, LRU maintains it in the set, yielding a **49.80%** hit rate (nearly 100% hits on the hot line).
- **FIFO (First-In, First-Out)**: Agnostic to hits. FIFO does not update insertion times on hits, so the hot line `0x0` is evicted when its original insertion order becomes the oldest, yielding a significantly lower hit rate (**25.00%** under 2-way, **37.50%** under 4-way).

This trace demonstrates the fundamental performance advantage of recency-aware policies (LRU) over simple time-in-cache policies (FIFO) on skewed access distributions.
