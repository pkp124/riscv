# RISC-V System Simulation Platform

A comprehensive, self-contained learning and development environment for RISC-V system simulation. Build, run, and analyze bare-metal RISC-V applications across multiple simulation platforms.

## Features

- **Multi-platform**: Run the same bare-metal code on QEMU, Spike, and gem5
- **Single-core and multi-core (SMP)**: Configurable hart count (1 to 8+)
- **RISC-V Vector Extension (RVV 1.0)**: Progressive vector workloads with VLEN-agnostic code
- **Multi-processor (AMP)**: Heterogeneous configurations on gem5
- **Devcontainer**: Fully reproducible development environment
- **CI/CD**: Automated builds, simulation runs, and validation

## Project Status

**Phase: Design Proposals** -- See the `docs/` folder for detailed design documents.

> No application code has been written yet. The design documents are awaiting review and approval before implementation begins.

## Design Documents

| Document | Description |
|----------|-------------|
| [00 - Project Overview](docs/00-project-overview.md) | High-level vision, goals, repository structure, and phased roadmap |
| [01 - Platform Assessment](docs/01-platform-assessment.md) | Comprehensive evaluation of RISC-V simulation/emulation platforms (QEMU, Spike, gem5, Renode, TinyEMU, FireSim, etc.) |
| [02 - Bare-Metal Application](docs/02-baremetal-application.md) | Application architecture: boot sequence, HAL, UART/HTIF drivers, CSR access, and module design |
| [03 - Platform Configurations](docs/03-platform-configurations.md) | Single-core, multi-core SMP, and multi-processor AMP configuration designs |
| [04 - RVV Vector Extension](docs/04-rvv-vector-extension.md) | RVV 1.0 learning plan: concepts, instruction categories, progressive workloads, and LMUL exploration |
| [05 - Build System](docs/05-build-system.md) | GNU Make build system, toolchain setup, compilation flags, and run targets |
| [06 - CI/CD Pipeline](docs/06-ci-cd-pipeline.md) | GitHub Actions workflows for build, simulation, lint, and gem5 |

## Quick Start (After Implementation)

### Prerequisites

Use the devcontainer (recommended) or install manually:

```bash
# Using devcontainer (VS Code / Codespaces)
# Just open the project -- the container has everything pre-installed.

# Manual setup
./scripts/setup-toolchain.sh    # Install RISC-V GCC
./scripts/setup-simulators.sh   # Install QEMU + Spike
```

### Build

```bash
cd app

# Default: QEMU virt, single-core
make

# QEMU with 4-hart SMP
make PLATFORM=qemu CONFIG=smp NUM_HARTS=4

# QEMU with RVV (Vector Extension)
make PLATFORM=qemu CONFIG=single RVV=1

# Build all configurations
make all-platforms
```

### Run

```bash
# Run on QEMU
make run-qemu

# Run on Spike
make run-spike

# Run on QEMU with GDB debugging
make run-qemu-debug
# Then in another terminal: riscv64-unknown-elf-gdb -ex "target remote :1234" build/qemu-single/app.elf
```

## Supported Platforms

| Platform | Type | RVV | Multi-Core | Cycle-Accurate | CI |
|----------|------|-----|------------|----------------|-----|
| **QEMU** | DBT Emulator | Yes | Yes (SMP) | No | Yes |
| **Spike** | Functional ISA Sim | Yes | Yes (SMP) | No | Yes |
| **gem5** | Micro-arch Sim | Partial | Yes (SMP+AMP) | Yes | On merge |
| **Renode** | Functional Emulator | Limited | Yes (SMP+AMP) | No | Planned |

## Target ISA Configurations

| Configuration | ISA String | Description |
|--------------|------------|-------------|
| Minimal | `rv64imac` | Integer + Atomic + Compressed |
| Standard | `rv64gc` | Full general-purpose (= `rv64imafdc`) |
| Vector | `rv64gcv` | General-purpose + Vector 1.0 |

## License

Apache License 2.0. See [LICENSE](LICENSE) for details.
