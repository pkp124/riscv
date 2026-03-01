#!/usr/bin/env bash
# =============================================================================
# Run gem5 cycle count comparison test
# =============================================================================
# Runs gem5 twice (Atomic and Timing CPU models), then compares stats using
# parse-gem5-stats.py. Used by Phase 6 CTest.
#
# Usage: run-gem5-compare-test.sh <GEM5_OPT> <GEM5_FS_CONFIG> <APP_ELF> <PARSE_SCRIPT> <WORK_DIR>
# =============================================================================

set -e

if [ $# -lt 5 ]; then
    echo "Usage: $0 <GEM5_OPT> <GEM5_FS_CONFIG> <APP_ELF> <PARSE_SCRIPT> <WORK_DIR>"
    exit 1
fi

GEM5_OPT="$1"
GEM5_FS_CONFIG="$2"
APP_ELF="$3"
PARSE_SCRIPT="$4"
WORK_DIR="$5"

# Reduced tick count for faster comparison (50M ticks ~ few seconds)
MAX_TICKS=50000000

mkdir -p "$WORK_DIR"
cd "$WORK_DIR"

echo "=== gem5 Cycle Count Comparison Test ==="
echo "Running AtomicSimpleCPU..."
mkdir -p atomic
cd atomic
"$GEM5_OPT" "$GEM5_FS_CONFIG" \
    --cpu-type=AtomicSimpleCPU \
    --cmd="$APP_ELF" \
    --max-ticks=$MAX_TICKS \
    > gem5_atomic.log 2>&1
cd ..

echo "Running TimingSimpleCPU..."
mkdir -p timing
cd timing
"$GEM5_OPT" "$GEM5_FS_CONFIG" \
    --cpu-type=TimingSimpleCPU \
    --cmd="$APP_ELF" \
    --max-ticks=$MAX_TICKS \
    > gem5_timing.log 2>&1
cd ..

STATS_ATOMIC="atomic/m5out/stats.txt"
STATS_TIMING="timing/m5out/stats.txt"

if [ ! -f "$STATS_ATOMIC" ]; then
    echo "Error: Atomic stats not found at $WORK_DIR/$STATS_ATOMIC"
    cat atomic/gem5_atomic.log 2>/dev/null || true
    exit 1
fi

if [ ! -f "$STATS_TIMING" ]; then
    echo "Error: Timing stats not found at $WORK_DIR/$STATS_TIMING"
    cat timing/gem5_timing.log 2>/dev/null || true
    exit 1
fi

echo ""
echo "Comparing CPU models..."
"$PARSE_SCRIPT" --compare "$STATS_ATOMIC" "$STATS_TIMING"

echo ""
echo "=== gem5 cycle comparison test PASSED ==="
