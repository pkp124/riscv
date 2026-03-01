# Claude AI Assistant - Project Context

## Project Overview

This is a **RISC-V bare-metal system simulation platform** designed for learning and experimentation with RISC-V ISA, multi-core configurations, vector extensions (RVV), and various simulation platforms.

**Key Characteristics:**
- **Bare-metal C/Assembly**: No OS, no RTOS, direct hardware interaction
- **Multi-platform**: QEMU, Spike, gem5 (SE + FS), Renode
- **Progressive complexity**: Single-core → SMP → RVV → AMP
- **Test-Driven Development**: Write tests first, then implement
- **CMake + CTest**: Modern build system with comprehensive testing

---

## Current Project Status

**Completed:** Phase 0-7 (Design, Build System, QEMU, Spike, SMP, RVV, gem5, Renode)  
**Current Phase:** Phase 8 (AMP) - Next  
**Last Updated:** 2026-02-24  

### What Exists
✅ Comprehensive design documents (docs/00-06)  
✅ Devcontainer configuration (.devcontainer/)  
✅ Full CI pipeline (lint, build matrix, QEMU + Spike simulations, cross-validation)  
✅ CMake build system with 20+ presets (CMakeLists.txt, CMakePresets.json)  
✅ RISC-V toolchain file (cmake/toolchain/riscv64-elf.cmake)  
✅ CTest: 7 QEMU Phase 2 + 7 QEMU Phase 4 + 10 QEMU Phase 5 + 8 Spike Phase 3 + 5 Spike Phase 4 + 9 Spike Phase 5 + 14 gem5 Phase 6 + 5 Renode Phase 7 tests  
✅ Application source (startup.S, main.c, uart.c, htif.c, gem5_se_io.c, platform.c, smp.c)  
✅ Platform headers (platform.h, csr.h, uart.h, htif.h, gem5_se_io.h, console.h, smp.h, atomic.h)  
✅ RVV infrastructure (rvv/rvv_detect.h, rvv/rvv_common.h)  
✅ RVV workloads (vec_add, vec_memcpy, vec_dotprod, vec_saxpy, vec_matmul)  
✅ Linker scripts (qemu-virt.ld, spike.ld, gem5.ld) with SMP stack allocation  
✅ Setup scripts (setup-toolchain.sh, setup-simulators.sh, verify-environment.sh)  
✅ Cross-platform validation (QEMU vs Spike output functionally identical)  
✅ SMP support: spinlocks, barriers, atomic ops, multi-hart boot (2-8 harts)  
✅ RVV support: 7 workloads, VLEN-agnostic, inline asm, scalar verification  
✅ gem5 platform support: SE mode (syscall I/O), FS mode (UART + m5ops exit)  
✅ gem5 Python configs: fs_config.py (4 CPU models), se_config.py  
✅ gem5 performance analysis: parse-gem5-stats.py (JSON/CSV/comparison)  
✅ gem5 simulations in ci-build.yml (unified workflow)  

### What Doesn't Exist Yet
❌ AMP configurations (Phase 8)  
❌ Advanced testing & validation (Phase 9)

---

## Development Philosophy

### 1. Test-Driven Development (TDD)
**Always follow the Red-Green-Refactor cycle:**

1. **RED**: Write a failing test first
   ```cmake
   add_test(NAME my_feature_test COMMAND ...)
   set_tests_properties(my_feature_test PROPERTIES
     PASS_REGULAR_EXPRESSION "Expected output"
   )
   ```

2. **GREEN**: Write minimal code to pass the test
   ```c
   // Implement just enough to make test pass
   void my_feature() { /* ... */ }
   ```

3. **REFACTOR**: Clean up code while keeping tests passing
   - Extract functions
   - Improve naming
   - Add documentation
   - Optimize

**Never write implementation code before writing its test.**

### 2. Progressive Complexity
Start simple, add complexity incrementally:
- ✅ Single-core before multi-core
- ✅ QEMU (fast) before gem5 (slow)
- ✅ Functional before performance analysis
- ✅ Basic I/O before advanced features

### 3. Cross-Platform Validation
Every feature must work on multiple platforms:
- Primary: QEMU (fast iteration)
- Reference: Spike (ISA correctness)
- Analysis: gem5 (performance)
- Extended: Renode (SoC modeling)

### 4. Bare-Metal Constraints
Remember these are bare-metal applications:
- **No stdlib**: No malloc, printf, file I/O (implement your own)
- **No OS syscalls**: Direct hardware access only
- **No dynamic linking**: Statically linked ELF binaries
- **Fixed memory layout**: Defined by linker scripts
- **Manual initialization**: Clear BSS, set up stack, configure peripherals

---

## Architecture & Design Decisions

### Build System: CMake + CTest
- **CMake 3.20+**: Modern build system
- **CMakePresets.json**: Pre-configured build profiles
- **CTest**: Test orchestration and validation
- **Cross-compilation**: RISC-V GCC toolchain via toolchain files

### Toolchain
- **Compiler**: `riscv64-unknown-elf-gcc` (GCC 13+)
- **Linker**: `riscv64-unknown-elf-ld`
- **Target ISA**: `rv64gc` (default), `rv64gcv` (with RVV)
- **ABI**: `lp64d` (64-bit with double-precision float)

### Platform Abstraction
Use compile-time platform selection via CMake:
```c
#if defined(PLATFORM_QEMU_VIRT)
    #include "uart.h"  // NS16550A UART
#elif defined(PLATFORM_SPIKE)
    #include "htif.h"  // HTIF for Spike
#elif defined(PLATFORM_GEM5)
    #include "uart.h"  // gem5 UART
#elif defined(PLATFORM_RENODE)
    #include "uart.h"  // Renode UART
#endif
```

### Memory Maps
Common DRAM base: `0x8000_0000` (2 GiB mark)

| Platform | RAM Base | UART/IO Base | Notes |
|----------|----------|--------------|-------|
| QEMU virt | 0x80000000 | 0x10000000 (NS16550A) | Most compatible |
| Spike | 0x80000000 | HTIF (not MMIO) | Uses tohost/fromhost |
| gem5 FS | 0x80000000 | 0x10000000 | Configurable via Python |
| Renode | 0x80000000 | Configurable | Platform-dependent |

### Directory Structure
```
.
├── CMakeLists.txt              # Root CMake
├── CMakePresets.json           # Build presets
├── ROADMAP.md                  # Project roadmap (this is the source of truth)
├── cmake/                      # CMake modules
│   ├── toolchain/
│   │   └── riscv64-elf.cmake  # Toolchain file
│   └── platforms/             # Platform-specific configs
├── app/                        # Application source
│   ├── CMakeLists.txt
│   ├── src/                   # C/Assembly source
│   │   ├── startup.S          # Boot code
│   │   ├── main.c             # Entry point
│   │   ├── uart.c             # UART driver (QEMU/gem5)
│   │   ├── htif.c             # HTIF driver (Spike)
│   │   ├── smp.c              # SMP support
│   │   └── rvv/               # RVV workloads (Phase 5)
│   │       ├── rvv_detect.c   # RVV capability detection
│   │       ├── vec_add.c      # Integer & float vector add
│   │       ├── vec_memcpy.c   # Vectorized memory copy
│   │       ├── vec_dotprod.c  # Dot product with reduction
│   │       ├── vec_saxpy.c    # SAXPY (y = a*x + y)
│   │       └── vec_matmul.c   # Matrix multiplication
│   ├── include/               # Headers
│   └── linker/                # Linker scripts
│       ├── qemu-virt.ld
│       ├── spike.ld
│       ├── gem5.ld
│       └── renode.ld
├── tests/                      # CTest test definitions
│   ├── CMakeLists.txt
│   ├── integration/           # Integration tests
│   └── utils/                 # Test helpers
├── platforms/                  # Platform launch configs
│   ├── qemu/
│   ├── spike/
│   ├── gem5/
│   │   └── configs/           # gem5 Python configs
│   └── renode/
│       └── configs/           # Renode .repl and .resc files
└── scripts/                    # Setup and helper scripts
    ├── setup-toolchain.sh
    ├── setup-simulators.sh
    └── verify-environment.sh
```

---

## Working with This Project

### When Starting a New Feature

1. **Check ROADMAP.md** for the current phase and tasks
2. **Read relevant design docs** in `docs/` directory
3. **Write tests first** (TDD approach)
4. **Implement feature** to pass tests
5. **Validate on all platforms** (QEMU, Spike, etc.)
6. **Update CI** to include new tests
7. **Update documentation** if needed

### Example: Adding UART Driver (Phase 2)

#### Step 1: Write Test First
```cmake
# tests/integration/CMakeLists.txt
add_test(
  NAME uart_hello_world
  COMMAND qemu-system-riscv64 -M virt -nographic -bios none 
          -kernel $<TARGET_FILE:app>
)
set_tests_properties(uart_hello_world PROPERTIES
  PASS_REGULAR_EXPRESSION "Hello RISC-V"
  TIMEOUT 30
)
```

#### Step 2: Run Test (Expect Failure)
```bash
cmake --preset default
cmake --build build/default
ctest --test-dir build/default --output-on-failure
# Expected: FAIL (no uart.c implementation yet)
```

#### Step 3: Implement UART Driver
```c
// app/src/uart.c
#include "uart.h"
#include <stdint.h>

#define UART_BASE 0x10000000
#define UART_THR  (*(volatile uint8_t*)(UART_BASE + 0))

void uart_putc(char c) {
    UART_THR = c;
}

void uart_puts(const char* s) {
    while (*s) uart_putc(*s++);
}
```

```c
// app/src/main.c
#include "uart.h"

int main() {
    uart_puts("Hello RISC-V\n");
    return 0;
}
```

#### Step 4: Rebuild and Verify
```bash
cmake --build build/default
ctest --test-dir build/default --output-on-failure
# Expected: PASS
```

#### Step 5: Cross-Validate on Spike
```bash
cmake --preset spike
cmake --build build/spike
ctest --test-dir build/spike --output-on-failure
# Expected: FAIL (needs HTIF implementation for Spike)
```

#### Step 6: Add Platform Abstraction
```c
// app/include/platform.h
#if defined(PLATFORM_QEMU_VIRT) || defined(PLATFORM_GEM5)
    void uart_puts(const char* s);
#elif defined(PLATFORM_SPIKE)
    void htif_puts(const char* s);
    #define uart_puts htif_puts
#endif
```

#### Step 7: Implement HTIF for Spike
```c
// app/src/htif.c (Spike-specific)
void htif_puts(const char* s) {
    // Implement HTIF tohost/fromhost mechanism
}
```

#### Step 8: Verify All Platforms
```bash
ctest --preset default  # QEMU
ctest --preset spike    # Spike
# Both should PASS
```

---

## Common Tasks & Commands

### Build Commands
```bash
# Configure (using preset)
cmake --preset default

# Build
cmake --build build/default

# Build specific target
cmake --build build/default --target app

# Clean
cmake --build build/default --target clean

# Or clean entire build directory
rm -rf build/
```

### Test Commands
```bash
# Run all tests
ctest --test-dir build/default

# Run with output on failure
ctest --test-dir build/default --output-on-failure

# Run specific test
ctest --test-dir build/default -R uart_hello_world

# Verbose output
ctest --test-dir build/default -V

# List all tests
ctest --test-dir build/default -N
```

### Platform-Specific Builds
```bash
# QEMU single-core
cmake --preset default && cmake --build build/default

# QEMU SMP (4 harts)
cmake --preset qemu-smp && cmake --build build/qemu-smp

# QEMU with RVV
cmake --preset qemu-rvv && cmake --build build/qemu-rvv

# Spike
cmake --preset spike && cmake --build build/spike

# gem5 SE mode
cmake --preset gem5-se && cmake --build build/gem5-se

# gem5 FS mode
cmake --preset gem5-fs && cmake --build build/gem5-fs

# Renode
cmake --preset renode && cmake --build build/renode
```

### Run Simulations Manually
```bash
# QEMU
qemu-system-riscv64 -M virt -nographic -bios none \
  -kernel build/default/app/app.elf

# Spike
spike --isa=rv64gc build/spike/app/app.elf

# gem5 (after building gem5)
/opt/gem5/build/RISCV/gem5.opt \
  platforms/gem5/configs/se_config.py \
  --cmd=build/gem5-se/app/app.elf

# Renode
renode -e "s @platforms/renode/configs/riscv_virt.resc"
```

---

## Important Coding Guidelines

### Bare-Metal C Specifics

#### 1. No Standard Library
```c
// ❌ DON'T:
#include <stdio.h>
printf("Hello\n");

// ✅ DO:
#include "uart.h"
uart_puts("Hello\n");
```

#### 2. Implement Your Own Utilities
```c
// app/src/string.c
void* memset(void* s, int c, size_t n) {
    unsigned char* p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = dest;
    const unsigned char* s = src;
    while (n--) *d++ = *s++;
    return dest;
}
```

#### 3. Fixed-Width Types
```c
// ✅ Always use fixed-width types
#include <stdint.h>
uint32_t mhartid;
uint64_t address = 0x80000000ULL;
```

#### 4. Volatile for MMIO
```c
// ✅ Use volatile for memory-mapped I/O
#define UART_BASE 0x10000000
#define UART_THR (*(volatile uint8_t*)(UART_BASE + 0))

void uart_putc(char c) {
    UART_THR = c;  // Direct hardware write
}
```

#### 5. CSR Access via Inline Assembly
```c
// app/include/csr.h
#define read_csr(reg) ({ \
    unsigned long __tmp; \
    asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
    __tmp; })

#define write_csr(reg, val) ({ \
    asm volatile ("csrw " #reg ", %0" :: "r"(val)); })

// Usage:
uint64_t hartid = read_csr(mhartid);
write_csr(mstatus, 0x1800);  // Set MPP=M-mode
```

### Assembly Coding

#### Startup Code Structure
```assembly
# app/src/startup.S
.section .text.start
.global _start
_start:
    # 1. Clear BSS section
    la   t0, __bss_start
    la   t1, __bss_end
1:  bge  t0, t1, 2f
    sd   zero, 0(t0)
    addi t0, t0, 8
    j    1b
2:
    # 2. Set up stack pointer (per-hart if SMP)
    la   sp, __stack_top
    
    # 3. Jump to C main
    call main
    
    # 4. Exit (simulation-specific)
    j    .  # Infinite loop or HTIF exit
```

### Multi-Hart (SMP) Considerations
```c
// Primary hart (hart 0) initializes system
// Secondary harts (1, 2, ...) spin until released

void smp_boot() {
    uint64_t hartid = read_csr(mhartid);
    
    if (hartid == 0) {
        // Primary hart
        bss_clear();
        platform_init();
        release_secondary_harts();
        main();
    } else {
        // Secondary harts
        wait_for_release(hartid);
        secondary_hart_main(hartid);
    }
}
```

---

## Testing Strategy

### Test Categories

#### 1. Smoke Tests (Phase 2-3)
Verify basic functionality:
- Boot and print "Hello"
- CSR read/write
- Memory access

#### 2. Functional Tests (Phase 2-4)
Validate features work correctly:
- UART output matches expected
- SMP harts boot successfully
- Atomic operations work

#### 3. Cross-Platform Tests (Phase 3+)
Ensure functional equivalence:
- QEMU output == Spike output
- All platforms pass same tests

#### 4. Performance Tests (Phase 6+)
gem5-specific:
- Cycle count validation
- Cache hit/miss rates
- IPC (Instructions Per Cycle)

#### 5. Regression Tests (Phase 9)
Prevent regressions:
- Known-good outputs
- Bug fix validation

### Test Writing Best Practices

```cmake
# Good test: Clear name, expected output, timeout
add_test(
  NAME smp_4hart_boot
  COMMAND qemu-system-riscv64 -M virt -smp 4 -nographic 
          -bios none -kernel $<TARGET_FILE:app>
)
set_tests_properties(smp_4hart_boot PROPERTIES
  PASS_REGULAR_EXPRESSION "Hart 0 online.*Hart 1 online.*Hart 2 online.*Hart 3 online"
  TIMEOUT 60
  LABELS "smp;qemu"
)

# Multiple checks in one test
set_tests_properties(smp_4hart_boot PROPERTIES
  PASS_REGULAR_EXPRESSION "All 4 harts online"
  FAIL_REGULAR_EXPRESSION "ERROR|FAIL|panic"
)
```

---

## gem5 Specifics (Phase 6 Implementation)

### gem5 Modes

#### SE (Syscall Emulation) Mode
- **Simpler**: No full system boot
- **Faster**: Skips bootloader/firmware
- **Use case**: Quick functional testing
- **I/O**: Via ecall syscalls (write=64, exit=93) implemented in `gem5_se_io.c`
- **Config**: `platforms/gem5/configs/se_config.py`
- **Preset**: `gem5-se`

#### FS (Full System) Mode
- **Realistic**: Full system with devices (UART, CLINT)
- **Slower**: Complete bare-metal boot sequence
- **Use case**: Bare-metal firmware, cycle-accurate analysis
- **I/O**: NS16550A UART at 0x10000000 (same driver as QEMU)
- **Exit**: m5ops pseudo-instruction (`gem5_m5_exit()` in platform.h)
- **Config**: `platforms/gem5/configs/fs_config.py`
- **Presets**: `gem5-fs`, `gem5-fs-timing`, `gem5-fs-minor`, `gem5-fs-o3`, `gem5-fs-smp`

**For bare-metal, we primarily use FS mode. SE mode is a simpler alternative.**

### gem5 CPU Models
| Model | Preset | Speed | Use Case |
|-------|--------|-------|----------|
| AtomicSimpleCPU | `gem5-fs` | Fast | Quick functional testing |
| TimingSimpleCPU | `gem5-fs-timing` | Medium | Memory system timing |
| MinorCPU | `gem5-fs-minor` | Slow | In-order pipeline analysis |
| DerivO3CPU | `gem5-fs-o3` | Very slow | Out-of-order analysis |

**Start with AtomicSimpleCPU for functional tests, use TimingSimpleCPU/MinorCPU for performance.**

### gem5 m5ops (Pseudo-Instructions)
For RISC-V, gem5 recognizes custom instruction encodings as m5ops:
```c
// In platform.h (available when PLATFORM_GEM5 is defined):
gem5_m5_exit(delay);        // Exit simulation
gem5_m5_dump_stats(d, p);   // Dump performance stats
gem5_m5_reset_stats(d, p);  // Reset stats counters
```

### gem5 Performance Analysis
```bash
# Parse stats after simulation
python3 scripts/parse-gem5-stats.py m5out/stats.txt

# Compare two CPU models
python3 scripts/parse-gem5-stats.py --compare \
  m5out/atomic/stats.txt m5out/timing/stats.txt

# JSON output for automation
python3 scripts/parse-gem5-stats.py --json m5out/stats.txt
```

---

## Renode Specifics

### Platform Description (.repl files)
Renode uses `.repl` (REnode Platform) files to describe hardware:

```
# platforms/renode/configs/riscv_virt.repl
cpu: CPU.RiscV64 @ sysbus
    cpuType: "rv64gc"
    hartId: 0

ram: Memory.MappedMemory @ sysbus 0x80000000
    size: 0x8000000  # 128 MB

uart: UART.NS16550 @ sysbus 0x10000000
    IRQ -> plic@10
```

### Renode Scripts (.resc files)
```
# platforms/renode/configs/run.resc
using sysbus
mach create
machine LoadPlatformDescription @platforms/renode/configs/riscv_virt.repl
sysbus LoadELF @build/renode/app/app.elf
start
```

---

## Debugging Tips

### QEMU Debugging
```bash
# Start QEMU with GDB server
qemu-system-riscv64 -M virt -nographic -bios none \
  -kernel app.elf -s -S

# In another terminal:
riscv64-unknown-elf-gdb app.elf
(gdb) target remote :1234
(gdb) break main
(gdb) continue
```

### Spike Debugging
```bash
# Interactive debug mode
spike -d --isa=rv64gc app.elf

# Spike commands:
: until pc 0 0x80000000  # Run until PC reaches address
: reg 0 a0               # Read register a0 on hart 0
: mem 0x80000000 8       # Read 8 bytes from address
```

### gem5 Debugging
```bash
# Enable debug flags
/opt/gem5/build/RISCV/gem5.opt \
  --debug-flags=Exec,IntRegs \
  platforms/gem5/configs/se_config.py \
  --cmd=app.elf

# Common debug flags:
# - Exec: Instruction execution trace
# - IntRegs: Register file access
# - Cache: Cache access trace
# - DRAM: Memory access
```

### Disassembly
```bash
# Generate disassembly
riscv64-unknown-elf-objdump -d -S app.elf > app.dump

# View specific sections
riscv64-unknown-elf-objdump -d -j .text app.elf
```

---

## Common Pitfalls & Solutions

### Pitfall 1: Missing BSS Clearing
**Problem:** Uninitialized global variables have garbage values.
**Solution:** Clear BSS in startup.S before calling main().

### Pitfall 2: Stack Overflow
**Problem:** Deep call stack or large local arrays overflow stack.
**Solution:** Allocate larger stack in linker script, use heap/static instead.

### Pitfall 3: Misaligned Access
**Problem:** RISC-V requires natural alignment (4-byte for uint32_t, etc.).
**Solution:** Use `__attribute__((aligned(X)))` or check pointer alignment.

### Pitfall 4: Cache Coherence (SMP)
**Problem:** Hart 0 writes, Hart 1 reads stale data.
**Solution:** Use FENCE instructions for memory ordering.

### Pitfall 5: Platform-Specific UART Addresses
**Problem:** Code works on QEMU but not Spike.
**Solution:** Use platform abstraction layer with compile-time selection.

---

## Communication & Output

### Preferred Output Format for Tests
Use structured output for easy parsing:

```c
// Good: Structured output
uart_puts("[INFO] Booting hart 0\n");
uart_puts("[TEST] UART write... PASS\n");
uart_puts("[TEST] CSR read... PASS\n");
uart_puts("[RESULT] All tests complete: 2/2 PASS\n");

// Parsing in CTest:
// PASS_REGULAR_EXPRESSION: "\\[RESULT\\].*2/2 PASS"
```

### Exit Codes (for gem5 SE mode)
```c
#ifdef PLATFORM_GEM5_SE
    #include <unistd.h>  // exit() syscall
    exit(0);  // Success
    exit(1);  // Failure
#else
    // Bare-metal: infinite loop or HTIF exit
    while (1) { asm volatile ("wfi"); }
#endif
```

---

## Asking for Help

### When Stuck
If you encounter issues:
1. **Check ROADMAP.md** - Are you on the right phase?
2. **Read design docs** - `docs/0X-*.md` have detailed specs
3. **Check CI logs** - GitHub Actions show what's failing
4. **Validate toolchain** - Run `scripts/verify-environment.sh`
5. **Isolate the problem** - Test on QEMU first (fastest debug cycle)

### Information to Provide
When reporting issues or asking questions:
- Current phase and task (from ROADMAP.md)
- Platform being tested (QEMU, Spike, gem5, Renode)
- Test output (CTest verbose logs)
- Compilation errors (full CMake output)
- GDB/simulator traces if available

---

## Key Design Documents to Read

| Document | When to Read | Purpose |
|----------|-------------|---------|
| [00-project-overview.md](docs/00-project-overview.md) | **Always start here** | High-level vision, goals, structure |
| [01-platform-assessment.md](docs/01-platform-assessment.md) | Phase 2-7 | Platform details, memory maps, trade-offs |
| [02-baremetal-application.md](docs/02-baremetal-application.md) | Phase 2-4 | Boot sequence, drivers, module design |
| [03-platform-configurations.md](docs/03-platform-configurations.md) | Phase 4, 8 | SMP and AMP configuration strategies |
| [04-rvv-vector-extension.md](docs/04-rvv-vector-extension.md) | Phase 5 | RVV concepts, workloads, LMUL |
| [05-build-system.md](docs/05-build-system.md) | Phase 1 | ~~Make-based~~ Now CMake-based (update this) |
| [06-ci-cd-pipeline.md](docs/06-ci-cd-pipeline.md) | Phase 1+ | CI design, test strategy, artifacts |

---

## Quick Reference Card

### File Locations
- **Roadmap**: `ROADMAP.md` ← **Source of truth**
- **Design docs**: `docs/0X-*.md`
- **CMake root**: `CMakeLists.txt`
- **Presets**: `CMakePresets.json`
- **App source**: `app/src/`
- **Tests**: `tests/`
- **CI configs**: `.github/workflows/`

### Essential Commands
```bash
# Setup
./scripts/setup-toolchain.sh
./scripts/setup-simulators.sh

# Configure
cmake --preset default

# Build
cmake --build build/default

# Test
ctest --test-dir build/default --output-on-failure

# Run manually
qemu-system-riscv64 -M virt -nographic -bios none -kernel build/default/app/app.elf
```

### Key Defines
- `PLATFORM_QEMU_VIRT` - QEMU virt machine
- `PLATFORM_SPIKE` - Spike ISA simulator
- `PLATFORM_GEM5` - gem5 (SE or FS)
- `PLATFORM_RENODE` - Renode
- `ENABLE_SMP` - Multi-core support
- `ENABLE_RVV` - Vector extension
- `NUM_HARTS` - Number of harts (1, 2, 4, 8)

---

## Summary for Claude

When working on this project:
1. **Always start with ROADMAP.md** to know where we are
2. **Follow TDD religiously** - write tests before code
3. **Read relevant design docs** before implementing
4. **Think bare-metal** - no stdlib, no OS, direct hardware
5. **Validate cross-platform** - if it works on QEMU, test on Spike
6. **Update CI** as you add features
7. **Keep documentation in sync** with implementation
8. **Ask questions** if design is unclear

**This project values correctness over speed. Quality tests and clean code matter more than rushing to completion.**

---

**Last Updated:** 2026-02-08  
**Version:** 1.0  
**Maintained By:** Project Team
