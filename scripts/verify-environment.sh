#!/bin/bash
# =============================================================================
# Environment Verification Script
# =============================================================================
# This script checks that all required tools and dependencies are installed.
# =============================================================================

set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_pass() {
    echo -e "${GREEN}✓${NC} $1"
}

log_fail() {
    echo -e "${RED}✗${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}⚠${NC} $1"
}

log_section() {
    echo ""
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}$(printf '=%.0s' {1..66})${NC}"
}

# Track status
ERRORS=0
WARNINGS=0

# =============================================================================
# Check Functions
# =============================================================================

check_command() {
    local cmd=$1
    local required=$2
    local install_hint=$3
    
    if command -v "$cmd" &>/dev/null; then
        local version=$("$cmd" --version 2>&1 | head -1 || echo "unknown version")
        log_pass "$cmd: $version"
    else
        if [ "$required" = "required" ]; then
            log_fail "$cmd: NOT FOUND (required)"
            echo "     Install: $install_hint"
            ERRORS=$((ERRORS + 1))
        else
            log_warn "$cmd: not found (optional)"
            echo "     Install: $install_hint"
            WARNINGS=$((WARNINGS + 1))
        fi
    fi
}

check_toolchain() {
    local prefix="riscv64-unknown-elf-"
    
    if command -v "${prefix}gcc" &>/dev/null; then
        local version=$("${prefix}gcc" --version | head -1)
        log_pass "RISC-V Toolchain: $version"
        
        # Check for individual tools
        for tool in gcc as ld objcopy objdump size gdb; do
            if command -v "${prefix}${tool}" &>/dev/null; then
                log_pass "  - ${prefix}${tool}"
            else
                log_fail "  - ${prefix}${tool}: missing"
                ERRORS=$((ERRORS + 1))
            fi
        done
    else
        log_fail "RISC-V Toolchain: NOT FOUND"
        echo "     Install: ./scripts/setup-toolchain.sh"
        ERRORS=$((ERRORS + 1))
    fi
}

check_file() {
    local file=$1
    local required=$2
    local description=$3
    
    if [ -f "$file" ]; then
        log_pass "$description: $file"
    else
        if [ "$required" = "required" ]; then
            log_fail "$description: NOT FOUND"
            echo "     Expected: $file"
            ERRORS=$((ERRORS + 1))
        else
            log_warn "$description: not found (will be created)"
        fi
    fi
}

# =============================================================================
# Verification
# =============================================================================

log_section "Build Tools"
check_command cmake required "sudo apt install cmake"
check_command make optional "sudo apt install make"
check_command ninja optional "sudo apt install ninja-build"
check_command git required "sudo apt install git"

log_section "RISC-V Toolchain"
check_toolchain

log_section "Simulators"
check_command qemu-system-riscv64 required "./scripts/setup-simulators.sh"
check_command spike optional "./scripts/setup-simulators.sh"

# Check gem5
if [ -f "/opt/gem5/build/RISCV/gem5.opt" ]; then
    log_pass "gem5: /opt/gem5/build/RISCV/gem5.opt"
else
    log_warn "gem5: not installed (optional)"
    echo "     Install: INSTALL_GEM5=1 ./scripts/setup-simulators.sh"
    WARNINGS=$((WARNINGS + 1))
fi

# Check Renode
check_command renode optional "https://renode.io/"

log_section "Code Quality Tools"
check_command clang-format optional "sudo apt install clang-format"
check_command cppcheck optional "sudo apt install cppcheck"
check_command clang-tidy optional "sudo apt install clang-tidy"

log_section "Python Tools"
check_command python3 required "sudo apt install python3"
check_command pip3 optional "sudo apt install python3-pip"

log_section "Project Files"
check_file "CMakeLists.txt" required "Root CMake file"
check_file "CMakePresets.json" required "CMake presets"
check_file ".clang-format" optional "clang-format config"
check_file "ROADMAP.md" required "Project roadmap"

log_section "Project Directories"
for dir in cmake tests app platforms scripts; do
    if [ -d "$dir" ]; then
        log_pass "Directory: $dir/"
    else
        log_warn "Directory: $dir/ (will be created)"
    fi
done

# =============================================================================
# Summary
# =============================================================================

log_section "Summary"

echo ""
if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    log_pass "Environment is fully configured!"
    echo ""
    echo "Next steps:"
    echo "  1. Configure: cmake --preset default"
    echo "  2. Build:     cmake --build build/default"
    echo "  3. Test:      ctest --test-dir build/default --output-on-failure"
    exit 0
elif [ $ERRORS -eq 0 ]; then
    log_warn "Environment is ready with $WARNINGS optional tools missing"
    echo ""
    echo "You can proceed, but some features may not be available."
    exit 0
else
    log_fail "Environment has $ERRORS errors and $WARNINGS warnings"
    echo ""
    echo "Please install missing required tools before proceeding."
    exit 1
fi
