#!/bin/bash
# =============================================================================
# RISC-V Simulators Setup Script
# =============================================================================
# This script installs RISC-V simulators:
#   - QEMU (qemu-system-riscv64)
#   - Spike (RISC-V ISA Simulator)
#   - gem5 (optional, takes long time)
#   - Renode (optional)
# =============================================================================

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SPIKE_INSTALL_DIR="${SPIKE_INSTALL_DIR:-/opt/riscv}"
GEM5_INSTALL_DIR="${GEM5_INSTALL_DIR:-/opt/gem5}"
INSTALL_GEM5="${INSTALL_GEM5:-0}"
INSTALL_RENODE="${INSTALL_RENODE:-0}"

# =============================================================================
# Helper Functions
# =============================================================================

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_section() {
    echo ""
    echo -e "${BLUE}==================================================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}==================================================================${NC}"
}

# =============================================================================
# QEMU Installation
# =============================================================================

install_qemu() {
    log_section "Installing QEMU"
    
    if command -v qemu-system-riscv64 &>/dev/null; then
        local version=$(qemu-system-riscv64 --version | head -1)
        log_info "QEMU already installed: $version"
        return 0
    fi
    
    log_info "Installing QEMU via apt..."
    sudo apt-get update
    sudo apt-get install -y qemu-system-misc
    
    if command -v qemu-system-riscv64 &>/dev/null; then
        log_info "QEMU installed successfully:"
        qemu-system-riscv64 --version | head -1
        return 0
    else
        log_error "QEMU installation failed"
        return 1
    fi
}

# =============================================================================
# Spike Installation
# =============================================================================

install_spike() {
    log_section "Installing Spike"
    
    if command -v spike &>/dev/null; then
        log_info "Spike already installed"
        spike --help 2>&1 | head -1 || true
        return 0
    fi
    
    log_info "Building Spike from source..."
    
    # Install dependencies
    log_info "Installing dependencies..."
    sudo apt-get install -y \
        device-tree-compiler \
        libboost-regex-dev \
        libboost-system-dev \
        build-essential \
        git
    
    # Clone repository
    local temp_dir=$(mktemp -d)
    log_info "Cloning Spike repository..."
    git clone --depth 1 https://github.com/riscv-software-src/riscv-isa-sim.git "$temp_dir"
    
    cd "$temp_dir"
    
    # Build
    log_info "Configuring Spike..."
    mkdir -p build
    cd build
    ../configure --prefix="$SPIKE_INSTALL_DIR"
    
    log_info "Building Spike (this may take a few minutes)..."
    make -j$(nproc)
    
    log_info "Installing Spike to $SPIKE_INSTALL_DIR..."
    sudo make install
    
    # Cleanup
    cd /
    rm -rf "$temp_dir"
    
    # Verify installation
    if command -v spike &>/dev/null || [ -f "$SPIKE_INSTALL_DIR/bin/spike" ]; then
        log_info "Spike installed successfully"
        if [ -f "$SPIKE_INSTALL_DIR/bin/spike" ]; then
            log_warn "Add to PATH: export PATH=\"$SPIKE_INSTALL_DIR/bin:\$PATH\""
        fi
        return 0
    else
        log_error "Spike installation failed"
        return 1
    fi
}

# =============================================================================
# gem5 Installation (Optional)
# =============================================================================

install_gem5() {
    log_section "Installing gem5"
    
    if [ -f "$GEM5_INSTALL_DIR/build/RISCV/gem5.opt" ]; then
        log_info "gem5 already installed at: $GEM5_INSTALL_DIR"
        return 0
    fi
    
    log_warn "gem5 build takes 30-60 minutes and requires ~10GB disk space"
    log_warn "Set INSTALL_GEM5=1 to proceed with installation"
    
    if [ "$INSTALL_GEM5" != "1" ]; then
        log_info "Skipping gem5 installation"
        return 0
    fi
    
    log_info "Installing gem5 dependencies..."
    sudo apt-get install -y \
        build-essential \
        git \
        m4 \
        scons \
        zlib1g \
        zlib1g-dev \
        libprotobuf-dev \
        protobuf-compiler \
        libprotoc-dev \
        libgoogle-perftools-dev \
        python3-dev \
        python3-pip \
        libboost-all-dev \
        pkg-config \
        libhdf5-dev
    
    log_info "Cloning gem5 repository..."
    sudo git clone --depth 1 https://github.com/gem5/gem5.git "$GEM5_INSTALL_DIR"
    
    cd "$GEM5_INSTALL_DIR"
    
    log_info "Building gem5 RISC-V target (this will take a while)..."
    scons build/RISCV/gem5.opt -j$(nproc)
    
    if [ -f "$GEM5_INSTALL_DIR/build/RISCV/gem5.opt" ]; then
        log_info "gem5 installed successfully at: $GEM5_INSTALL_DIR"
        return 0
    else
        log_error "gem5 build failed"
        return 1
    fi
}

# =============================================================================
# Renode Installation (Optional)
# =============================================================================

install_renode() {
    log_section "Installing Renode"
    
    if command -v renode &>/dev/null; then
        log_info "Renode already installed"
        renode --version 2>&1 | head -1 || true
        return 0
    fi
    
    log_warn "Set INSTALL_RENODE=1 to proceed with installation"
    
    if [ "$INSTALL_RENODE" != "1" ]; then
        log_info "Skipping Renode installation"
        log_info "To install manually: https://renode.io/"
        return 0
    fi
    
    log_info "Installing Renode..."
    
    # Try to install via apt (if repo is available)
    if sudo apt-get install -y renode 2>/dev/null; then
        log_info "Renode installed via apt"
        return 0
    fi
    
    # Fall back to manual instructions
    log_warn "Automatic installation not available"
    log_info "Download from: https://github.com/renode/renode/releases"
    log_info "Or follow: https://renode.readthedocs.io/en/latest/introduction/installing.html"
    
    return 0
}

# =============================================================================
# Verify Installation
# =============================================================================

verify_installation() {
    log_section "Verification"
    
    local all_good=true
    
    # Check QEMU
    if command -v qemu-system-riscv64 &>/dev/null; then
        log_info "✓ QEMU: $(qemu-system-riscv64 --version | head -1)"
    else
        log_error "✗ QEMU not found"
        all_good=false
    fi
    
    # Check Spike
    if command -v spike &>/dev/null || [ -f "$SPIKE_INSTALL_DIR/bin/spike" ]; then
        log_info "✓ Spike: installed"
    else
        log_warn "✗ Spike not found"
    fi
    
    # Check gem5
    if [ -f "$GEM5_INSTALL_DIR/build/RISCV/gem5.opt" ]; then
        log_info "✓ gem5: installed at $GEM5_INSTALL_DIR"
    else
        log_warn "✗ gem5 not installed (optional)"
    fi
    
    # Check Renode
    if command -v renode &>/dev/null; then
        log_info "✓ Renode: installed"
    else
        log_warn "✗ Renode not installed (optional)"
    fi
    
    echo ""
    if [ "$all_good" = true ]; then
        log_info "All required simulators installed successfully!"
    else
        log_warn "Some simulators are missing"
    fi
}

# =============================================================================
# Main
# =============================================================================

main() {
    log_info "==================================================================="
    log_info "RISC-V Simulators Setup"
    log_info "==================================================================="
    
    # Install simulators
    install_qemu
    install_spike
    
    # Optional simulators
    if [ "$INSTALL_GEM5" = "1" ]; then
        install_gem5
    fi
    
    if [ "$INSTALL_RENODE" = "1" ]; then
        install_renode
    fi
    
    # Verify
    verify_installation
    
    log_info ""
    log_info "Setup complete!"
    log_info ""
    log_info "Next steps:"
    log_info "  1. Configure: cmake --preset default"
    log_info "  2. Build:     cmake --build build/default"
    log_info "  3. Test:      ctest --test-dir build/default"
}

# =============================================================================
# Entry Point
# =============================================================================

main "$@"
