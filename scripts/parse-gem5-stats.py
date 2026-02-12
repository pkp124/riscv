#!/usr/bin/env python3
"""
gem5 Statistics Parser
======================

Parses gem5's stats.txt output and extracts key performance metrics:
  - Simulation ticks and seconds
  - CPI (Cycles Per Instruction)
  - Instructions executed
  - Cache hit/miss rates (L1I, L1D, L2)
  - Memory accesses
  - Pipeline statistics (for MinorCPU/O3CPU)

Usage:
  python3 parse-gem5-stats.py [options] <stats_file>
  python3 parse-gem5-stats.py m5out/stats.txt
  python3 parse-gem5-stats.py --json m5out/stats.txt
  python3 parse-gem5-stats.py --compare m5out/atomic/stats.txt m5out/timing/stats.txt

Options:
  --json        Output in JSON format
  --csv         Output in CSV format
  --compare     Compare two stats files side by side
  --verbose     Show all parsed stats
  --filter KEY  Only show stats matching KEY pattern
"""

import argparse
import json
import os
import re
import sys


def parse_stats_file(filepath):
    """Parse a gem5 stats.txt file and return a dictionary of stats."""
    stats = {}

    if not os.path.exists(filepath):
        print(f"Error: Stats file not found: {filepath}", file=sys.stderr)
        return stats

    with open(filepath, "r") as f:
        for line in f:
            line = line.strip()

            # Skip comments and empty lines
            if not line or line.startswith("---") or line.startswith("==="):
                continue

            # Parse stat lines: "stat_name  value  # description"
            match = re.match(r"^(\S+)\s+([\d.eE+-]+(?:%?)?)\s*(?:#.*)?$", line)
            if match:
                name = match.group(1)
                value_str = match.group(2)

                # Try to convert to numeric
                try:
                    if "." in value_str or "e" in value_str.lower():
                        value = float(value_str.rstrip("%"))
                    else:
                        value = int(value_str)
                except ValueError:
                    value = value_str

                stats[name] = value

    return stats


def extract_key_metrics(stats, cpu_id=0):
    """Extract key performance metrics from parsed stats."""
    prefix = f"system.cpu" if cpu_id == 0 else f"system.cpu{cpu_id}"

    # Try both cpu and cpu0 naming conventions
    def get_stat(name, default=None):
        # Try exact name first
        if name in stats:
            return stats[name]
        # Try with cpu prefix
        full_name = f"{prefix}.{name}"
        if full_name in stats:
            return stats[full_name]
        # Try system-level
        sys_name = f"system.{name}"
        if sys_name in stats:
            return stats[sys_name]
        return default

    metrics = {}

    # Simulation info
    metrics["sim_ticks"] = get_stat("simTicks", stats.get("simTicks"))
    metrics["sim_seconds"] = get_stat("simSeconds", stats.get("simSeconds"))
    metrics["sim_insts"] = get_stat("simInsts", stats.get("simInsts"))
    metrics["sim_ops"] = get_stat("simOps", stats.get("simOps"))
    metrics["host_seconds"] = get_stat("hostSeconds", stats.get("hostSeconds"))

    # CPU performance
    metrics["num_cycles"] = get_stat("numCycles")
    metrics["num_insts"] = get_stat(
        "committedInsts", get_stat("numInsts", get_stat("exec_context.thread_0.numInsts"))
    )
    metrics["cpi"] = get_stat("cpi")
    metrics["ipc"] = get_stat("ipc")

    # Calculate CPI/IPC if not directly available
    if metrics["cpi"] is None and metrics["num_cycles"] and metrics["num_insts"]:
        if metrics["num_insts"] > 0:
            metrics["cpi"] = metrics["num_cycles"] / metrics["num_insts"]
            metrics["ipc"] = metrics["num_insts"] / metrics["num_cycles"]

    # Cache statistics (L1 Data)
    metrics["l1d_hits"] = get_stat("dcache.overallHits::total")
    metrics["l1d_misses"] = get_stat("dcache.overallMisses::total")
    metrics["l1d_accesses"] = get_stat("dcache.overallAccesses::total")
    if metrics["l1d_accesses"] and metrics["l1d_accesses"] > 0 and metrics["l1d_hits"]:
        metrics["l1d_hit_rate"] = metrics["l1d_hits"] / metrics["l1d_accesses"] * 100.0
    else:
        metrics["l1d_hit_rate"] = None

    # Cache statistics (L1 Instruction)
    metrics["l1i_hits"] = get_stat("icache.overallHits::total")
    metrics["l1i_misses"] = get_stat("icache.overallMisses::total")
    metrics["l1i_accesses"] = get_stat("icache.overallAccesses::total")
    if metrics["l1i_accesses"] and metrics["l1i_accesses"] > 0 and metrics["l1i_hits"]:
        metrics["l1i_hit_rate"] = metrics["l1i_hits"] / metrics["l1i_accesses"] * 100.0
    else:
        metrics["l1i_hit_rate"] = None

    # Cache statistics (L2)
    metrics["l2_hits"] = stats.get("system.l2cache.overallHits::total")
    metrics["l2_misses"] = stats.get("system.l2cache.overallMisses::total")
    metrics["l2_accesses"] = stats.get("system.l2cache.overallAccesses::total")
    if metrics["l2_accesses"] and metrics["l2_accesses"] > 0 and metrics["l2_hits"]:
        metrics["l2_hit_rate"] = metrics["l2_hits"] / metrics["l2_accesses"] * 100.0
    else:
        metrics["l2_hit_rate"] = None

    # Memory controller
    metrics["mem_reads"] = stats.get("system.mem_ctrl.readReqs")
    metrics["mem_writes"] = stats.get("system.mem_ctrl.writeReqs")

    return metrics


def print_metrics(metrics, title="gem5 Performance Metrics"):
    """Print metrics in a human-readable table format."""
    print(f"\n{'=' * 60}")
    print(f"  {title}")
    print(f"{'=' * 60}")

    # Simulation info
    print(f"\n  Simulation Overview:")
    if metrics.get("sim_ticks"):
        print(f"    Sim Ticks:     {metrics['sim_ticks']:>15,}")
    if metrics.get("sim_seconds"):
        print(f"    Sim Seconds:   {metrics['sim_seconds']:>15.6f}")
    if metrics.get("sim_insts"):
        print(f"    Instructions:  {metrics['sim_insts']:>15,}")
    if metrics.get("sim_ops"):
        print(f"    Ops:           {metrics['sim_ops']:>15,}")
    if metrics.get("host_seconds"):
        print(f"    Host Seconds:  {metrics['host_seconds']:>15.2f}")

    # CPU performance
    print(f"\n  CPU Performance:")
    if metrics.get("num_cycles"):
        print(f"    Cycles:        {metrics['num_cycles']:>15,}")
    if metrics.get("num_insts"):
        print(f"    Instructions:  {metrics['num_insts']:>15,}")
    if metrics.get("cpi"):
        print(f"    CPI:           {metrics['cpi']:>15.4f}")
    if metrics.get("ipc"):
        print(f"    IPC:           {metrics['ipc']:>15.4f}")

    # Cache stats
    if metrics.get("l1d_accesses"):
        print(f"\n  L1 Data Cache:")
        print(f"    Accesses:      {metrics['l1d_accesses']:>15,}")
        print(f"    Hits:          {metrics['l1d_hits']:>15,}")
        print(f"    Misses:        {metrics['l1d_misses']:>15,}")
        if metrics.get("l1d_hit_rate"):
            print(f"    Hit Rate:      {metrics['l1d_hit_rate']:>14.2f}%")

    if metrics.get("l1i_accesses"):
        print(f"\n  L1 Instruction Cache:")
        print(f"    Accesses:      {metrics['l1i_accesses']:>15,}")
        print(f"    Hits:          {metrics['l1i_hits']:>15,}")
        print(f"    Misses:        {metrics['l1i_misses']:>15,}")
        if metrics.get("l1i_hit_rate"):
            print(f"    Hit Rate:      {metrics['l1i_hit_rate']:>14.2f}%")

    if metrics.get("l2_accesses"):
        print(f"\n  L2 Cache:")
        print(f"    Accesses:      {metrics['l2_accesses']:>15,}")
        print(f"    Hits:          {metrics['l2_hits']:>15,}")
        print(f"    Misses:        {metrics['l2_misses']:>15,}")
        if metrics.get("l2_hit_rate"):
            print(f"    Hit Rate:      {metrics['l2_hit_rate']:>14.2f}%")

    if metrics.get("mem_reads") or metrics.get("mem_writes"):
        print(f"\n  Memory Controller:")
        if metrics.get("mem_reads"):
            print(f"    Read Reqs:     {metrics['mem_reads']:>15,}")
        if metrics.get("mem_writes"):
            print(f"    Write Reqs:    {metrics['mem_writes']:>15,}")

    print(f"\n{'=' * 60}\n")


def print_comparison(metrics1, metrics2, title1="Config 1", title2="Config 2"):
    """Print side-by-side comparison of two metrics sets."""
    print(f"\n{'=' * 75}")
    print(f"  Performance Comparison: {title1} vs {title2}")
    print(f"{'=' * 75}")

    def fmt(val, is_float=False):
        if val is None:
            return "N/A"
        if is_float:
            return f"{val:.4f}"
        if isinstance(val, float):
            return f"{val:.2f}"
        return f"{val:,}"

    def ratio(v1, v2):
        if v1 is None or v2 is None or v2 == 0:
            return "N/A"
        return f"{v1 / v2:.2f}x"

    headers = ["Metric", title1, title2, "Ratio"]
    print(f"\n  {'Metric':<25} {title1:>15} {title2:>15} {'Ratio':>10}")
    print(f"  {'-' * 25} {'-' * 15} {'-' * 15} {'-' * 10}")

    comparisons = [
        ("Sim Ticks", "sim_ticks", False),
        ("Instructions", "sim_insts", False),
        ("Cycles", "num_cycles", False),
        ("CPI", "cpi", True),
        ("IPC", "ipc", True),
        ("L1D Hit Rate %", "l1d_hit_rate", True),
        ("L1I Hit Rate %", "l1i_hit_rate", True),
        ("L2 Hit Rate %", "l2_hit_rate", True),
    ]

    for label, key, is_float in comparisons:
        v1 = metrics1.get(key)
        v2 = metrics2.get(key)
        r = ratio(v1, v2) if not is_float else ratio(v1, v2)
        print(f"  {label:<25} {fmt(v1, is_float):>15} {fmt(v2, is_float):>15} {r:>10}")

    print(f"\n{'=' * 75}\n")


def main():
    parser = argparse.ArgumentParser(
        description="Parse and analyze gem5 stats.txt files"
    )
    parser.add_argument("stats_files", nargs="+", help="Path(s) to gem5 stats.txt")
    parser.add_argument("--json", action="store_true", help="Output in JSON format")
    parser.add_argument("--csv", action="store_true", help="Output in CSV format")
    parser.add_argument(
        "--compare", action="store_true", help="Compare two stats files"
    )
    parser.add_argument("--verbose", action="store_true", help="Show all parsed stats")
    parser.add_argument(
        "--filter", default=None, help="Filter stats by pattern"
    )
    parser.add_argument(
        "--cpu-id", type=int, default=0, help="CPU ID to extract stats for"
    )

    args = parser.parse_args()

    if args.compare and len(args.stats_files) != 2:
        print("Error: --compare requires exactly 2 stats files", file=sys.stderr)
        sys.exit(1)

    results = []
    for filepath in args.stats_files:
        stats = parse_stats_file(filepath)
        if not stats:
            print(f"Warning: No stats parsed from {filepath}", file=sys.stderr)
            continue

        metrics = extract_key_metrics(stats, args.cpu_id)
        results.append((filepath, stats, metrics))

    if not results:
        print("Error: No valid stats files found", file=sys.stderr)
        sys.exit(1)

    if args.json:
        output = {}
        for filepath, _, metrics in results:
            # Filter out None values for JSON
            output[filepath] = {k: v for k, v in metrics.items() if v is not None}
        print(json.dumps(output, indent=2))
    elif args.csv:
        # CSV header
        keys = [
            "file", "sim_ticks", "sim_insts", "num_cycles", "cpi", "ipc",
            "l1d_hit_rate", "l1i_hit_rate", "l2_hit_rate",
        ]
        print(",".join(keys))
        for filepath, _, metrics in results:
            values = [filepath]
            for k in keys[1:]:
                v = metrics.get(k)
                values.append(str(v) if v is not None else "")
            print(",".join(values))
    elif args.compare:
        title1 = os.path.basename(os.path.dirname(results[0][0])) or results[0][0]
        title2 = os.path.basename(os.path.dirname(results[1][0])) or results[1][0]
        print_comparison(results[0][2], results[1][2], title1, title2)
    elif args.verbose:
        for filepath, stats, _ in results:
            print(f"\n--- All stats from {filepath} ---")
            if args.filter:
                pattern = re.compile(args.filter, re.IGNORECASE)
                stats = {k: v for k, v in stats.items() if pattern.search(k)}
            for name in sorted(stats.keys()):
                print(f"  {name}: {stats[name]}")
    else:
        for filepath, _, metrics in results:
            print_metrics(metrics, title=f"gem5 Stats: {os.path.basename(filepath)}")


if __name__ == "__main__":
    main()
