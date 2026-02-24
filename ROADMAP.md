# RISC-V System Simulation Platform - Implementation Roadmap

## Overview

This roadmap outlines the progressive implementation strategy for building a comprehensive RISC-V bare-metal simulation platform. We follow a **Test-Driven Development (TDD)** approach with **CMake + CTest** build system, starting from simple single-core configurations and progressively adding complexity.

**Current Status:** Phase 7 - Renode Integration ✅ COMPLETE  
**Previous Phase:** Phase 6 - gem5 Integration (Both Modes) ✅ COMPLETE

---

## Development Philosophy

### Test-Driven Development (TDD)
- **Write tests first**: Define expected behavior before implementation
- **Red-Green-Refactor cycle**: Fail → Pass → Clean
- **Continuous validation**: Every feature has automated tests
- **Cross-platform verification**: Tests run on all supported simulators

### Progressive Complexity
- Start with simplest configurations (single-core, basic I/O)
- Validate on fast platforms (QEMU) first
- Add complexity incrementally (SMP, RVV, gem5, Renode)
- Each phase builds on previous validated work

### Build System Strategy
- **CMake** for cross-platform build configuration
- **CTest** for test orchestration and validation
- **Presets** for common configurations (Debug, Release, platforms)
- **Hierarchical targets**: Build specific platforms or all at once

---

## Milestone Phases

### ✅ Phase 0: Design & Setup (COMPLETE)
**Goal:** Establish project structure and documentation foundation

**Deliverables:**
- [x] Design documents (00-06)
- [x] Devcontainer setup
- [x] CI pipeline skeleton (GitHub Actions)
- [x] Project README
- [x] License and contributing guidelines

---

### ✅ Phase 1: Foundation & Build System (COMPLETE)
**Goal:** Establish CMake build system and basic toolchain infrastructure

**Completed:** 2026-02-08

#### 1.1 CMake Build System
- [x] Create root CMakeLists.txt with project structure
- [x] Define CMake presets (CMakePresets.json) - 10+ presets
  - `default` - QEMU single-core
  - `qemu-smp` - QEMU 4-hart SMP
  - `qemu-rvv-256` / `qemu-smp-rvv` - QEMU with RVV
  - `spike` / `spike-smp` / `spike-rvv` - Spike presets
  - `gem5-se` / `gem5-fs` - gem5 presets
  - `renode` - Renode preset
- [x] Create toolchain file (riscv64-elf.cmake)
- [x] Add ISA configuration options (rv64gc, rv64gcv)
- [x] Implement build directory structure

#### 1.2 CTest Infrastructure
- [x] Create tests/ directory structure
- [x] Set up CTest framework
- [x] Add test helper function (add_simulator_test)
- [x] Add test configuration (CTestConfig.cmake)

#### 1.3 Toolchain & Simulator Setup Scripts
- [x] scripts/setup-toolchain.sh - Install RISC-V GCC toolchain
- [x] scripts/setup-simulators.sh - Install QEMU, Spike
- [x] scripts/verify-environment.sh - Check all dependencies
- [x] Documentation: BUILD.md with setup instructions

#### 1.4 CI Integration
- [x] ci-build.yml - Build matrix (7 configs) + QEMU simulations
- [x] ci-lint.yml - clang-format + cppcheck
- [x] ci-gem5.yml - gem5 workflow skeleton (Phase 6)

---

### ✅ Phase 2: Single-Core Bare-Metal (COMPLETE)
**Goal:** Minimal working bare-metal application on QEMU

**Completed:** 2026-02-08  
**Platform:** QEMU virt machine

#### 2.1 TDD: Tests Written and Passing
- [x] Test: Boot and print "Hello RISC-V" (phase2_qemu_boot_hello)
- [x] Test: CSR read Hart ID (phase2_qemu_csr_hartid)
- [x] Test: CSR read mstatus (phase2_qemu_csr_mstatus)
- [x] Test: UART character output (phase2_qemu_uart_output)
- [x] Test: Basic memory operations (phase2_qemu_memory_ops)
- [x] Test: Function call stack (phase2_qemu_function_calls)
- [x] Test: Integration - all pass (phase2_qemu_complete)

#### 2.2 Implementation
- [x] startup.S - Reset vector, stack initialization, BSS clearing
- [x] Linker script: qemu-virt.ld
- [x] platform.h / platform.c - Platform abstraction layer
- [x] uart.c / uart.h - NS16550A UART driver for QEMU virt
- [x] csr.h - CSR access macros (mhartid, mstatus, mtvec, etc.)
- [x] main.c - Application with structured test output (5/5 PASS)

#### 2.3 CMake Integration
- [x] app/CMakeLists.txt with platform-specific compile definitions
- [x] QEMU run target via CTest
- [x] 7 CTest test cases for QEMU

#### 2.4 CI
- [x] Build matrix: 7 configurations (QEMU x4, Spike x3) all passing
- [x] QEMU simulation tests: 3 configs (single, SMP, RVV) all passing
- [x] Devcontainer build verification passing

**Results:**
- CI: All checks GREEN (lint, build 7/7, simulate 3/3, devcontainer)
- Output: `[RESULT] Phase 2 tests: 5/5 PASS`

---

### ✅ Phase 3: Cross-Platform Support (Spike) (COMPLETE)
**Goal:** Port application to Spike ISA simulator

**Completed:** 2026-02-12  
**Platform:** Spike

#### 3.1 HTIF Driver Fixes
- [x] spike.ld: Export `tohost`/`fromhost` symbols (Spike requires exact names in ELF)
- [x] htif.c: Use extern linker symbols instead of hardcoded addresses
- [x] htif.c: Clean HTIF init and character output
- [x] platform.c: `platform_exit()` with HTIF poweroff for Spike
- [x] platform.c: `platform_exit()` with sifive_test finisher for QEMU
- [x] startup.S: Call `platform_exit()` after main returns

#### 3.2 TDD: Spike-Specific Tests (8 tests, all passing)
- [x] phase3_spike_boot_hello - Boot and print via HTIF
- [x] phase3_spike_csr_hartid - CSR access on Spike
- [x] phase3_spike_csr_mstatus - mstatus read on Spike
- [x] phase3_spike_htif_output - HTIF console output
- [x] phase3_spike_memory_ops - Memory operations
- [x] phase3_spike_function_calls - Function calls
- [x] phase3_spike_complete - Integration (5/5 PASS)
- [x] phase3_spike_platform_name - Platform reports "Spike"

#### 3.3 CI Integration
- [x] Spike simulation job in ci-build.yml (single-core + RVV)
- [x] Spike built from source with caching
- [x] Output validation for Spike simulations
- [x] Cross-platform validation job (QEMU vs Spike output diff)
- [x] Improved QEMU validation to match Spike approach

#### 3.4 Cross-Platform Validation
- [x] QEMU and Spike produce functionally identical test output
- [x] Both platforms: 5/5 tests PASS, same CSR values, same test flow
- [x] Clean exit on both platforms (sifive_test for QEMU, HTIF for Spike)

**Results:**
- Spike: 8/8 CTest tests PASS (0.10s)
- QEMU: 7/7 CTest tests PASS (0.19s)
- Cross-platform: Functionally identical output

---

### ✅ Phase 4: Multi-Core SMP Support (COMPLETE)
**Goal:** Enable symmetric multi-processing (2-8 harts)

**Completed:** 2026-02-12  
**Platforms:** QEMU, Spike

#### 4.1 TDD: SMP Tests (7 QEMU + 5 Spike, all passing)
- [x] Test: Secondary hart boot (phase4_qemu_smp_boot, phase4_spike_smp_boot)
- [x] Test: Per-hart stack allocation (phase4_qemu_smp_per_hart)
- [x] Test: Spinlock operations (phase4_qemu_smp_spinlock, phase4_spike_smp_spinlock)
- [x] Test: Barrier synchronization (phase4_qemu_smp_barrier, phase4_spike_smp_barrier)
- [x] Test: Atomic operations (AMO) (phase4_qemu_smp_atomic, phase4_spike_smp_atomic)
- [x] Test: Hello RISC-V in SMP mode (phase4_qemu_smp_hello)
- [x] Test: Integration (phase4_qemu_smp_complete, phase4_spike_smp_complete)

#### 4.2 Implementation
- [x] smp.c/smp.h - Multi-hart boot protocol and synchronization
- [x] atomic.h - RISC-V AMO instructions (amoadd, amoswap, lr/sc CAS)
- [x] console.h - Platform-independent console abstraction
- [x] Per-hart stack allocation in linker scripts (16-byte aligned)
- [x] Spinlock using LR/SC with acquire-release ordering
- [x] Centralized barrier with generation counter
- [x] startup.S - SMP boot protocol (hart 0 init, secondaries spin-wait)
- [x] main.c - Phase 4 SMP tests (boot, spinlock, atomic, barrier)

#### 4.3 CMake Integration
- [x] NUM_HARTS build option (existing, now fully functional)
- [x] SMP presets: qemu-smp (4), qemu-smp-8 (8), spike-smp (4)
- [x] Phase 2 tests gated on NUM_HARTS==1, Phase 4 on NUM_HARTS>1
- [x] CTest: 7 QEMU SMP tests + 5 Spike SMP tests

#### 4.4 CI Updates
- [x] QEMU SMP simulation with validation (boot, spinlock, atomic, barrier)
- [x] Spike SMP simulation added to CI matrix
- [x] Cross-platform validation still works (single-core QEMU vs Spike)

**Results:**
- QEMU 4-hart: 4/4 SMP tests PASS, 7/7 CTest PASS
- QEMU 8-hart: 4/4 SMP tests PASS
- Spike 4-hart: 4/4 SMP tests PASS
- Single-core regression: Phase 2 (5/5) and Phase 3 (8/8) unaffected

---

### ✅ Phase 5: RISC-V Vector Extension (RVV 1.0) (COMPLETE)
**Goal:** Implement RVV workloads with progressive complexity

**Completed:** 2026-02-12  
**Platforms:** QEMU, Spike (RVV-enabled)

#### 5.1 TDD: RVV Tests (10 QEMU + 9 Spike, all passing)
- [x] Test: RVV detection (V bit in misa) (phase5_qemu_rvv_detect, phase5_spike_rvv_detect)
- [x] Test: VLEN detection (phase5_qemu_rvv_vlen, phase5_spike_rvv_vlen)
- [x] Test: Integer vector add (phase5_qemu_rvv_vec_add_i32)
- [x] Test: Float32 vector add (phase5_qemu_rvv_vec_add_f32)
- [x] Test: Vector dot product with reduction (phase5_qemu_rvv_dot_product)
- [x] Test: SAXPY fused multiply-accumulate (phase5_qemu_rvv_saxpy)
- [x] Test: Matrix multiplication (phase5_qemu_rvv_matmul)
- [x] Test: Vectorized memcpy (phase5_qemu_rvv_memcpy)
- [x] Test: All 7/7 workloads pass (phase5_qemu_rvv_complete, phase5_spike_rvv_complete)

#### 5.2 Implementation: RVV Infrastructure
- [x] rvv/rvv_detect.h - RVV detection (misa V-bit, VLEN, VLENB via CSR)
- [x] rvv/rvv_detect.c - Runtime capability reporting with VL for SEW/LMUL combos
- [x] rvv/rvv_common.h - Common types, benchmark structs, float comparison
- [x] platform.c - Enable mstatus.VS and mstatus.FS during init
- [x] Compile-time RVV enabling (march=rv64gcv)

#### 5.3 Implementation: Progressive Workloads (all inline assembly, VLEN-agnostic)
- [x] **Level 1:** rvv/vec_add.c - Integer vector addition (vadd.vv)
- [x] **Level 1:** rvv/vec_add.c - Float32 vector addition (vfadd.vv)
- [x] **Level 1:** rvv/vec_memcpy.c - Vectorized memcpy (vle8/vse8, LMUL=8)
- [x] **Level 2:** rvv/vec_dotprod.c - Dot product with vfredosum reduction
- [x] **Level 2:** rvv/vec_saxpy.c - SAXPY using vfmacc.vf (fused multiply-accumulate)
- [x] **Level 3:** rvv/vec_matmul.c - Matrix multiply vectorized across columns
- [x] Scalar reference implementations for all workloads (correctness verification)
- [x] Cycle count comparison via mcycle CSR

#### 5.4 CMake Integration
- [x] ENABLE_RVV build option with conditional source inclusion
- [x] RVV-specific presets (qemu-rvv, qemu-rvv-256, spike-rvv, qemu-smp-rvv)
- [x] Phase 2/3 tests gated to NOT run when ENABLE_RVV=ON
- [x] 10 QEMU + 9 Spike CTest test cases for Phase 5
- [x] Build and test presets for qemu-rvv, qemu-rvv-256, spike-rvv

#### 5.5 CI Updates
- [x] RVV builds in matrix (QEMU RVV, QEMU SMP+RVV, Spike RVV)
- [x] QEMU with -cpu rv64,v=true,vlen=${VLEN}
- [x] Spike with --isa=rv64gcv
- [x] Phase 5 validation checks in simulation jobs
- [x] cppcheck --inline-suppr for assembly false positives

**Results:**
- QEMU VLEN=128: 7/7 workloads PASS, 10/10 CTests PASS
- QEMU VLEN=256: 7/7 workloads PASS, 10/10 CTests PASS
- Single-core regression: Phase 2 (7/7) unaffected
- SMP regression: Phase 4 (7/7) unaffected

---

### ✅ Phase 6: gem5 Integration (Both Modes) (COMPLETE)
**Goal:** Add cycle-accurate simulation with gem5 SE and FS modes

**Completed:** 2026-02-24  
**Platform:** gem5 (v23.0+)

#### 6.1 TDD: gem5 Tests (14 tests defined)
- [x] Test: Boot on gem5 FS mode (AtomicSimpleCPU) (phase6_gem5_fs_boot_hello)
- [x] Test: CSR read on gem5 FS (phase6_gem5_fs_csr_hartid, phase6_gem5_fs_csr_mstatus)
- [x] Test: All Phase 2 tests pass on gem5 FS (phase6_gem5_fs_complete)
- [x] Test: Platform name reports "gem5" (phase6_gem5_fs_platform_name)
- [x] Test: Boot on gem5 FS with TimingSimpleCPU (phase6_gem5_fs_timing_boot)
- [x] Test: Boot on gem5 FS with MinorCPU (phase6_gem5_fs_minor_boot)
- [x] Test: gem5 stats file generated (phase6_gem5_fs_stats_generated)
- [x] Test: SMP boot on gem5 FS (phase6_gem5_fs_smp_boot)
- [x] Test: SMP complete on gem5 FS (phase6_gem5_fs_smp_complete)
- [x] Test: Boot on gem5 SE mode (phase6_gem5_se_boot_hello)
- [x] Test: All Phase 2 tests pass on gem5 SE (phase6_gem5_se_complete)
- [x] Test: Platform name in SE mode (phase6_gem5_se_platform_name)
- [x] Test: Compare cycle counts across CPU models (phase6_gem5_fs_cycle_compare)

#### 6.2 gem5 SE (Syscall Emulation) Mode
- [x] Port application to gem5 SE mode (gem5_se_io.c/h)
- [x] Syscall-based I/O (ecall: write=64, exit=93)
- [x] Platform configuration for SE mode (console.h, platform.c)
- [x] CMake preset: gem5-se
- [x] CTest integration for SE mode (3 tests)
- [x] Python config: platforms/gem5/configs/se_config.py

#### 6.3 gem5 FS (Full System) Mode
- [x] Linker script: app/linker/gem5.ld (based on qemu-virt.ld)
- [x] UART (NS16550A at 0x10000000) reused from QEMU driver
- [x] Boot sequence: same startup.S (BSS clear, stack setup, trap handler)
- [x] Platform abstraction: m5ops for gem5-specific exit
- [x] CMake preset: gem5-fs
- [x] Python config: platforms/gem5/configs/fs_config.py
- [x] gem5 UART (Uart8250) in FS config

#### 6.4 gem5 CPU Models
- [x] AtomicSimpleCPU configuration (gem5-fs preset, default)
- [x] TimingSimpleCPU configuration (gem5-fs-timing preset)
- [x] MinorCPU (in-order) configuration (gem5-fs-minor preset)
- [x] O3CPU (out-of-order) configuration (gem5-fs-o3 preset)

#### 6.5 Performance Analysis
- [x] Extract and parse gem5 stats.txt (scripts/parse-gem5-stats.py)
- [x] CPI (Cycles Per Instruction) analysis
- [x] Cache hit/miss rates (L1I, L1D, L2)
- [x] Memory controller statistics
- [x] Comparison scripts (--compare mode for CPU model comparison)
- [x] JSON and CSV output formats

#### 6.6 CMake Integration
- [x] gem5-se and gem5-fs presets (existing, verified)
- [x] gem5-fs-timing, gem5-fs-minor, gem5-fs-o3 presets (new)
- [x] gem5-fs-smp preset (existing, verified)
- [x] GEM5_CPU_TYPE cache variable
- [x] gem5 run targets for each CPU model (Simulators.cmake)
- [x] Build and test presets for all gem5 configurations

#### 6.7 CI Updates
- [x] ci-gem5.yml: Full workflow (build ELFs, build gem5, simulate, compare)
- [x] Build ELFs for gem5-fs, gem5-se, gem5-fs-smp in CI
- [x] gem5 simulator caching between CI runs
- [x] FS simulation with AtomicSimpleCPU and TimingSimpleCPU
- [x] SE simulation with AtomicSimpleCPU
- [x] Performance comparison job (Atomic vs Timing)
- [x] Upload gem5 stats as CI artifacts
- [x] ci-build.yml: gem5 presets in build matrix (gem5-fs, gem5-se, gem5-fs-smp)

**Exit Criteria:**
- [x] Application builds for gem5 SE and FS modes
- [x] gem5 Python configs for all CPU models (Atomic, Timing, Minor, O3)
- [x] CTest tests defined for gem5 SE and FS (14 tests)
- [x] Performance statistics parser (JSON/CSV/comparison)
- [x] CI builds gem5 binaries and runs simulations
- [x] Validate on actual gem5 simulator (CI runs gem5 FS/SE simulations)

---

### ✅ Phase 7: Renode Integration (COMPLETE)
**Goal:** Add Renode for SoC-level simulation and peripheral modeling

**Completed:** 2026-02-24  
**Platform:** Renode

#### 7.1 TDD: Renode Tests
- [x] Test: Boot on Renode (phase7_renode_boot_hello)
- [x] Test: UART communication (via run-renode-test.sh, uart_output.txt)
- [x] Test: Multi-core on Renode (phase7_renode_smp_boot, phase7_renode_smp_complete)
- [ ] Test: Platform peripherals (GPIO, timers) - deferred to Phase 10
- [x] Test: Cross-validation with QEMU output (CI cross-validate job)

#### 7.2 Implementation
- [x] Platform configuration file: platforms/renode/riscv_virt.repl
- [x] Platform configuration file: platforms/renode/riscv_virt_smp.repl (4-hart SMP)
- [x] Renode launch script: platforms/renode/configs/run.resc
- [x] Renode launch script: platforms/renode/configs/run_smp.resc
- [x] Platform abstraction updates for Renode (PLATFORM_RENODE)
- [x] Linker script: app/linker/renode.ld

#### 7.3 Renode-Specific Features
- [ ] Peripheral modeling exploration (timers, GPIO) - deferred
- [ ] Multi-machine configuration (AMP) - Phase 8
- [ ] Robot Framework test integration - deferred

#### 7.4 CMake Integration
- [x] Renode CMake preset (renode, renode-smp)
- [x] Renode run targets (run-renode)
- [x] CTest integration (5 tests: 3 single-core + 2 SMP)

#### 7.5 CI Updates
- [x] Install Renode in CI (antmicro/renode Docker image)
- [x] Add Renode simulation tests (simulate-renode, simulate-renode-smp)
- [x] Validate across QEMU, Spike, gem5, Renode (cross-validate job)

**Exit Criteria:**
- [x] Application runs on Renode
- [x] Output matches QEMU behavior (cross-validation)
- [x] Multi-core configurations validated
- [x] CI includes Renode tests
- [x] Documentation: BUILD.md Renode setup guide

---

### 🔗 Phase 8: Multi-Processor (AMP) Configurations
**Goal:** Heterogeneous multi-processor configurations

**Duration Estimate:** 3-4 weeks  
**Priority:** P2 (Medium)  
**Platforms:** gem5, Renode

#### 8.1 TDD: AMP Tests
- [ ] Test: Asymmetric hart configurations
- [ ] Test: Shared memory communication
- [ ] Test: Message passing between processors
- [ ] Test: Different ISA variants on different harts

#### 8.2 Implementation
- [ ] AMP boot protocol
- [ ] Shared memory regions in linker script
- [ ] Message queue implementation
- [ ] Heterogeneous hart management

#### 8.3 gem5 AMP Configuration
- [ ] Python configs for heterogeneous CPU types
- [ ] Different clock domains
- [ ] Asymmetric cache hierarchies

#### 8.4 Renode AMP Configuration
- [ ] Multi-machine .repl files
- [ ] Inter-machine communication

#### 8.5 CMake & CI
- [ ] AMP presets
- [ ] CTest for AMP configurations
- [ ] CI validation

**Exit Criteria:**
- AMP configurations boot successfully
- Message passing validated
- gem5 and Renode AMP working
- CI covers AMP scenarios

---

### 🧪 Phase 9: Advanced Testing & Validation
**Goal:** Comprehensive test coverage and cross-platform validation

**Duration Estimate:** 2-3 weeks  
**Priority:** P2 (Medium)

#### 9.1 Test Suite Expansion
- [ ] ISA compliance tests (adapted from riscv-tests)
- [ ] Stress tests (memory, computation)
- [ ] Concurrency tests (SMP race conditions)
- [ ] Endurance tests (long-running workloads)
- [ ] Regression test database

#### 9.2 Cross-Platform Validation
- [ ] Output comparison framework
- [ ] Functional equivalence validation
- [ ] Performance regression tracking
- [ ] Automated diff reports

#### 9.3 Code Quality
- [ ] Code coverage analysis (gcov/llvm-cov)
- [ ] Static analysis (cppcheck, clang-tidy)
- [ ] Memory safety validation (ASan-compatible)
- [ ] Misra-C compliance checks [optional]

#### 9.4 Documentation
- [ ] Test methodology guide
- [ ] How to write tests
- [ ] Debugging guide
- [ ] Performance analysis guide

**Exit Criteria:**
- Test coverage ≥90%
- All platforms cross-validated
- Regression suite operational
- Documentation complete

---

### 📚 Phase 10: Advanced Features & Optimization
**Goal:** Performance counters, cache analysis, optimization

**Duration Estimate:** 3-4 weeks  
**Priority:** P3 (Low)

#### 10.1 Performance Monitoring
- [ ] PMU (Performance Monitoring Unit) access
- [ ] Hardware performance counters
- [ ] Cycle-accurate benchmarking
- [ ] Cache performance analysis (gem5)

#### 10.2 Advanced RVV
- [ ] Auto-vectorization experiments (compiler flags)
- [ ] Custom RVV intrinsics library
- [ ] RVV performance optimization
- [ ] LMUL tuning guide

#### 10.3 Optimization
- [ ] Compiler optimization exploration (-O2, -O3, -Os)
- [ ] Link-time optimization (LTO)
- [ ] Profile-guided optimization (PGO)
- [ ] Size optimization techniques

#### 10.4 Comparison & Analysis
- [ ] QEMU vs Spike functional comparison
- [ ] gem5 CPU model comparison report
- [ ] Performance vs accuracy trade-off analysis
- [ ] Platform selection guide

**Exit Criteria:**
- Performance analysis tools complete
- Optimization guide published
- Comparison reports generated
- Best practices documented

---

## CMake Build System Structure

### Directory Layout
```
.
├── CMakeLists.txt                 # Root CMake configuration
├── CMakePresets.json              # Build presets
├── cmake/
│   ├── toolchain/
│   │   └── riscv64-elf.cmake     # RISC-V toolchain file
│   ├── platforms/
│   │   ├── qemu.cmake            # QEMU-specific settings
│   │   ├── spike.cmake           # Spike-specific settings
│   │   ├── gem5.cmake            # gem5-specific settings
│   │   └── renode.cmake          # Renode-specific settings
│   └── modules/
│       ├── Simulators.cmake       # Find simulator executables
│       ├── Testing.cmake          # CTest helpers
│       └── Formatting.cmake       # clang-format integration
├── app/
│   ├── CMakeLists.txt            # Application build
│   ├── src/
│   ├── include/
│   └── linker/
├── tests/
│   ├── CMakeLists.txt            # Test definitions
│   ├── integration/              # Integration tests
│   ├── unit/                     # Unit tests (if applicable)
│   └── utils/                    # Test utilities
└── platforms/
    ├── qemu/
    ├── spike/
    ├── gem5/
    └── renode/
```

### CMake Presets (CMakePresets.json)
```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "default",
      "displayName": "Default (QEMU Single-Core)",
      "description": "QEMU virt machine, single hart, no RVV",
      "binaryDir": "${sourceDir}/build/default",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "PLATFORM": "qemu",
        "CONFIG": "single",
        "ENABLE_RVV": "OFF",
        "NUM_HARTS": "1"
      }
    },
    {
      "name": "qemu-smp",
      "inherits": "default",
      "displayName": "QEMU SMP (4 harts)",
      "binaryDir": "${sourceDir}/build/qemu-smp",
      "cacheVariables": {
        "CONFIG": "smp",
        "NUM_HARTS": "4"
      }
    },
    {
      "name": "qemu-rvv",
      "inherits": "default",
      "displayName": "QEMU with RVV",
      "binaryDir": "${sourceDir}/build/qemu-rvv",
      "cacheVariables": {
        "ENABLE_RVV": "ON"
      }
    },
    {
      "name": "spike",
      "inherits": "default",
      "displayName": "Spike Single-Core",
      "binaryDir": "${sourceDir}/build/spike",
      "cacheVariables": {
        "PLATFORM": "spike"
      }
    },
    {
      "name": "gem5-se",
      "inherits": "default",
      "displayName": "gem5 Syscall Emulation",
      "binaryDir": "${sourceDir}/build/gem5-se",
      "cacheVariables": {
        "PLATFORM": "gem5",
        "GEM5_MODE": "se"
      }
    },
    {
      "name": "gem5-fs",
      "inherits": "default",
      "displayName": "gem5 Full System",
      "binaryDir": "${sourceDir}/build/gem5-fs",
      "cacheVariables": {
        "PLATFORM": "gem5",
        "GEM5_MODE": "fs"
      }
    },
    {
      "name": "renode",
      "inherits": "default",
      "displayName": "Renode",
      "binaryDir": "${sourceDir}/build/renode",
      "cacheVariables": {
        "PLATFORM": "renode"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "default",
      "configurePreset": "default"
    }
  ],
  "testPresets": [
    {
      "name": "default",
      "configurePreset": "default",
      "output": {
        "outputOnFailure": true
      }
    }
  ]
}
```

### Key CMake Targets

| Target | Description |
|--------|-------------|
| `app` | Build bare-metal ELF |
| `all` | Build all configurations |
| `test` | Run all CTests |
| `run-qemu` | Build and run on QEMU |
| `run-spike` | Build and run on Spike |
| `run-gem5-se` | Build and run on gem5 SE mode |
| `run-gem5-fs` | Build and run on gem5 FS mode |
| `run-renode` | Build and run on Renode |
| `format` | Run clang-format |
| `lint` | Run static analysis |
| `clean` | Clean build artifacts |

---

## TDD Workflow Example

### Red-Green-Refactor Cycle

#### 1. RED: Write failing test
```cmake
# tests/integration/test_uart.cmake
add_test(
  NAME uart_hello_world
  COMMAND qemu-system-riscv64 -M virt -nographic -bios none -kernel $<TARGET_FILE:app>
)
set_tests_properties(uart_hello_world PROPERTIES
  PASS_REGULAR_EXPRESSION "Hello RISC-V"
  TIMEOUT 30
)
```

Run test (expect failure):
```bash
cmake --preset default
cmake --build build/default
ctest --test-dir build/default
# FAIL: uart_hello_world (Expected)
```

#### 2. GREEN: Implement feature
```c
// app/src/main.c
int main() {
    uart_init();
    uart_puts("Hello RISC-V\n");
    return 0;
}
```

Rebuild and test:
```bash
cmake --build build/default
ctest --test-dir build/default
# PASS: uart_hello_world
```

#### 3. REFACTOR: Improve code quality
- Extract functions
- Add documentation
- Optimize performance
- Ensure tests still pass

---

## CI/CD Integration with CMake

### Updated CI Workflow (.github/workflows/ci-build.yml)
```yaml
jobs:
  build:
    strategy:
      matrix:
        preset: [default, qemu-smp, qemu-rvv, spike, gem5-se, renode]
    steps:
      - name: Configure
        run: cmake --preset ${{ matrix.preset }}
      
      - name: Build
        run: cmake --build build/${{ matrix.preset }}
      
      - name: Test
        run: ctest --test-dir build/${{ matrix.preset }} --output-on-failure
```

---

## Success Metrics

### Per-Phase Metrics
- ✅ **Tests Written**: Number of test cases created
- ✅ **Tests Passing**: Percentage of tests passing
- ✅ **Code Coverage**: Line/branch coverage percentage
- ✅ **Platforms Validated**: Number of platforms passing tests
- ✅ **CI Status**: All CI jobs passing
- ✅ **Documentation**: Phase-specific docs complete

### Overall Project Metrics
- **Total Test Cases**: 200+ by Phase 10
- **Code Coverage**: ≥90%
- **Platform Support**: 5 simulators (QEMU, Spike, gem5 SE/FS, Renode)
- **Configuration Variants**: 20+ (platforms × configs × ISA)
- **CI Pipeline**: <15 minutes for main build, <60 minutes for full matrix

---

## Dependencies & Prerequisites

### Per Phase
| Phase | Tools Required | Time Investment |
|-------|---------------|-----------------|
| 1 | CMake 3.20+, RISC-V GCC | 1-2 days setup |
| 2 | QEMU 7.0+ | 5 min install |
| 3 | Spike | 15 min build |
| 4 | (same as 2-3) | None |
| 5 | QEMU+Spike with RVV | 5 min config |
| 6 | gem5 (30-60 min build) | 1 hour setup |
| 7 | Renode | 10 min install |
| 8 | gem5 + Renode | (already installed) |
| 9 | Code coverage tools | 10 min |
| 10 | Python for analysis | 5 min |

---

## Open Questions & Decisions

### Build System
- [x] **Decision**: Migrate from Make to CMake for better cross-platform support
- [ ] Should we support both Make and CMake during transition? **Recommendation:** CMake only for cleaner codebase
- [ ] Should we use Ninja backend? **Recommendation:** Yes, for faster builds

### Testing Strategy
- [ ] Unit tests for bare-metal (challenging)? **Recommendation:** Focus on integration tests
- [ ] How to handle flaky tests? **Recommendation:** 3-retry policy with timeout enforcement
- [ ] Golden reference files or programmatic validation? **Recommendation:** Both - golden for complex outputs, programmatic for simple checks

### Platform Priority
- [x] QEMU first (fast iteration)
- [x] Spike second (reference correctness)
- [ ] gem5 SE or FS first? **Recommendation:** SE first (simpler), then FS
- [x] Renode after gem5 (lower priority)

### RVV Strategy
- [ ] RVV intrinsics or inline assembly? **Recommendation:** Intrinsics primarily, inline asm for edge cases
- [ ] Which VLEN to target? **Recommendation:** VLEN-agnostic code, test with 128/256/512

---

## Maintenance & Updates

### Continuous Activities
- **CI Monitoring**: Watch for flaky tests, fix immediately
- **Dependency Updates**: Monthly check for QEMU/Spike/gem5 updates
- **Documentation**: Update as features evolve
- **Test Expansion**: Add tests for every bug fix
- **Performance Tracking**: Monitor gem5 cycle counts for regressions

### Quarterly Reviews
- Assess roadmap progress
- Reprioritize based on feedback
- Update tools and simulators
- Review test coverage

---

## Contributing & Development Workflow

### For Contributors
1. **Pick a phase** from the roadmap
2. **Write tests first** (TDD approach)
3. **Implement feature** to pass tests
4. **Ensure all platforms pass** (QEMU, Spike, etc.)
5. **Update CI** to include new tests
6. **Submit PR** with tests + implementation

### PR Requirements
- ✅ All existing tests pass
- ✅ New tests added for new features
- ✅ Code coverage maintained or improved
- ✅ CI passes on all platforms
- ✅ Documentation updated
- ✅ clang-format compliant

---

## References

- [Design Proposals](docs/) - Detailed design documents (00-06)
- [CMake Documentation](https://cmake.org/documentation/)
- [CTest Documentation](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- [RISC-V ISA Specification](https://riscv.org/technical/specifications/)
- [RISC-V Vector Extension](https://github.com/riscv/riscv-v-spec)
- [QEMU RISC-V Documentation](https://www.qemu.org/docs/master/system/target-riscv.html)
- [gem5 Documentation](https://www.gem5.org/documentation/)
- [Renode Documentation](https://renode.readthedocs.io/)

---

**Last Updated:** 2026-02-12  
**Next Review:** Start of Phase 5  
**Maintained By:** Project Team
