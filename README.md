# RISC-V System Simulation Platform

A comprehensive, self-contained learning and development environment for RISC-V system simulation. Build, run, and analyze bare-metal RISC-V applications across multiple simulation platforms.

## Features

- **Multi-platform**: Run the same bare-metal code on QEMU, Spike, gem5 (SE + FS), and Renode
- **Single-core and multi-core (SMP)**: Configurable hart count (1 to 8+)
- **RISC-V Vector Extension (RVV 1.0)**: Progressive vector workloads with VLEN-agnostic code
- **Multi-processor (AMP)**: Heterogeneous configurations on gem5 and Renode
- **CMake + CTest**: Modern build system with presets and comprehensive testing
- **Test-Driven Development**: TDD approach with automated validation
- **Devcontainer**: Fully reproducible development environment
- **CI/CD**: Automated builds, simulation runs, and validation

## Project Status

**Current Phase: Phase 1 - Foundation & Build System** ✅ **COMPLETE**  
**Next Phase: Phase 2 - Single-Core Bare-Metal (QEMU)**

### Completed
- ✅ Phase 0: Design & Setup (comprehensive documentation)
- ✅ Phase 1: CMake build system, CTest framework, toolchain setup scripts, CI integration

### In Progress
- 🔨 Phase 2: Bare-metal application implementation (startup, UART, tests)

> See [ROADMAP.md](ROADMAP.md) for detailed milestones and implementation plan.

## Documentation

| Document | Description |
|----------|-------------|
| [ROADMAP.md](ROADMAP.md) | **⭐ Implementation roadmap with phased milestones** |
| [BUILD.md](BUILD.md) | **⭐ Build system setup and usage guide** |
| [claude.md](claude.md) | AI assistant context and development guidelines |
| [.cursorrules](.cursorrules) | Cursor-specific development rules |

### Design Documents

| Document | Description |
|----------|-------------|
| [00 - Project Overview](docs/00-project-overview.md) | High-level vision, goals, repository structure, and phased roadmap |
| [01 - Platform Assessment](docs/01-platform-assessment.md) | Comprehensive evaluation of RISC-V simulation/emulation platforms (QEMU, Spike, gem5, Renode, etc.) |
| [02 - Bare-Metal Application](docs/02-baremetal-application.md) | Application architecture: boot sequence, HAL, UART/HTIF drivers, CSR access, and module design |
| [03 - Platform Configurations](docs/03-platform-configurations.md) | Single-core, multi-core SMP, and multi-processor AMP configuration designs |
| [04 - RVV Vector Extension](docs/04-rvv-vector-extension.md) | RVV 1.0 learning plan: concepts, instruction categories, progressive workloads, and LMUL exploration |
| [05 - Build System](docs/05-build-system.md) | Build system design (now implemented with CMake) |
| [06 - CI/CD Pipeline](docs/06-ci-cd-pipeline.md) | GitHub Actions workflows for build, simulation, lint, and gem5 |

## Quick Start

### Prerequisites

Use the devcontainer (recommended) or install manually:

```bash
# Using devcontainer (VS Code / Codespaces)
# Just open the project -- the container has everything pre-installed.

# Manual setup
./scripts/setup-toolchain.sh    # Install RISC-V GCC toolchain
./scripts/setup-simulators.sh   # Install QEMU + Spike

# Verify environment
./scripts/verify-environment.sh
```

### Build with CMake

```bash
# Configure with a preset (see CMakePresets.json)
cmake --preset default          # QEMU single-core

# Build
cmake --build build/default

# Test (when Phase 2 is complete)
ctest --test-dir build/default --output-on-failure
```

### Available Presets

```bash
cmake --list-presets            # List all presets

# Common presets:
cmake --preset default          # QEMU single-core
cmake --preset qemu-smp         # QEMU 4-hart SMP
cmake --preset qemu-rvv-256     # QEMU with RVV (VLEN=256)
cmake --preset spike            # Spike ISA simulator
cmake --preset gem5-fs          # gem5 Full System
cmake --preset renode           # Renode
```

See [BUILD.md](BUILD.md) for detailed build instructions.

## Supported Platforms

| Platform | Type | RVV | Multi-Core | Cycle-Accurate | CI | Phase |
|----------|------|-----|------------|----------------|-----|-------|
| **QEMU** | DBT Emulator | Yes | Yes (SMP) | No | ✅ Yes | 2-5 |
| **Spike** | Functional ISA Sim | Yes | Yes (SMP) | No | ✅ Yes | 3 |
| **gem5 SE** | Micro-arch Sim (Syscall Emulation) | Partial | Yes (SMP) | Yes | On merge | 6 |
| **gem5 FS** | Micro-arch Sim (Full System) | Partial | Yes (SMP+AMP) | Yes | On merge | 6 |
| **Renode** | Functional Emulator | Limited | Yes (SMP+AMP) | No | Planned | 7 |

## Target ISA Configurations

| Configuration | ISA String | Description |
|--------------|------------|-------------|
| Minimal | `rv64imac` | Integer + Atomic + Compressed |
| Standard | `rv64gc` | Full general-purpose (= `rv64imafdc`) |
| Vector | `rv64gcv` | General-purpose + Vector 1.0 |

## License

Apache License 2.0. See [LICENSE](LICENSE) for details.
