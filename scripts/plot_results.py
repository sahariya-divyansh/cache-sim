import os
import csv
import matplotlib.pyplot as plt
import numpy as np

csv_path = "results.csv"
output_path = "results.png"

if not os.path.exists(csv_path):
    print(f"Error: {csv_path} not found. Please run scripts/run_benchmarks first.")
    exit(1)

# Grouping results by (trace, associativity, policy) -> hit_rate
data = {}
traces = set()
policies = ["lru", "fifo", "random"]
associativities = set()

with open(csv_path, mode='r') as f:
    reader = csv.DictReader(f)
    for row in reader:
        trace = row['trace'].replace("_pattern.txt", "").capitalize()
        assoc = int(row['associativity'])
        policy = row['policy']
        hit_rate = float(row['hit_rate'])
        
        traces.add(trace)
        associativities.add(assoc)
        data[(trace, assoc, policy)] = hit_rate

traces = sorted(list(traces))
associativities = sorted(list(associativities))

# Configure plot subplots based on associativities tested
fig, axes = plt.subplots(1, len(associativities), figsize=(12, 5), sharey=True)
if len(associativities) == 1:
    axes = [axes]

# Premium HSL color scheme
colors = {
    'lru': '#3498db',    # Bright blue
    'fifo': '#e74c3c',   # Deep red
    'random': '#2ecc71'  # Soft green
}

x = np.arange(len(traces))
width = 0.25

for idx, assoc in enumerate(associativities):
    ax = axes[idx]
    
    # Plot bars for each replacement policy
    for p_idx, policy in enumerate(policies):
        rates = []
        for trace in traces:
            rates.append(data.get((trace, assoc, policy), 0.0))
        
        offset = (p_idx - 1) * width
        ax.bar(x + offset, rates, width, label=policy.upper(), color=colors[policy], edgecolor='black', linewidth=0.5)
        
    ax.set_title(f"Associativity: {assoc}-way Set Associative", fontsize=11, fontweight='bold', pad=10)
    ax.set_xticks(x)
    ax.set_xticklabels(traces, fontsize=10)
    ax.set_xlabel("Workload Trace Pattern", fontsize=10, labelpad=8)
    ax.grid(axis='y', linestyle='--', alpha=0.4)

axes[0].set_ylabel("Cache Hit Rate (%)", fontsize=10, labelpad=8)
axes[0].set_ylim(0, 105)

# Place legend cleanly
plt.legend(title="Policy", frameon=True, bbox_to_anchor=(1.05, 1), loc='upper left')
plt.suptitle("Performance Comparison of Cache Eviction Policies", fontsize=13, fontweight='bold', y=0.98)
plt.tight_layout()

plt.savefig(output_path, dpi=300, bbox_inches='tight')
print(f"Benchmark plot successfully generated and saved to {output_path}")
