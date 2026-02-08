# RISC-V System Simulation Platform - Implementation Roadmap

## Overview

This roadmap outlines the progressive implementation strategy for building a comprehensive RISC-V bare-metal simulation platform. We follow a **Test-Driven Development (TDD)** approach with **CMake + CTest** build system, starting from simple single-core configurations and progressively adding complexity.

**Current Status:** Phase 0 - Design & Setup ✓  
**Next Phase:** Phase 1 - Foundation

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

### 🎯 Phase 1: Foundation & Build System
**Goal:** Establish CMake build system and basic toolchain infrastructure

**Duration Estimate:** 2-3 weeks  
**Priority:** P0 (Critical)

#### 1.1 CMake Build System
- [ ] Create root CMakeLists.txt with project structure
- [ ] Define CMake presets (CMakePresets.json)
  - `default` - QEMU single-core
  - `debug` - Debug build with symbols
  - `release` - Optimized release build
  - Platform-specific presets (qemu, spike, gem5, renode)
- [ ] Create toolchain file (riscv64-unknown-elf.cmake)
- [ ] Add ISA configuration options (rv64gc, rv64gcv)
- [ ] Implement build directory structure
- [ ] Add helper scripts (configure.sh, build.sh)

#### 1.2 CTest Infrastructure
- [ ] Create tests/ directory structure
- [ ] Set up CTest framework
- [ ] Add test discovery mechanism
- [ ] Create test helper utilities (output validation, timeout handling)
- [ ] Add test configuration (CTestConfig.cmake)
- [ ] Implement test result reporting

#### 1.3 Toolchain & Simulator Setup Scripts
- [ ] scripts/setup-toolchain.sh - Install RISC-V GCC toolchain
- [ ] scripts/setup-simulators.sh - Install QEMU, Spike
- [ ] scripts/verify-environment.sh - Check all dependencies
- [ ] Documentation: BUILD.md with setup instructions

#### 1.4 TDD Infrastructure
- [ ] Test template generator
- [ ] Output comparison utilities
- [ ] Golden reference mechanism
- [ ] Test result parser

**Exit Criteria:**
- CMake builds successfully with all presets
- CTest framework runs (even with empty tests)
- CI validates build system
- Documentation covers build/test workflows

---

### 🔨 Phase 2: Single-Core Bare-Metal (QEMU)
**Goal:** Minimal working bare-metal application on QEMU

**Duration Estimate:** 2-3 weeks  
**Priority:** P0 (Critical)  
**Platform:** QEMU virt machine

#### 2.1 TDD: Write Tests First
- [ ] Test: Boot and print "Hello RISC-V"
- [ ] Test: CSR read/write (mhartid, mstatus)
- [ ] Test: UART character output
- [ ] Test: Basic memory operations
- [ ] Test: Function call stack

#### 2.2 Implementation
- [ ] startup.S - Reset vector, stack initialization, BSS clearing
- [ ] Linker script: qemu-virt.ld
- [ ] platform.h - Platform abstraction layer
- [ ] uart.c/uart.h - NS16550A UART driver for QEMU virt
- [ ] printf.c - Minimal printf implementation
- [ ] csr.h - CSR access macros (mhartid, mstatus, mtvec, etc.)
- [ ] main.c - Simple main with test output
- [ ] trap.c/trap.h - Basic trap handler

#### 2.3 CMake Integration
- [ ] Add app/ CMakeLists.txt
- [ ] Add platform-specific compile definitions
- [ ] Create QEMU run target
- [ ] Add CTest test cases

#### 2.4 CI Updates
- [ ] Update ci-build.yml to use CMake
- [ ] Add QEMU simulation tests via CTest
- [ ] Validate test output in CI

**Exit Criteria:**
- All Phase 2.1 tests pass on QEMU
- Application boots, prints, exits cleanly
- CI runs and validates QEMU execution
- Code coverage ≥80%

---

### 🔀 Phase 3: Cross-Platform Support (Spike)
**Goal:** Port application to Spike ISA simulator

**Duration Estimate:** 1-2 weeks  
**Priority:** P0 (Critical)  
**Platform:** Spike

#### 3.1 TDD: Spike-Specific Tests
- [ ] Test: Boot on Spike
- [ ] Test: HTIF output
- [ ] Test: CSR access on Spike
- [ ] Test: Compare output with QEMU (functional equivalence)

#### 3.2 Implementation
- [ ] htif.c/htif.h - HTIF (Host-Target Interface) driver
- [ ] Linker script: spike.ld
- [ ] Platform abstraction for HTIF vs UART
- [ ] Spike-specific startup configuration

#### 3.3 CMake Integration
- [ ] Add Spike CMake preset
- [ ] Create Spike run target
- [ ] Add cross-platform test validation

#### 3.4 CI Updates
- [ ] Install Spike in CI
- [ ] Add Spike simulation tests
- [ ] Cross-validate QEMU vs Spike outputs

**Exit Criteria:**
- Application runs on both QEMU and Spike
- Output is functionally equivalent
- CI validates both platforms
- Cross-platform test harness operational

---

### 🚀 Phase 4: Multi-Core SMP Support
**Goal:** Enable symmetric multi-processing (2-8 harts)

**Duration Estimate:** 3-4 weeks  
**Priority:** P1 (High)  
**Platforms:** QEMU, Spike

#### 4.1 TDD: SMP Tests
- [ ] Test: Secondary hart boot
- [ ] Test: Per-hart stack allocation
- [ ] Test: Inter-hart communication
- [ ] Test: Spinlock operations
- [ ] Test: Barrier synchronization
- [ ] Test: Atomic operations (AMO)
- [ ] Test: Cache coherence validation (memory ordering)

#### 4.2 Implementation
- [ ] smp.c/smp.h - Multi-hart boot and synchronization
- [ ] Per-hart stack allocation in linker script
- [ ] Spinlock primitives (using AMO instructions)
- [ ] Barrier implementation
- [ ] Hart ID detection and management
- [ ] IPI (Inter-Processor Interrupt) support via CLINT

#### 4.3 CMake Integration
- [ ] Add NUM_HARTS build option
- [ ] SMP-specific presets (2-hart, 4-hart, 8-hart)
- [ ] CTest with SMP configurations

#### 4.4 CI Updates
- [ ] Matrix build: 1, 2, 4, 8 harts
- [ ] SMP validation tests in CI

**Exit Criteria:**
- Application boots all harts successfully
- Synchronization primitives validated
- SMP tests pass on QEMU and Spike
- CI covers 1/2/4/8 hart configurations

---

### 📊 Phase 5: RISC-V Vector Extension (RVV 1.0)
**Goal:** Implement RVV workloads with progressive complexity

**Duration Estimate:** 4-5 weeks  
**Priority:** P1 (High)  
**Platforms:** QEMU, Spike (RVV-enabled)

#### 5.1 TDD: RVV Tests
- [ ] Test: RVV detection (V bit in misa)
- [ ] Test: VLEN and ELEN detection
- [ ] Test: Basic vector add (VLEN-agnostic)
- [ ] Test: Vector dot product
- [ ] Test: Vector-matrix multiply
- [ ] Test: Vector memcpy
- [ ] Test: Different LMUL configurations (1/2, 1, 2, 4, 8)
- [ ] Test: Masked operations
- [ ] Test: Segment loads/stores

#### 5.2 Implementation: RVV Infrastructure
- [ ] rvv/rvv_detect.c - Runtime RVV capability detection
- [ ] rvv/rvv_common.h - RVV macros and intrinsics
- [ ] Compile-time RVV enabling (march=rv64gcv)

#### 5.3 Implementation: Progressive Workloads
- [ ] **Level 1:** rvv/vec_add.c - Element-wise vector addition
- [ ] **Level 2:** rvv/vec_dotprod.c - Vector dot product
- [ ] **Level 3:** rvv/vec_saxpy.c - SAXPY (y = a*x + y)
- [ ] **Level 4:** rvv/vec_memcpy.c - Vectorized memory copy
- [ ] **Level 5:** rvv/vec_matmul.c - Matrix multiplication
- [ ] **Level 6:** rvv/vec_reduce.c - Reduction operations
- [ ] **Level 7:** rvv/vec_permute.c - Permutation and slide

#### 5.4 CMake Integration
- [ ] RVV build option (ENABLE_RVV)
- [ ] RVV-specific presets
- [ ] Conditional compilation for RVV
- [ ] CTest for each RVV workload level

#### 5.5 CI Updates
- [ ] RVV builds in matrix
- [ ] QEMU with -cpu rv64,v=true,vlen=256
- [ ] Spike with --isa=rv64gcv --varch=vlen:256,elen:64

**Exit Criteria:**
- All RVV workloads pass on QEMU and Spike
- Tests cover multiple VLEN configurations (128, 256, 512)
- LMUL variations validated
- CI includes RVV test suite

---

### 🔬 Phase 6: gem5 Integration (Both Modes)
**Goal:** Add cycle-accurate simulation with gem5 SE and FS modes

**Duration Estimate:** 4-6 weeks  
**Priority:** P1 (High)  
**Platform:** gem5 (v23.0+)

#### 6.1 TDD: gem5 Tests
- [ ] Test: Boot on gem5 SE mode (AtomicSimpleCPU)
- [ ] Test: Boot on gem5 FS mode (TimingSimpleCPU)
- [ ] Test: Multi-core on gem5 (2-4 CPUs)
- [ ] Test: Performance counter validation
- [ ] Test: Cache statistics extraction
- [ ] Test: Compare cycle counts across CPU models

#### 6.2 gem5 SE (Syscall Emulation) Mode
- [ ] Port application to gem5 SE mode
- [ ] Platform configuration for SE mode
- [ ] CMake preset: gem5-se
- [ ] CTest integration for SE mode
- [ ] Python config: platforms/gem5/se_config.py

#### 6.3 gem5 FS (Full System) Mode
- [ ] Linker script: gem5-fs.ld
- [ ] UART/MMIO configuration for gem5 FS
- [ ] Boot sequence for gem5 FS
- [ ] Platform abstraction updates
- [ ] CMake preset: gem5-fs
- [ ] Python config: platforms/gem5/fs_config.py

#### 6.4 gem5 CPU Models
- [ ] AtomicSimpleCPU configuration
- [ ] TimingSimpleCPU configuration
- [ ] MinorCPU (in-order) configuration
- [ ] O3CPU (out-of-order) configuration [optional]

#### 6.5 Performance Analysis
- [ ] Extract and parse gem5 stats.txt
- [ ] CPI (Cycles Per Instruction) analysis
- [ ] Cache hit/miss rates
- [ ] Pipeline utilization
- [ ] Comparison scripts (QEMU functional vs gem5 performance)

#### 6.6 CMake Integration
- [ ] gem5-se and gem5-fs presets
- [ ] gem5 run targets for each CPU model
- [ ] Statistics collection targets
- [ ] Performance comparison tests

#### 6.7 CI Updates
- [ ] Separate workflow: ci-gem5.yml (already exists, update for SE+FS)
- [ ] Build and cache gem5
- [ ] Run gem5 SE and FS tests
- [ ] Upload performance statistics as artifacts

**Exit Criteria:**
- Application runs on gem5 SE mode (all CPU models)
- Application runs on gem5 FS mode (atomic, timing)
- Performance statistics collected and validated
- CI runs gem5 on main branch merges
- Documentation: gem5 setup and usage guide

---

### 🤖 Phase 7: Renode Integration
**Goal:** Add Renode for SoC-level simulation and peripheral modeling

**Duration Estimate:** 3-4 weeks  
**Priority:** P2 (Medium)  
**Platform:** Renode

#### 7.1 TDD: Renode Tests
- [ ] Test: Boot on Renode
- [ ] Test: UART communication
- [ ] Test: Multi-core on Renode
- [ ] Test: Platform peripherals (GPIO, timers)
- [ ] Test: Cross-validation with QEMU output

#### 7.2 Implementation
- [ ] Platform configuration file: platforms/renode/riscv_virt.repl
- [ ] Renode launch script: platforms/renode/run.resc
- [ ] Platform abstraction updates for Renode
- [ ] Linker script: renode.ld (or reuse qemu-virt.ld)

#### 7.3 Renode-Specific Features
- [ ] Peripheral modeling exploration (timers, GPIO)
- [ ] Multi-machine configuration (AMP)
- [ ] Robot Framework test integration

#### 7.4 CMake Integration
- [ ] Renode CMake preset
- [ ] Renode run targets
- [ ] CTest integration

#### 7.5 CI Updates
- [ ] Install Renode in CI
- [ ] Add Renode simulation tests
- [ ] Validate across QEMU, Spike, gem5, Renode

**Exit Criteria:**
- Application runs on Renode
- Output matches QEMU behavior
- Multi-core configurations validated
- CI includes Renode tests
- Documentation: Renode setup guide

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

**Last Updated:** 2026-02-08  
**Next Review:** Start of Phase 1  
**Maintained By:** Project Team
