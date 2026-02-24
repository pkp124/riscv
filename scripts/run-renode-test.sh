#!/usr/bin/env bash
# =============================================================================
# Run Renode simulation and output UART capture for CTest validation
# =============================================================================
# Renode writes UART to a file (CreateFileBackend), not stdout. This script
# runs Renode then cats the UART output so CTest PASS_REGULAR_EXPRESSION works.
#
# Usage: run-renode-test.sh <RENODE> <ELF_PATH> <WORK_DIR> [RESC_SCRIPT]
#   RESC_SCRIPT: optional, default platforms/renode/configs/run.resc
#                use platforms/renode/configs/run_smp.resc for SMP
# =============================================================================

set -e

if [ $# -lt 3 ]; then
    echo "Usage: $0 <RENODE> <ELF_PATH> <WORK_DIR> [RESC_SCRIPT]"
    exit 1
fi

RENODE="$1"
ELF_PATH="$2"
WORK_DIR="$3"
RESC_SCRIPT="${4:-platforms/renode/configs/run.resc}"

cd "$WORK_DIR"

# Run Renode (output goes to uart_output.txt)
"$RENODE" -e "set elf \"$ELF_PATH\"; s @${RESC_SCRIPT}" > renode.log 2>&1 || true

# Output UART capture for CTest validation
if [ -f "uart_output.txt" ]; then
    cat uart_output.txt
else
    echo "[ERROR] UART output file not found. Renode log:"
    cat renode.log 2>/dev/null || true
    exit 1
fi
