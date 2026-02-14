# =============================================================================
# RISC-V Bare-Metal Development Environment
# =============================================================================
# This Dockerfile creates a comprehensive development environment for
# RISC-V bare-metal development, including:
# - RISC-V GCC cross-compiler toolchain (with RVV support)
# - QEMU system emulator (RISC-V)
# - Spike ISA simulator
# - gem5 (RISC-V simulator, ~30-60 min build)
# - Renode (from project .deb package)
# - Static analysis tools
# - GDB for debugging
# =============================================================================

FROM ubuntu:24.04

LABEL maintainer="RISC-V System Simulation Project"
LABEL description="Development environment for RISC-V bare-metal simulation"

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# =============================================================================
# Stage 1: System packages
# =============================================================================
RUN apt-get update && apt-get install -y --no-install-recommends \
    # Build essentials
    build-essential \
    gcc \
    g++ \
    make \
    cmake \
    autoconf \
    automake \
    libtool \
    pkg-config \
    # RISC-V cross-compiler
    gcc-riscv64-unknown-elf \
    binutils-riscv64-unknown-elf \
    # QEMU
    qemu-system-misc \
    # Debugging
    gdb-multiarch \
    # Static analysis & formatting
    cppcheck \
    clang-format \
    clang-tidy \
    # Spike build dependencies
    device-tree-compiler \
    libboost-regex-dev \
    libboost-system-dev \
    # gem5 build dependencies
    m4 \
    scons \
    libprotobuf-dev \
    protobuf-compiler \
    libprotoc-dev \
    libgoogle-perftools-dev \
    libboost-all-dev \
    libhdf5-dev \
    # General utilities
    git \
    curl \
    wget \
    vim \
    nano \
    less \
    htop \
    tree \
    file \
    # Python (for test scripts and gem5)
    python3 \
    python3-pip \
    python3-venv \
    # Documentation
    doxygen \
    # Libraries
    zlib1g-dev \
    libglib2.0-dev \
    libpixman-1-dev \
    libfdt-dev \
    # Cleanup
    && rm -rf /var/lib/apt/lists/*

# =============================================================================
# Stage 2: Build Spike (RISC-V ISA Simulator) from source
# =============================================================================
# Spike is the official RISC-V reference simulator and not available via apt.
# We build from source to get the latest RVV support.

ARG SPIKE_COMMIT=master

RUN git clone https://github.com/riscv-software-src/riscv-isa-sim.git /tmp/spike && \
    cd /tmp/spike && \
    git checkout ${SPIKE_COMMIT} && \
    mkdir build && cd build && \
    ../configure --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/spike

# =============================================================================
# Stage 3: Build gem5 (RISC-V simulator)
# =============================================================================
# gem5 is used for micro-architectural analysis. Build takes ~30-60 min.
ARG GEM5_BRANCH=stable

RUN git clone --depth 1 --branch ${GEM5_BRANCH} https://github.com/gem5/gem5.git /opt/gem5 && \
    cd /opt/gem5 && \
    scons build/RISCV/gem5.opt -j$(nproc) && \
    echo "gem5 build complete"

# =============================================================================
# Stage 4: Install Renode (from project .deb package)
# =============================================================================
# Renode requires Mono 5.20+. Use ARG to easily update version.
ARG RENODE_VERSION=1.16.0

RUN apt-get update && apt-get install -y --no-install-recommends mono-complete && \
    wget -q "https://github.com/renode/renode/releases/download/v${RENODE_VERSION}/renode_${RENODE_VERSION}_amd64.deb" -O /tmp/renode.deb && \
    apt-get install -y /tmp/renode.deb && \
    rm /tmp/renode.deb && \
    rm -rf /var/lib/apt/lists/*

# =============================================================================
# Stage 5: Verify installations
# =============================================================================
RUN echo "=== Toolchain Versions ===" && \
    riscv64-unknown-elf-gcc --version | head -1 && \
    riscv64-unknown-elf-objdump --version | head -1 && \
    qemu-system-riscv64 --version | head -1 && \
    spike --help 2>&1 | head -1 || true && \
    /opt/gem5/build/RISCV/gem5.opt --version 2>&1 | head -1 || true && \
    renode --version | head -1 || true && \
    gdb-multiarch --version | head -1 && \
    cppcheck --version && \
    clang-format --version && \
    echo "=== All tools verified ==="

# =============================================================================
# Stage 6: Environment setup
# =============================================================================

# Set PATH to include RISC-V tools
ENV PATH="/usr/local/bin:${PATH}"

# Create workspace
WORKDIR /workspace

# Default shell
SHELL ["/bin/bash", "-c"]

# Default command
CMD ["/bin/bash"]
