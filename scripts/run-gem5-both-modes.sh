#!/usr/bin/env bash
# =============================================================================
# Run gem5 Simulations for Both Modes (SE and FS)
# =============================================================================
# This script builds and runs gem5 simulations in both:
#   - SE (Syscall Emulation): Simpler, faster, uses ecall-based I/O
#   - FS (Full System): Bare-metal with UART, more realistic
#
# Prerequisites:
#   - RISC-V toolchain (riscv64-unknown-elf-gcc)
#   - gem5 at /opt/gem5/build/RISCV/gem5.opt
#     Use devcontainer (includes gem5), or: INSTALL_GEM5=1 ./scripts/setup-simulators.sh
#
# Usage:
#   ./scripts/run-gem5-both-modes.sh [--build-only] [--se-only] [--fs-only]
#
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Find gem5: GEM5_OPT env, workspace build, or /opt/gem5
if [[ -z "${GEM5_OPT:-}" ]]; then
    if [[ -f "${PROJECT_ROOT}/gem5-build/build/RISCV/gem5.opt" ]]; then
        GEM5_OPT="${PROJECT_ROOT}/gem5-build/build/RISCV/gem5.opt"
    elif [[ -f "/opt/gem5/build/RISCV/gem5.opt" ]]; then
        GEM5_OPT="/opt/gem5/build/RISCV/gem5.opt"
    else
        GEM5_OPT="/opt/gem5/build/RISCV/gem5.opt"
    fi
fi
BUILD_ONLY=false
SE_ONLY=false
FS_ONLY=false

# Parse arguments
for arg in "$@"; do
    case "$arg" in
        --build-only) BUILD_ONLY=true ;;
        --se-only)    SE_ONLY=true ;;
        --fs-only)    FS_ONLY=true ;;
        -h|--help)
            echo "Usage: $0 [--build-only] [--se-only] [--fs-only]"
            echo ""
            echo "  --build-only  Build ELFs only, do not run gem5"
            echo "  --se-only     Run only SE (Syscall Emulation) mode"
            echo "  --fs-only     Run only FS (Full System) mode"
            exit 0
            ;;
    esac
done

# If neither --se-only nor --fs-only, run both
if [[ "$SE_ONLY" == "false" && "$FS_ONLY" == "false" ]]; then
    SE_ONLY=true
    FS_ONLY=true
fi

cd "${PROJECT_ROOT}"

# -----------------------------------------------------------------------------
# Step 1: Build ELFs for both modes
# -----------------------------------------------------------------------------
echo "=============================================="
echo "Building gem5 ELF binaries"
echo "=============================================="

if [[ "$SE_ONLY" == "true" ]]; then
    echo ""
    echo "--- Building gem5 SE mode ---"
    cmake --preset gem5-se
    cmake --build build/gem5-se
    ELF_SE=$(find build/gem5-se -name 'app' -type f 2>/dev/null | head -1)
    if [[ -z "$ELF_SE" ]]; then
        echo "ERROR: gem5 SE ELF not found"
        exit 1
    fi
    echo "  SE ELF: ${ELF_SE}"
fi

if [[ "$FS_ONLY" == "true" ]]; then
    echo ""
    echo "--- Building gem5 FS mode ---"
    cmake --preset gem5-fs
    cmake --build build/gem5-fs
    ELF_FS=$(find build/gem5-fs -name 'app' -type f 2>/dev/null | head -1)
    if [[ -z "$ELF_FS" ]]; then
        echo "ERROR: gem5 FS ELF not found"
        exit 1
    fi
    echo "  FS ELF: ${ELF_FS}"
fi

if [[ "$BUILD_ONLY" == "true" ]]; then
    echo ""
    echo "Build complete. Use without --build-only to run simulations."
    exit 0
fi

# -----------------------------------------------------------------------------
# Step 2: Check for gem5
# -----------------------------------------------------------------------------
if [[ ! -f "$GEM5_OPT" ]]; then
    echo ""
    echo "=============================================="
    echo "ERROR: gem5 not found at ${GEM5_OPT}"
    echo "=============================================="
    echo ""
    echo "To install gem5 (takes 30-60 minutes):"
    echo "  INSTALL_GEM5=1 ./scripts/setup-simulators.sh"
    echo ""
    echo "Or set GEM5_OPT to your gem5.opt path:"
    echo "  GEM5_OPT=/path/to/gem5.opt $0"
    echo ""
    exit 1
fi

echo ""
echo "Using gem5: ${GEM5_OPT}"
"${GEM5_OPT}" --version 2>/dev/null || true

# -----------------------------------------------------------------------------
# Step 3: Run gem5 SE simulation
# -----------------------------------------------------------------------------
if [[ "$SE_ONLY" == "true" ]]; then
    echo ""
    echo "=============================================="
    echo "gem5 SE (Syscall Emulation) Simulation"
    echo "=============================================="
    "${GEM5_OPT}" \
        platforms/gem5/configs/se_config.py \
        --cpu-type=AtomicSimpleCPU \
        --cmd="${ELF_SE}" \
        --max-ticks=100000000

    if [[ -f m5out/stats.txt ]]; then
        echo ""
        echo "--- gem5 SE Stats ---"
        python3 scripts/parse-gem5-stats.py m5out/stats.txt 2>/dev/null || true
    fi
fi

# -----------------------------------------------------------------------------
# Step 4: Run gem5 FS simulation
# -----------------------------------------------------------------------------
if [[ "$FS_ONLY" == "true" ]]; then
    echo ""
    echo "=============================================="
    echo "gem5 FS (Full System) Simulation"
    echo "=============================================="
    "${GEM5_OPT}" \
        platforms/gem5/configs/fs_config.py \
        --cpu-type=AtomicSimpleCPU \
        --cmd="${ELF_FS}" \
        --max-ticks=100000000

    if [[ -f m5out/stats.txt ]]; then
        echo ""
        echo "--- gem5 FS Stats ---"
        python3 scripts/parse-gem5-stats.py m5out/stats.txt 2>/dev/null || true
    fi
fi

echo ""
echo "=============================================="
echo "gem5 simulations complete"
echo "=============================================="
