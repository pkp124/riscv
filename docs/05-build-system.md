# Design Proposal 05: Build System and Toolchain

## 1. Overview

This document describes the build system, toolchain setup, and compilation pipeline for the RISC-V bare-metal project. The build system must support:

- Multiple target platforms (QEMU, Spike, gem5)
- Multiple configurations (single-core, SMP, RVV)
- Multiple ISA variants (rv64gc, rv64gcv, rv32imac)
- Cross-compilation from x86_64 host
- Reproducible builds in CI and devcontainer

---

## 2. Toolchain Selection

### 2.1 Primary: riscv-gnu-toolchain (GCC)

| Component | Tool | Version Target |
|-----------|------|---------------|
| C Compiler | `riscv64-unknown-elf-gcc` | GCC 13+ (for RVV intrinsics) |
| Assembler | `riscv64-unknown-elf-as` | binutils 2.40+ |
| Linker | `riscv64-unknown-elf-ld` | binutils 2.40+ |
| Objcopy | `riscv64-unknown-elf-objcopy` | binutils 2.40+ |
| Objdump | `riscv64-unknown-elf-objdump` | binutils 2.40+ |
| GDB | `riscv64-unknown-elf-gdb` | GDB 13+ |

**Installation options:**

1. **Prebuilt binaries** (fastest):
   - SiFive Freedom Tools: https://github.com/sifive/freedom-tools
   - xPack RISC-V GCC: https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack
   - Ubuntu/Debian: `apt install gcc-riscv64-unknown-elf` (may be older version)

2. **Build from source** (most control):
   ```bash
   git clone https://github.com/riscv-collab/riscv-gnu-toolchain
   cd riscv-gnu-toolchain
   ./configure --prefix=/opt/riscv --with-arch=rv64gcv --with-abi=lp64d
   make -j$(nproc)  # Bare-metal (newlib) toolchain
   ```

### 2.2 Secondary: LLVM/Clang (Optional)

For auto-vectorization comparison and alternative compilation:

```bash
clang --target=riscv64-unknown-elf -march=rv64gcv -mabi=lp64d \
    -nostdlib -ffreestanding -O2 -c source.c
```

### 2.3 Toolchain Decision

| Criterion | GCC | Clang/LLVM |
|-----------|-----|------------|
| RVV intrinsics | Good (GCC 13+) | Very good (Clang 16+) |
| Auto-vectorization | Good | Often better |
| Bare-metal support | Excellent (newlib) | Good (needs picolibc or no libc) |
| Inline asm | Full support | Full support |
| Availability | Easy (prebuilt widely available) | Moderate |
| GDB integration | Native | Via lldb or gdb |

**Decision**: Use GCC as primary toolchain. Clang as optional secondary for comparison.

---

## 3. Build System: GNU Make

### 3.1 Why Make?

- Zero additional dependencies
- Widely understood
- Sufficient for our project size
- Easy to integrate with CI
- No CMake/Meson/Ninja complexity overhead

### 3.2 Makefile Structure

```
app/
├── Makefile              # Top-level build orchestration
├── config.mk             # Toolchain and flag configuration
├── src/
├── include/
└── linker/
```

### 3.3 Top-Level Makefile Design

```makefile
# ============================================================
# RISC-V Bare-Metal Application - Top-Level Makefile
# ============================================================

# Default configuration
PLATFORM  ?= qemu
CONFIG    ?= single
NUM_HARTS ?= 1
RVV       ?= 0
VLEN      ?= 128
ARCH      ?= rv64

# Include toolchain configuration
include config.mk

# ============================================================
# Source files
# ============================================================

# Core sources (always included)
CORE_SRCS = \
    src/startup.S \
    src/main.c \
    src/platform.c \
    src/printf.c \
    src/trap.c

# Platform-specific sources
ifeq ($(PLATFORM),qemu)
    PLAT_SRCS = src/uart.c
    PLAT_DEFS = -DPLATFORM_QEMU_VIRT
    LINKER_SCRIPT = linker/qemu-virt.ld
else ifeq ($(PLATFORM),spike)
    PLAT_SRCS = src/htif.c
    PLAT_DEFS = -DPLATFORM_SPIKE
    LINKER_SCRIPT = linker/spike.ld
else ifeq ($(PLATFORM),gem5)
    PLAT_SRCS = src/uart.c
    PLAT_DEFS = -DPLATFORM_GEM5
    LINKER_SCRIPT = linker/gem5.ld
endif

# SMP sources (when CONFIG includes smp)
ifneq (,$(findstring smp,$(CONFIG)))
    SMP_SRCS = src/smp.c
    SMP_DEFS = -DENABLE_SMP -DNUM_HARTS=$(NUM_HARTS)
else
    SMP_SRCS =
    SMP_DEFS = -DNUM_HARTS=1
endif

# RVV sources (when RVV=1 or CONFIG includes rvv)
ifeq ($(RVV),1)
    RVV_SRCS = \
        src/rvv/rvv_detect.c \
        src/rvv/vec_add.c \
        src/rvv/vec_dotprod.c \
        src/rvv/vec_matmul.c \
        src/rvv/vec_memcpy.c
    RVV_DEFS = -DENABLE_RVV -DRVV_VLEN=$(VLEN)
else
    RVV_SRCS =
    RVV_DEFS =
endif

# All sources
SRCS = $(CORE_SRCS) $(PLAT_SRCS) $(SMP_SRCS) $(RVV_SRCS)

# ============================================================
# Compiler flags
# ============================================================

# ISA selection
ifeq ($(RVV),1)
    MARCH = rv64gcv
else
    MARCH = rv64gc
endif

MABI = lp64d

# Common flags
CFLAGS = \
    -march=$(MARCH) \
    -mabi=$(MABI) \
    -mcmodel=medany \
    -nostdlib \
    -nostartfiles \
    -ffreestanding \
    -O2 \
    -Wall -Wextra -Werror \
    -g \
    -Iinclude \
    $(PLAT_DEFS) $(SMP_DEFS) $(RVV_DEFS)

ASFLAGS = \
    -march=$(MARCH) \
    -mabi=$(MABI) \
    -Iinclude \
    $(PLAT_DEFS) $(SMP_DEFS) $(RVV_DEFS)

LDFLAGS = -T $(LINKER_SCRIPT) -nostdlib

# ============================================================
# Build directory
# ============================================================

BUILD_DIR = build/$(PLATFORM)-$(CONFIG)
ifeq ($(RVV),1)
    BUILD_DIR := $(BUILD_DIR)-rvv
endif

# Object files
OBJS = $(patsubst %,$(BUILD_DIR)/%.o,$(SRCS))

# Output ELF
TARGET = $(BUILD_DIR)/app.elf

# ============================================================
# Build rules
# ============================================================

.PHONY: all clean run dump

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "  LD    $@"
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.S.o: %.S
	@echo "  AS    $<"
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.c.o: %.c
	@echo "  CC    $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# Dump disassembly
dump: $(TARGET)
	$(OBJDUMP) -d -S $< > $(BUILD_DIR)/app.dump

# Clean
clean:
	rm -rf build/

# ============================================================
# Convenience targets
# ============================================================

# QEMU targets
qemu-single:
	$(MAKE) PLATFORM=qemu CONFIG=single RVV=0
qemu-smp:
	$(MAKE) PLATFORM=qemu CONFIG=smp NUM_HARTS=4 RVV=0
qemu-rvv:
	$(MAKE) PLATFORM=qemu CONFIG=single RVV=1
qemu-smp-rvv:
	$(MAKE) PLATFORM=qemu CONFIG=smp NUM_HARTS=4 RVV=1

# Spike targets
spike-single:
	$(MAKE) PLATFORM=spike CONFIG=single RVV=0
spike-smp:
	$(MAKE) PLATFORM=spike CONFIG=smp NUM_HARTS=4 RVV=0
spike-rvv:
	$(MAKE) PLATFORM=spike CONFIG=single RVV=1

# gem5 targets
gem5-single:
	$(MAKE) PLATFORM=gem5 CONFIG=single RVV=0
gem5-smp:
	$(MAKE) PLATFORM=gem5 CONFIG=smp NUM_HARTS=4 RVV=0

# Build all configurations
all-platforms: qemu-single qemu-smp qemu-rvv qemu-smp-rvv \
               spike-single spike-smp spike-rvv \
               gem5-single gem5-smp
```

### 3.4 config.mk

```makefile
# ============================================================
# Toolchain Configuration
# ============================================================

# Cross-compiler prefix
CROSS_PREFIX ?= riscv64-unknown-elf-

# Tools
CC      = $(CROSS_PREFIX)gcc
AS      = $(CROSS_PREFIX)as
LD      = $(CROSS_PREFIX)ld
OBJCOPY = $(CROSS_PREFIX)objcopy
OBJDUMP = $(CROSS_PREFIX)objdump
SIZE    = $(CROSS_PREFIX)size
GDB     = $(CROSS_PREFIX)gdb

# Verify toolchain exists
TOOLCHAIN_CHECK := $(shell which $(CC) 2>/dev/null)
ifeq ($(TOOLCHAIN_CHECK),)
    $(error "RISC-V toolchain not found. Run 'scripts/setup-toolchain.sh' first.")
endif
```

---

## 4. Run Targets

### 4.1 QEMU Run Scripts

```makefile
# In Makefile
run-qemu: $(BUILD_DIR)/app.elf
	qemu-system-riscv64 \
	    -machine virt \
	    -smp $(NUM_HARTS) \
	    -m 128M \
	    -nographic \
	    -bios none \
	    $(if $(filter 1,$(RVV)),-cpu rv64$(,)v=true$(,)vlen=$(VLEN),) \
	    -kernel $<

run-qemu-debug: $(BUILD_DIR)/app.elf
	qemu-system-riscv64 \
	    -machine virt \
	    -smp $(NUM_HARTS) \
	    -m 128M \
	    -nographic \
	    -bios none \
	    $(if $(filter 1,$(RVV)),-cpu rv64$(,)v=true$(,)vlen=$(VLEN),) \
	    -kernel $< \
	    -s -S
```

### 4.2 Spike Run Scripts

```makefile
run-spike: $(BUILD_DIR)/app.elf
	spike \
	    --isa=$(MARCH) \
	    $(if $(filter 1,$(RVV)),--varch=vlen:$(VLEN)$(,)elen:64,) \
	    -p$(NUM_HARTS) \
	    $<
```

---

## 5. Toolchain Setup Script

```bash
#!/bin/bash
# scripts/setup-toolchain.sh
# Downloads and installs the RISC-V bare-metal toolchain

set -euo pipefail

INSTALL_DIR="${RISCV_TOOLCHAIN_DIR:-/opt/riscv}"
TOOLCHAIN_VERSION="2024.04.12"  # Update as needed

echo "=== RISC-V Toolchain Setup ==="
echo "Install directory: ${INSTALL_DIR}"

# Option 1: Try apt (Ubuntu/Debian)
if command -v apt-get &>/dev/null; then
    echo "Attempting apt install..."
    sudo apt-get update
    sudo apt-get install -y \
        gcc-riscv64-unknown-elf \
        binutils-riscv64-unknown-elf \
        gdb-multiarch
    echo "Toolchain installed via apt."
    exit 0
fi

# Option 2: Download xPack prebuilt
echo "Downloading xPack RISC-V GCC..."
XPACK_URL="https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases"
# ... download and extract logic ...

echo "Done. Add ${INSTALL_DIR}/bin to your PATH."
```

---

## 6. Simulator Setup Script

```bash
#!/bin/bash
# scripts/setup-simulators.sh
# Installs QEMU, Spike, and optionally gem5

set -euo pipefail

echo "=== Simulator Setup ==="

# QEMU
install_qemu() {
    echo "--- Installing QEMU ---"
    if command -v qemu-system-riscv64 &>/dev/null; then
        echo "QEMU already installed: $(qemu-system-riscv64 --version | head -1)"
        return
    fi
    sudo apt-get update
    sudo apt-get install -y qemu-system-misc
    echo "QEMU installed: $(qemu-system-riscv64 --version | head -1)"
}

# Spike
install_spike() {
    echo "--- Installing Spike ---"
    if command -v spike &>/dev/null; then
        echo "Spike already installed."
        return
    fi
    sudo apt-get install -y device-tree-compiler libboost-regex-dev libboost-system-dev
    git clone https://github.com/riscv-software-src/riscv-isa-sim.git /tmp/spike
    cd /tmp/spike
    mkdir build && cd build
    ../configure --prefix=/opt/riscv
    make -j$(nproc)
    sudo make install
    echo "Spike installed."
}

# gem5 (optional, takes long to build)
install_gem5() {
    echo "--- Installing gem5 ---"
    if [ -f /opt/gem5/build/RISCV/gem5.opt ]; then
        echo "gem5 already installed."
        return
    fi
    sudo apt-get install -y \
        build-essential git m4 scons zlib1g zlib1g-dev \
        libprotobuf-dev protobuf-compiler libprotoc-dev \
        libgoogle-perftools-dev python3-dev python3-pip \
        libboost-all-dev pkg-config libhdf5-dev
    git clone https://github.com/gem5/gem5.git /opt/gem5
    cd /opt/gem5
    scons build/RISCV/gem5.opt -j$(nproc)
    echo "gem5 installed."
}

# Run installations
install_qemu
install_spike

# gem5 is optional (takes 30+ minutes)
if [ "${INSTALL_GEM5:-0}" = "1" ]; then
    install_gem5
fi

echo "=== Setup Complete ==="
```

---

## 7. Build Output Structure

```
build/
├── qemu-single/
│   ├── app.elf           # Final ELF binary
│   ├── app.dump          # Disassembly
│   └── src/
│       ├── startup.S.o
│       ├── main.c.o
│       └── ...
├── qemu-smp/
│   └── ...
├── qemu-single-rvv/
│   └── ...
├── qemu-smp-rvv/
│   └── ...
├── spike-single/
│   └── ...
├── spike-smp/
│   └── ...
├── spike-single-rvv/
│   └── ...
├── gem5-single/
│   └── ...
└── gem5-smp/
    └── ...
```

---

## 8. Static Analysis and Formatting

### 8.1 clang-format

```yaml
# .clang-format
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 100
AllowShortFunctionsOnASingleLine: None
AllowShortIfStatementsOnASingleLine: Never
BreakBeforeBraces: Linux
```

### 8.2 clang-tidy (optional)

```yaml
# .clang-tidy
Checks: >
    -*,
    bugprone-*,
    -bugprone-easily-swappable-parameters,
    cert-*,
    misc-*,
    readability-*,
    -readability-magic-numbers
```

### 8.3 cppcheck

```bash
cppcheck --enable=all --std=c11 --suppress=missingIncludeSystem \
    -I include/ src/
```

---

## 9. Makefile Targets Summary

| Target | Description |
|--------|-------------|
| `make` | Build default (QEMU, single-core, no RVV) |
| `make PLATFORM=X CONFIG=Y` | Build for specific platform/config |
| `make qemu-single` | Build for QEMU, 1 hart |
| `make qemu-smp` | Build for QEMU, 4 harts SMP |
| `make qemu-rvv` | Build for QEMU, 1 hart, RVV |
| `make qemu-smp-rvv` | Build for QEMU, 4 harts SMP + RVV |
| `make spike-single` | Build for Spike, 1 hart |
| `make gem5-single` | Build for gem5, 1 hart |
| `make all-platforms` | Build all configurations |
| `make run-qemu` | Build and run on QEMU |
| `make run-spike` | Build and run on Spike |
| `make dump` | Generate disassembly |
| `make clean` | Remove all build artifacts |
| `make format` | Run clang-format |
| `make lint` | Run cppcheck |

---

## 10. Open Questions

1. **Should we use a prebuilt toolchain or build from source in the Dockerfile?** Recommendation: prebuilt for CI speed, with option to build from source.
2. **Should we add a `make test` target that runs on QEMU and checks output?** Yes, essential for CI.
3. **Do we need `objcopy` to produce raw binary (`.bin`) in addition to ELF?** Some platforms may need it (e.g., FPGA loaders).
4. **Should the Makefile support parallel builds per-configuration?** Yes, each config should be independently buildable.

---

*Next: See [06-ci-cd-pipeline.md](06-ci-cd-pipeline.md) for CI/CD pipeline design.*
