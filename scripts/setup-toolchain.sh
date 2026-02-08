#!/bin/bash
# =============================================================================
# RISC-V Toolchain Setup Script
# =============================================================================
# This script installs the RISC-V GCC toolchain for bare-metal development.
# Supports multiple installation methods:
#   1. Package manager (apt) - fastest
#   2. xPack prebuilt binaries - latest versions
#   3. Build from source - most control (not implemented here, see manual)
# =============================================================================

set -euo pipefail

# Configuration
INSTALL_DIR="${RISCV_TOOLCHAIN_DIR:-/opt/riscv}"
TOOLCHAIN_PREFIX="riscv64-unknown-elf-"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

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

check_toolchain() {
    if command -v ${TOOLCHAIN_PREFIX}gcc &>/dev/null; then
        local version=$(${TOOLCHAIN_PREFIX}gcc --version | head -1)
        log_info "RISC-V toolchain found: $version"
        return 0
    else
        return 1
    fi
}

# =============================================================================
# Installation Methods
# =============================================================================

install_via_apt() {
    log_info "Installing RISC-V toolchain via apt..."
    
    if ! command -v apt-get &>/dev/null; then
        log_warn "apt-get not available on this system"
        return 1
    fi
    
    sudo apt-get update
    sudo apt-get install -y \
        gcc-riscv64-unknown-elf \
        binutils-riscv64-unknown-elf \
        gdb-multiarch \
        picolibc-riscv64-unknown-elf
    
    log_info "Toolchain installed successfully via apt"
    return 0
}

install_via_xpack() {
    log_info "Installing RISC-V toolchain via xPack..."
    
    # Detect architecture
    local arch=$(uname -m)
    local os=$(uname -s | tr '[:upper:]' '[:lower:]')
    
    # xPack release URL (update version as needed)
    local version="13.2.0-2"
    local xpack_name="xpack-riscv-none-elf-gcc-${version}"
    
    case "$os-$arch" in
        linux-x86_64)
            local platform="linux-x64"
            ;;
        linux-aarch64)
            local platform="linux-arm64"
            ;;
        darwin-x86_64)
            local platform="darwin-x64"
            ;;
        darwin-arm64)
            local platform="darwin-arm64"
            ;;
        *)
            log_error "Unsupported platform: $os-$arch"
            return 1
            ;;
    esac
    
    local download_url="https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v${version}/${xpack_name}-${platform}.tar.gz"
    local temp_dir=$(mktemp -d)
    
    log_info "Downloading from: $download_url"
    
    if ! curl -L -o "${temp_dir}/toolchain.tar.gz" "$download_url"; then
        log_error "Failed to download toolchain"
        rm -rf "$temp_dir"
        return 1
    fi
    
    log_info "Extracting to: $INSTALL_DIR"
    sudo mkdir -p "$INSTALL_DIR"
    sudo tar -xzf "${temp_dir}/toolchain.tar.gz" -C "$INSTALL_DIR" --strip-components=1
    
    rm -rf "$temp_dir"
    
    log_info "Toolchain installed successfully to $INSTALL_DIR"
    log_info "Add to PATH: export PATH=\"$INSTALL_DIR/bin:\$PATH\""
    
    return 0
}

# =============================================================================
# Main Installation Logic
# =============================================================================

main() {
    log_info "==================================================================="
    log_info "RISC-V Toolchain Setup"
    log_info "==================================================================="
    
    # Check if already installed
    if check_toolchain; then
        log_info "Toolchain already installed. Nothing to do."
        exit 0
    fi
    
    log_info "RISC-V toolchain not found. Installing..."
    
    # Try apt first (fastest and most reliable)
    if install_via_apt; then
        if check_toolchain; then
            log_info "Installation successful!"
            ${TOOLCHAIN_PREFIX}gcc --version
            exit 0
        fi
    fi
    
    # Fall back to xPack
    log_warn "apt installation not available or failed. Trying xPack..."
    if install_via_xpack; then
        # Update PATH for current session
        export PATH="$INSTALL_DIR/bin:$PATH"
        
        if check_toolchain; then
            log_info "Installation successful!"
            ${TOOLCHAIN_PREFIX}gcc --version
            
            echo ""
            log_warn "Add to your shell profile (.bashrc, .zshrc, etc.):"
            echo "    export PATH=\"$INSTALL_DIR/bin:\$PATH\""
            exit 0
        fi
    fi
    
    # If we get here, all methods failed
    log_error "Failed to install RISC-V toolchain"
    log_error ""
    log_error "Manual installation options:"
    log_error "1. Build from source:"
    log_error "   git clone https://github.com/riscv-collab/riscv-gnu-toolchain"
    log_error "   cd riscv-gnu-toolchain"
    log_error "   ./configure --prefix=$INSTALL_DIR --with-arch=rv64gc --with-abi=lp64d"
    log_error "   make -j\$(nproc)"
    log_error ""
    log_error "2. Download prebuilt from SiFive:"
    log_error "   https://github.com/sifive/freedom-tools/releases"
    
    exit 1
}

# =============================================================================
# Entry Point
# =============================================================================

main "$@"
