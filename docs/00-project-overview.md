# Design Proposal 00: Project Overview and Roadmap

## 1. Project Vision

This project provides a **comprehensive, self-contained learning and development environment** for RISC-V system simulation. It enables building, running, and analyzing bare-metal RISC-V applications across multiple simulation platforms, with support for:

- **Single-core** and **multi-core** (SMP) configurations
- **Multi-processor** (AMP / heterogeneous) configurations
- **RISC-V Vector Extension (RVV 1.0)** workloads
- **Multiple simulation backends** (QEMU, gem5, Spike, and more)

The project is designed as a **learning platform** for system architects who want to understand RISC-V ISA, micro-architecture simulation, and platform bring-up.

---

## 2. Project Goals

| # | Goal | Description |
|---|------|-------------|
| G1 | **Platform Assessment** | Evaluate and document all major RISC-V simulation/emulation platforms |
| G2 | **Bare-Metal Application** | Build a non-trivial bare-metal C application targeting RV64GCV |
| G3 | **Cross-Platform ELF** | Produce ELF binaries that run on QEMU, gem5, Spike, and other simulators |
| G4 | **RVV Learning** | Include vector-extension workloads with progressive complexity |
| G5 | **Configurable Topology** | Support 1-core, N-core SMP, and multi-processor configurations |
| G6 | **Reproducible Environment** | Devcontainer + CI for fully reproducible builds and simulation runs |
| G7 | **Documentation-First** | Every design decision is documented before implementation |

---

## 3. Repository Structure (Proposed)

```
riscv/
├── .devcontainer/              # Dev environment (Docker + VSCode config)
│   ├── Dockerfile
│   └── devcontainer.json
├── .github/
│   └── workflows/
│       ├── ci-build.yml        # Build bare-metal ELFs
│       ├── ci-simulate.yml     # Run on QEMU/Spike in CI
│       └── ci-lint.yml         # Static analysis + format checks
├── docs/                       # Design proposals (this folder)
│   ├── 00-project-overview.md
│   ├── 01-platform-assessment.md
│   ├── 02-baremetal-application.md
│   ├── 03-platform-configurations.md
│   ├── 04-rvv-vector-extension.md
│   ├── 05-build-system.md
│   └── 06-ci-cd-pipeline.md
├── app/                        # Bare-metal application source
│   ├── src/                    # C source files
│   │   ├── main.c
│   │   ├── startup.S           # Assembly startup / reset vector
│   │   ├── uart.c              # UART driver (platform-abstracted)
│   │   ├── trap.c              # Trap/interrupt handler
│   │   ├── smp.c               # Multi-core boot and sync
│   │   └── rvv/                # RVV workloads
│   │       ├── vec_add.c
│   │       ├── vec_matmul.c
│   │       └── vec_memcpy.c
│   ├── include/                # Headers
│   │   ├── platform.h          # Platform abstraction
│   │   ├── uart.h
│   │   ├── csr.h               # CSR access macros
│   │   ├── smp.h
│   │   └── rvv.h               # RVV intrinsics / inline asm helpers
│   ├── linker/                 # Linker scripts
│   │   ├── qemu-virt.ld
│   │   ├── spike.ld
│   │   └── gem5.ld
│   └── Makefile                # Build system
├── platforms/                  # Simulator launch configs
│   ├── qemu/
│   │   ├── run-single.sh
│   │   ├── run-smp.sh
│   │   └── run-rvv.sh
│   ├── spike/
│   │   ├── run-single.sh
│   │   ├── run-smp.sh
│   │   └── run-rvv.sh
│   ├── gem5/
│   │   ├── configs/            # gem5 Python config scripts
│   │   │   ├── single_core.py
│   │   │   ├── multi_core.py
│   │   │   └── rvv_core.py
│   │   └── run.sh
│   └── riscvemu/               # TinyEMU / other lightweight sims
│       └── run.sh
├── scripts/                    # Helper scripts
│   ├── setup-toolchain.sh      # Download/install RISC-V GCC
│   ├── setup-simulators.sh     # Build/install simulators
│   └── compare-outputs.sh      # Compare sim outputs for validation
├── tests/                      # Validation tests
│   ├── test_uart_output.py
│   └── test_rvv_results.py
└── README.md
```

---

## 4. Phased Roadmap

### Phase 1: Foundation (Current)
- [x] Design proposals and documentation
- [x] Devcontainer setup
- [x] CI pipeline skeleton
- [ ] Toolchain setup scripts

### Phase 2: Single-Core Bare-Metal
- [ ] Startup assembly (reset vector, stack init, BSS clear)
- [ ] Linker scripts for QEMU virt machine
- [ ] UART driver (NS16550A for QEMU virt)
- [ ] Basic `main()` with "Hello RISC-V" output
- [ ] Run on QEMU and Spike

### Phase 3: Multi-Core (SMP)
- [ ] Secondary hart boot protocol (using CLINT IPI or SBI)
- [ ] Spin-lock and atomic operations
- [ ] Per-hart stack allocation
- [ ] SMP workload demonstration
- [ ] Run SMP configs on QEMU (up to 8 harts) and Spike

### Phase 4: RVV Vector Extension
- [ ] RVV intrinsics and inline assembly helpers
- [ ] Vector addition workload
- [ ] Vector matrix multiply
- [ ] Vector memcpy benchmark
- [ ] Run on QEMU (with `-cpu rv64,v=true`) and Spike (`--isa=rv64gcv`)

### Phase 5: gem5 Integration
- [ ] gem5 RISC-V system configuration scripts
- [ ] Single-core and multi-core gem5 configs
- [ ] Performance analysis and statistics extraction
- [ ] Comparison with functional simulators

### Phase 6: Multi-Processor (AMP)
- [ ] Heterogeneous hart configurations
- [ ] Shared memory and message-passing between processors
- [ ] Platform config for asymmetric workloads

### Phase 7: Advanced Topics
- [ ] Custom CSR access examples
- [ ] PMU / performance counter usage
- [ ] Cache coherence exploration (gem5)
- [ ] Comparison report across all simulators

---

## 5. Key Design Principles

1. **No OS dependency**: All code is bare-metal. No Linux, no RTOS, no SBI runtime (except for controlled hart boot).
2. **Platform abstraction**: A thin HAL layer isolates platform-specific details (UART base address, memory map, boot protocol).
3. **ELF portability**: A single source tree produces ELFs for different simulators via linker script selection and compile-time `#define`s.
4. **Progressive complexity**: Start with single-core hello-world, build up to SMP + RVV.
5. **Reproducibility**: Everything runs in Docker. CI validates every commit.

---

## 6. Target ISA Configurations

| Configuration | ISA String | Use Case |
|--------------|------------|----------|
| Minimal | `rv64imac` | Basic integer + atomic + compressed |
| Standard | `rv64gc` (= `rv64imafdc`) | Full general-purpose |
| Vector | `rv64gcv` | General-purpose + Vector 1.0 |
| Embedded | `rv32imac` | 32-bit embedded exploration |

The primary target is **RV64GCV** (64-bit, general-purpose with vector extension). RV32 support is secondary and optional.

---

## 7. Decision Log

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Primary ISA | RV64GCV | Covers the most ground for learning; V extension is key goal |
| Primary simulator | QEMU virt machine | Fastest iteration, best tooling support |
| Build system | GNU Make | Simple, widely understood, no extra dependencies |
| Toolchain | riscv64-unknown-elf-gcc | Standard bare-metal toolchain from riscv-gnu-toolchain |
| Container base | Ubuntu 24.04 | LTS, good package availability |

---

## 8. Open Questions for Review

1. **Should we support RV32 targets in Phase 2, or defer to a later phase?**
2. **Do we want to include Renode as an additional simulation platform?**
3. **Should the project include a minimal SBI implementation, or rely entirely on M-mode bare-metal?**
4. **What level of gem5 detail is desired -- SE mode (syscall emulation) or FS mode (full system)?**
5. **Should we include FPGA synthesis targets (e.g., for Arty FPGA with VexRiscv)?**

---

*This document is the entry point for all design proposals. See individual documents (01-06) for detailed designs.*
