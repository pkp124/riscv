# Building the RISC-V Bare-Metal Platform

This guide covers setting up the build environment and building the project using CMake.

---

## Table of Contents

- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Detailed Setup](#detailed-setup)
- [Building](#building)
- [Testing](#testing)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required Tools

- **CMake 3.20+**: Build system generator
- **RISC-V GCC Toolchain**: `riscv64-unknown-elf-gcc`
- **QEMU**: RISC-V system emulator (`qemu-system-riscv64`)
- **Git**: Version control

### Optional Tools

- **Spike**: RISC-V ISA simulator
- **gem5**: Cycle-accurate micro-architectural simulator
- **Renode**: SoC-level simulator
- **clang-format**: Code formatting
- **cppcheck**: Static analysis

### Supported Platforms

- **Linux**: Ubuntu 22.04+, Debian 11+, Fedora 36+
- **macOS**: 12+ (Monterey or later)
- **Windows**: WSL2 with Ubuntu 22.04+

---

## Quick Start

### 1. Install Dependencies

#### Ubuntu/Debian
```bash
# Install build tools and RISC-V toolchain
sudo apt-get update
sudo apt-get install -y \
    cmake \
    git \
    gcc-riscv64-unknown-elf \
    binutils-riscv64-unknown-elf \
    qemu-system-misc

# Optional: Install additional tools
sudo apt-get install -y clang-format cppcheck
```

#### Automated Setup
```bash
# Install RISC-V toolchain
./scripts/setup-toolchain.sh

# Install simulators (QEMU, Spike)
./scripts/setup-simulators.sh

# Install gem5 (optional, takes 30-60 minutes)
INSTALL_GEM5=1 ./scripts/setup-simulators.sh

# Install Renode (optional)
INSTALL_RENODE=1 ./scripts/setup-simulators.sh
```

### 2. Verify Environment

```bash
./scripts/verify-environment.sh
```

This script checks that all required tools are installed and properly configured.

### 3. Build

```bash
# Configure (using default preset: QEMU single-core)
cmake --preset default

# Build
cmake --build build/default

# Test (when app/ is implemented in Phase 2)
ctest --test-dir build/default --output-on-failure
```

---

## Detailed Setup

### Manual Toolchain Installation

#### Option 1: Package Manager (Fastest)

**Ubuntu/Debian:**
```bash
sudo apt-get install gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf
```

**Fedora:**
```bash
sudo dnf install riscv64-gnu-toolchain
```

#### Option 2: xPack Prebuilt Binaries

```bash
# Download xPack RISC-V GCC
XPACK_VERSION="13.2.0-2"
wget https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v${XPACK_VERSION}/xpack-riscv-none-elf-gcc-${XPACK_VERSION}-linux-x64.tar.gz

# Extract
sudo tar -xzf xpack-riscv-none-elf-gcc-${XPACK_VERSION}-linux-x64.tar.gz -C /opt

# Add to PATH
echo 'export PATH="/opt/xpack-riscv-none-elf-gcc-${XPACK_VERSION}/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

#### Option 3: Build from Source

```bash
# Clone riscv-gnu-toolchain
git clone https://github.com/riscv-collab/riscv-gnu-toolchain.git
cd riscv-gnu-toolchain

# Install dependencies
sudo apt-get install autoconf automake autotools-dev curl python3 \
    libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex \
    texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev

# Configure for bare-metal (newlib)
./configure --prefix=/opt/riscv --with-arch=rv64gc --with-abi=lp64d

# Build (takes 1-2 hours)
make -j$(nproc)

# Add to PATH
echo 'export PATH="/opt/riscv/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

### Simulator Installation

#### QEMU
```bash
# Ubuntu/Debian
sudo apt-get install qemu-system-misc

# Verify
qemu-system-riscv64 --version
```

#### Spike
```bash
# Install dependencies
sudo apt-get install device-tree-compiler libboost-regex-dev libboost-system-dev

# Clone and build
git clone https://github.com/riscv-software-src/riscv-isa-sim.git
cd riscv-isa-sim
mkdir build && cd build
../configure --prefix=/opt/riscv
make -j$(nproc)
sudo make install

# Add to PATH if not already
export PATH="/opt/riscv/bin:$PATH"
```

#### gem5 (Optional)
```bash
# Install dependencies
sudo apt-get install build-essential git m4 scons zlib1g zlib1g-dev \
    libprotobuf-dev protobuf-compiler libprotoc-dev \
    libgoogle-perftools-dev python3-dev libboost-all-dev \
    pkg-config libhdf5-dev

# Clone
git clone https://github.com/gem5/gem5.git /opt/gem5
cd /opt/gem5

# Build (takes 30-60 minutes)
scons build/RISCV/gem5.opt -j$(nproc)
```

#### Renode (Optional)
```bash
# Download from releases
wget https://github.com/renode/renode/releases/download/v1.14.0/renode_1.14.0_amd64.deb
sudo dpkg -i renode_1.14.0_amd64.deb

# Or follow: https://renode.readthedocs.io/en/latest/introduction/installing.html
```

---

## Building

### Using CMake Presets (Recommended)

CMake presets provide pre-configured build profiles for different platforms and configurations.

#### List Available Presets
```bash
cmake --list-presets
```

#### Configure Presets

| Preset | Description | Platform | Config |
|--------|-------------|----------|--------|
| `default` | QEMU single-core, Debug | QEMU | 1 hart |
| `qemu-smp` | QEMU 4-hart SMP | QEMU | 4 harts |
| `qemu-smp-8` | QEMU 8-hart SMP | QEMU | 8 harts |
| `qemu-rvv` | QEMU with RVV (VLEN=128) | QEMU | RVV |
| `qemu-rvv-256` | QEMU with RVV (VLEN=256) | QEMU | RVV |
| `qemu-smp-rvv` | QEMU 4-hart SMP + RVV | QEMU | SMP+RVV |
| `spike` | Spike single-core | Spike | 1 hart |
| `spike-smp` | Spike 4-hart SMP | Spike | 4 harts |
| `spike-rvv` | Spike with RVV | Spike | RVV |
| `gem5-se` | gem5 Syscall Emulation | gem5 | SE mode |
| `gem5-fs` | gem5 Full System | gem5 | FS mode |
| `gem5-fs-smp` | gem5 FS 4-hart SMP | gem5 | FS+SMP |
| `renode` | Renode single-core | Renode | 1 hart |
| `renode-smp` | Renode 4-hart SMP | Renode | SMP |
| `release` | Optimized release build | QEMU | Release |
| `minsize` | Size-optimized build | QEMU | MinSize |

#### Build Examples

```bash
# QEMU single-core (default)
cmake --preset default
cmake --build build/default

# QEMU 4-hart SMP
cmake --preset qemu-smp
cmake --build build/qemu-smp

# QEMU with RVV (VLEN=256)
cmake --preset qemu-rvv-256
cmake --build build/qemu-rvv-256

# Spike ISA simulator
cmake --preset spike
cmake --build build/spike

# gem5 Full System mode
cmake --preset gem5-fs
cmake --build build/gem5-fs

# Renode
cmake --preset renode
cmake --build build/renode

# Release build (optimized)
cmake --preset release
cmake --build build/release
```

### Manual CMake Configuration

If you need custom configuration not covered by presets:

```bash
cmake -B build/custom \
    -DPLATFORM=qemu \
    -DCONFIG=smp \
    -DNUM_HARTS=8 \
    -DENABLE_RVV=ON \
    -DVLEN=512 \
    -DCMAKE_BUILD_TYPE=Debug

cmake --build build/custom
```

#### CMake Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `PLATFORM` | qemu, spike, gem5, renode | qemu | Target platform |
| `CONFIG` | single, smp, amp | single | Configuration type |
| `NUM_HARTS` | 1, 2, 4, 8 | 1 | Number of harts |
| `ENABLE_RVV` | ON, OFF | OFF | Enable RVV extension |
| `VLEN` | 128, 256, 512, 1024 | 128 | Vector length (bits) |
| `GEM5_MODE` | se, fs | fs | gem5 mode |
| `CMAKE_BUILD_TYPE` | Debug, Release, RelWithDebInfo, MinSizeRel | Debug | Build type |

---

## Testing

### Run Tests with CTest

```bash
# Run all tests (when implemented in Phase 2+)
ctest --test-dir build/default --output-on-failure

# Run with verbose output
ctest --test-dir build/default -V

# Run specific test
ctest --test-dir build/default -R phase2_uart_hello

# Run tests by label
ctest --test-dir build/default -L smoke  # Only smoke tests
ctest --test-dir build/default -L qemu   # Only QEMU tests

# Run tests in parallel
ctest --test-dir build/default -j$(nproc)
```

### Test Presets

```bash
# Use test presets
ctest --preset default       # Default configuration
ctest --preset smoke         # Quick smoke tests
ctest --preset all           # All tests, verbose
```

### Manual Simulation

#### QEMU
```bash
# Single-core
qemu-system-riscv64 -machine virt -nographic -bios none \
    -kernel build/default/bin/app

# SMP (4 harts)
qemu-system-riscv64 -machine virt -smp 4 -nographic -bios none \
    -kernel build/qemu-smp/bin/app

# With RVV (VLEN=256)
qemu-system-riscv64 -machine virt -cpu rv64,v=true,vlen=256 \
    -nographic -bios none -kernel build/qemu-rvv-256/bin/app

# With GDB debugging
qemu-system-riscv64 -machine virt -nographic -bios none \
    -kernel build/default/bin/app -s -S
# Then: riscv64-unknown-elf-gdb build/default/bin/app
#       (gdb) target remote :1234
#       (gdb) break main
#       (gdb) continue
```

#### Spike
```bash
# Single-core
spike --isa=rv64gc build/spike/bin/app

# SMP (4 harts)
spike --isa=rv64gc -p4 build/spike-smp/bin/app

# With RVV
spike --isa=rv64gcv --varch=vlen:256,elen:64 build/spike-rvv/bin/app

# Interactive debug
spike -d --isa=rv64gc build/spike/bin/app
# Spike commands:
#   : until pc 0 0x80000000   # Run until address
#   : reg 0 a0                # Read register
#   : mem 0x80000000 8        # Read memory
```

#### gem5
```bash
# Syscall Emulation mode
/opt/gem5/build/RISCV/gem5.opt \
    platforms/gem5/configs/se_config.py \
    --cmd=build/gem5-se/bin/app

# Full System mode
/opt/gem5/build/RISCV/gem5.opt \
    platforms/gem5/configs/fs_config.py \
    --cmd=build/gem5-fs/bin/app
```

#### Renode
```bash
renode -e "s @platforms/renode/configs/run.resc"
```

---

## Code Quality

### Format Code
```bash
# Format all C source files
cmake --build build/default --target format

# Or manually:
find app -name '*.c' -o -name '*.h' | xargs clang-format -i
```

### Static Analysis
```bash
# Run static analysis
cmake --build build/default --target lint

# Or manually:
cppcheck --enable=all --std=c11 \
    --suppress=missingIncludeSystem \
    -I app/include/ app/src/
```

---

## Troubleshooting

### Common Issues

#### Issue: `RISC-V toolchain not found`

**Solution:**
```bash
# Install toolchain
./scripts/setup-toolchain.sh

# Or manually via apt
sudo apt-get install gcc-riscv64-unknown-elf

# Verify installation
riscv64-unknown-elf-gcc --version
```

#### Issue: `QEMU not found`

**Solution:**
```bash
# Install QEMU
sudo apt-get install qemu-system-misc

# Verify
qemu-system-riscv64 --version
```

#### Issue: `CMake version too old`

**Solution:**
```bash
# Ubuntu 20.04 has old CMake, upgrade via Kitware APT repo
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt-get update
sudo apt-get install cmake

# Or download from cmake.org
```

#### Issue: `Spike not found`

**Solution:**
```bash
# Build Spike
./scripts/setup-simulators.sh

# Or manually:
git clone https://github.com/riscv-software-src/riscv-isa-sim.git
cd riscv-isa-sim && mkdir build && cd build
../configure --prefix=/opt/riscv
make -j$(nproc) && sudo make install
```

#### Issue: Build fails with linker errors

**Cause:** No application code yet (Phase 2 not implemented).

**Solution:**
This is expected in Phase 1. The build system is ready, but the application
source code will be added in Phase 2.

### Clean Build

```bash
# Remove all build directories
rm -rf build/

# Or clean specific preset
cmake --build build/default --target clean
```

### Verbose Build

```bash
# See all compiler commands
cmake --build build/default --verbose

# Or with make
cmake --build build/default -- VERBOSE=1
```

---

## Development Workflow

### Typical Development Cycle

```bash
# 1. Configure
cmake --preset default

# 2. Build
cmake --build build/default

# 3. Test
ctest --test-dir build/default --output-on-failure

# 4. Run manually (when implemented)
qemu-system-riscv64 -M virt -nographic -bios none \
    -kernel build/default/bin/app

# 5. Debug (if needed)
qemu-system-riscv64 -M virt -nographic -bios none \
    -kernel build/default/bin/app -s -S &
riscv64-unknown-elf-gdb build/default/bin/app
```

### Switching Platforms

```bash
# Build for multiple platforms
cmake --preset default && cmake --build build/default
cmake --preset spike && cmake --build build/spike
cmake --preset gem5-fs && cmake --build build/gem5-fs

# Run tests on all
ctest --test-dir build/default
ctest --test-dir build/spike
ctest --test-dir build/gem5-fs
```

---

## Next Steps

- **Phase 2**: Implement bare-metal application (app/src/)
- **Phase 3**: Add Spike platform support
- **Phase 4**: Implement SMP support
- **Phase 5**: Add RVV workloads
- **Phase 6**: Integrate gem5 SE and FS modes
- **Phase 7**: Add Renode support

See [ROADMAP.md](ROADMAP.md) for detailed implementation plan.

---

## References

- [CMake Documentation](https://cmake.org/documentation/)
- [CTest Documentation](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- [RISC-V Toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain)
- [QEMU RISC-V](https://www.qemu.org/docs/master/system/target-riscv.html)
- [Spike](https://github.com/riscv-software-src/riscv-isa-sim)
- [gem5](https://www.gem5.org/)
- [Renode](https://renode.io/)

---

**Last Updated:** 2026-02-08  
**Phase:** Phase 1 - Foundation & Build System
