# PowerShell benchmark script for native Windows environment

# Ensure the bin directory has the compiled executable
if (-not (Test-Path "bin/cachesim.exe")) {
    Write-Error "Executable bin/cachesim.exe not found. Please compile using 'make' first."
    Exit 1
}

$traces = @(
    "traces/sequential_pattern.txt",
    "traces/loop_pattern.txt",
    "traces/random_pattern.txt"
)

$policies = @("lru", "fifo", "random")
$associativities = @(2, 4)
$cache_size = 1024
$block_size = 32

$resultsFile = "results.csv"

# Initialize CSV with header row
"trace,cache_size,block_size,associativity,policy,hit_rate" | Out-File -FilePath $resultsFile -Encoding ascii

Write-Host "Starting batch benchmarks..."
Write-Host "Results will be saved to $resultsFile"

foreach ($trace in $traces) {
    foreach ($assoc in $associativities) {
        foreach ($policy in $policies) {
            Write-Host "Running: Trace=$trace, Size=$cache_size, Block=$block_size, Assoc=$assoc, Policy=$policy"

            # Execute simulation and capture output lines
            $output = & "./bin/cachesim.exe" -c $cache_size -b $block_size -a $assoc -p $policy -t $trace -s 42

            $hit_rate = "0.00"
            foreach ($line in $output) {
                if ($line -match "Hit Rate:\s+([0-9.]+)%") {
                    $hit_rate = $Matches[1]
                    break
                }
            }

            $traceName = [System.IO.Path]::GetFileName($trace)
            # Append result row to CSV
            "$traceName,$cache_size,$block_size,$assoc,$policy,$hit_rate" | Out-File -FilePath $resultsFile -Append -Encoding ascii
        }
    }
}

Write-Host "Benchmarks finished successfully! CSV written to $resultsFile"
