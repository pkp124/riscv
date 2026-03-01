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

# Absolute paths for Renode (required for CreateFileBackend in headless mode)
ELF_ABS="$(cd "$(dirname "$ELF_PATH")" && pwd)/$(basename "$ELF_PATH")"
UART_FILE="${WORK_DIR}/uart_output.txt"

cd "$WORK_DIR"

# Run Renode with 15s timeout (app completes in ~1s; timeout ensures clean exit)
# Pass elf and uart_file for run.resc
timeout 15 "$RENODE" -e "set elf \"$ELF_ABS\"; set uart_file \"$UART_FILE\"; s @${RESC_SCRIPT}" > renode.log 2>&1 || true

# Output UART capture for CTest validation
if [ -f "$UART_FILE" ]; then
    cat "$UART_FILE"
else
    echo "[ERROR] UART output file not found at $UART_FILE. Renode log:"
    cat renode.log 2>/dev/null || true
    exit 1
fi
