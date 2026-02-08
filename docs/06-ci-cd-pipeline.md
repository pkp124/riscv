# Design Proposal 06: CI/CD Pipeline Design

## 1. Overview

This document describes the CI/CD pipeline for the RISC-V bare-metal project. The pipeline will:

- Build all ELF configurations on every push/PR
- Run functional simulations on QEMU and Spike
- Perform static analysis and formatting checks
- Validate simulation outputs
- Cache toolchain and simulator installations

---

## 2. Pipeline Architecture

```
                    ┌──────────────────────────────────────────────┐
                    │               GitHub Actions CI               │
                    │                                              │
  Push/PR ─────────┤  ┌────────────┐  ┌────────────┐  ┌────────┐ │
                    │  │   Lint &   │  │   Build    │  │  Sim   │ │
                    │  │   Format   │  │   Matrix   │  │  Test  │ │
                    │  │            │  │            │  │        │ │
                    │  │ - cppcheck │  │ - qemu-sc  │  │ - QEMU │ │
                    │  │ - clang-   │  │ - qemu-smp │  │ - Spike│ │
                    │  │   format   │  │ - qemu-rvv │  │        │ │
                    │  │            │  │ - spike-sc │  │        │ │
                    │  │            │  │ - spike-smp│  │        │ │
                    │  │            │  │ - gem5-sc  │  │        │ │
                    │  └────────────┘  └────────────┘  └────────┘ │
                    │        │               │              │      │
                    │        └───────┬───────┘              │      │
                    │                │                      │      │
                    │                ▼                      │      │
                    │         ┌──────────┐                  │      │
                    │         │ Artifact │◄─────────────────┘      │
                    │         │  Upload  │                         │
                    │         └──────────┘                         │
                    └──────────────────────────────────────────────┘
```

---

## 3. Workflow Definitions

### 3.1 Main CI Workflow (ci-build.yml)

```yaml
name: CI - Build and Test

on:
  push:
    branches: [main, 'feature/**']
  pull_request:
    branches: [main]

env:
  RISCV_TOOLCHAIN_VERSION: "2024.04.12"

jobs:
  # ──────────────────────────────────────────────
  # Job 1: Lint and Format Check
  # ──────────────────────────────────────────────
  lint:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4

      - name: Install lint tools
        run: |
          sudo apt-get update
          sudo apt-get install -y cppcheck clang-format

      - name: Check formatting
        run: |
          find app/src app/include -name '*.c' -o -name '*.h' | \
            xargs clang-format --dry-run --Werror

      - name: Run cppcheck
        run: |
          cppcheck --enable=all --std=c11 \
            --suppress=missingIncludeSystem \
            --suppress=unusedFunction \
            --error-exitcode=1 \
            -I app/include/ app/src/

  # ──────────────────────────────────────────────
  # Job 2: Build Matrix
  # ──────────────────────────────────────────────
  build:
    runs-on: ubuntu-24.04
    needs: lint
    strategy:
      fail-fast: false
      matrix:
        include:
          - name: "QEMU Single-Core"
            platform: qemu
            config: single
            rvv: 0
            num_harts: 1
          - name: "QEMU SMP (4 harts)"
            platform: qemu
            config: smp
            rvv: 0
            num_harts: 4
          - name: "QEMU RVV"
            platform: qemu
            config: single
            rvv: 1
            num_harts: 1
          - name: "QEMU SMP+RVV"
            platform: qemu
            config: smp
            rvv: 1
            num_harts: 4
          - name: "Spike Single-Core"
            platform: spike
            config: single
            rvv: 0
            num_harts: 1
          - name: "Spike SMP (4 harts)"
            platform: spike
            config: smp
            rvv: 0
            num_harts: 4
          - name: "Spike RVV"
            platform: spike
            config: single
            rvv: 1
            num_harts: 1
          - name: "gem5 Single-Core"
            platform: gem5
            config: single
            rvv: 0
            num_harts: 1

    steps:
      - uses: actions/checkout@v4

      - name: Cache RISC-V Toolchain
        uses: actions/cache@v4
        id: cache-toolchain
        with:
          path: /opt/riscv
          key: riscv-toolchain-${{ env.RISCV_TOOLCHAIN_VERSION }}-${{ runner.os }}

      - name: Install RISC-V Toolchain
        if: steps.cache-toolchain.outputs.cache-hit != 'true'
        run: |
          sudo apt-get update
          sudo apt-get install -y gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf
          # Or download prebuilt:
          # ./scripts/setup-toolchain.sh

      - name: Build
        working-directory: app
        run: |
          make PLATFORM=${{ matrix.platform }} \
               CONFIG=${{ matrix.config }} \
               NUM_HARTS=${{ matrix.num_harts }} \
               RVV=${{ matrix.rvv }}

      - name: Upload ELF artifact
        uses: actions/upload-artifact@v4
        with:
          name: elf-${{ matrix.platform }}-${{ matrix.config }}-rvv${{ matrix.rvv }}
          path: app/build/*/app.elf

  # ──────────────────────────────────────────────
  # Job 3: Simulation Tests
  # ──────────────────────────────────────────────
  simulate:
    runs-on: ubuntu-24.04
    needs: build
    strategy:
      fail-fast: false
      matrix:
        include:
          - name: "QEMU Single-Core"
            platform: qemu
            config: single
            rvv: 0
            num_harts: 1
            sim_cmd: >-
              qemu-system-riscv64 -machine virt -smp 1 -m 128M
              -nographic -bios none -kernel app.elf
          - name: "QEMU SMP"
            platform: qemu
            config: smp
            rvv: 0
            num_harts: 4
            sim_cmd: >-
              qemu-system-riscv64 -machine virt -smp 4 -m 256M
              -nographic -bios none -kernel app.elf
          - name: "QEMU RVV"
            platform: qemu
            config: single
            rvv: 1
            num_harts: 1
            sim_cmd: >-
              qemu-system-riscv64 -machine virt
              -cpu rv64,v=true,vlen=256
              -nographic -bios none -kernel app.elf

    steps:
      - uses: actions/checkout@v4

      - name: Download ELF artifacts
        uses: actions/download-artifact@v4
        with:
          name: elf-${{ matrix.platform }}-${{ matrix.config }}-rvv${{ matrix.rvv }}
          path: artifacts/

      - name: Install QEMU
        if: matrix.platform == 'qemu'
        run: |
          sudo apt-get update
          sudo apt-get install -y qemu-system-misc

      - name: Install Spike
        if: matrix.platform == 'spike'
        run: |
          # Build spike from source or use cached version
          ./scripts/setup-simulators.sh

      - name: Run Simulation
        timeout-minutes: 5
        run: |
          ELF=$(find artifacts/ -name 'app.elf' | head -1)
          echo "Running: ${{ matrix.sim_cmd }}"
          # Replace 'app.elf' in command with actual path
          CMD=$(echo "${{ matrix.sim_cmd }}" | sed "s|app.elf|${ELF}|")

          # Run with timeout (simulation should complete quickly)
          timeout 60 ${CMD} > output.log 2>&1 || true

          echo "=== Simulation Output ==="
          cat output.log

      - name: Validate Output
        run: |
          # Check for expected strings in output
          grep -q "RISC-V Bare-Metal System Explorer" output.log
          grep -q "All tests complete" output.log
          echo "Validation PASSED"

      - name: Upload simulation log
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: sim-log-${{ matrix.platform }}-${{ matrix.config }}-rvv${{ matrix.rvv }}
          path: output.log
```

### 3.2 Devcontainer Build Check (ci-devcontainer.yml)

```yaml
name: CI - Devcontainer Build

on:
  push:
    paths:
      - '.devcontainer/**'
  pull_request:
    paths:
      - '.devcontainer/**'

jobs:
  build-devcontainer:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4

      - name: Build devcontainer image
        run: |
          docker build -t riscv-dev .devcontainer/

      - name: Verify toolchain in container
        run: |
          docker run --rm riscv-dev riscv64-unknown-elf-gcc --version

      - name: Verify QEMU in container
        run: |
          docker run --rm riscv-dev qemu-system-riscv64 --version
```

---

## 4. CI Environment Requirements

### 4.1 Ubuntu 24.04 Runner Packages

```bash
# Build tools
apt-get install -y build-essential git make

# RISC-V toolchain
apt-get install -y gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf

# QEMU
apt-get install -y qemu-system-misc

# Lint tools
apt-get install -y cppcheck clang-format

# Spike dependencies (build from source)
apt-get install -y device-tree-compiler libboost-regex-dev libboost-system-dev

# Python (for test validation scripts)
apt-get install -y python3 python3-pip
```

### 4.2 Caching Strategy

| Cache | Key | Path | Size Estimate |
|-------|-----|------|---------------|
| RISC-V Toolchain | `riscv-toolchain-{version}-{os}` | `/opt/riscv` | ~500 MB |
| Spike | `spike-{commit}-{os}` | `/opt/riscv/bin/spike` | ~10 MB |
| gem5 | `gem5-{commit}-{os}` | `/opt/gem5/build/RISCV/gem5.opt` | ~500 MB |
| apt packages | `apt-{hash}` | N/A (use apt cache action) | Varies |

---

## 5. Simulation Timeout Strategy

Bare-metal programs on simulators should complete quickly. If they hang (e.g., due to a bug), we need timeouts:

| Simulator | Expected Duration | Timeout |
|-----------|------------------|---------|
| QEMU (single) | < 2 seconds | 30 seconds |
| QEMU (SMP) | < 5 seconds | 60 seconds |
| Spike (single) | < 5 seconds | 60 seconds |
| Spike (SMP) | < 10 seconds | 120 seconds |
| gem5 (atomic) | < 30 seconds | 300 seconds |
| gem5 (timing) | < 5 minutes | 600 seconds |

The application itself will include a watchdog/timeout mechanism to self-terminate after completing all tests.

---

## 6. Output Validation

### 6.1 Expected Output Patterns

```python
# tests/test_output.py
"""Validate simulation output against expected patterns."""

import re
import sys

REQUIRED_PATTERNS = [
    r"RISC-V Bare-Metal System Explorer",
    r"Hart: 0",
    r"ISA Extensions: .*[IMAFDC]",
    r"All tests complete",
]

SMP_PATTERNS = [
    r"Hart \d+ online",
    r"Parallel .* PASS",
]

RVV_PATTERNS = [
    r"Vector extension \(V\) detected",
    r"VLEN = \d+",
    r"\[RVV\] .* PASS",
]

def validate(output_file, config):
    with open(output_file) as f:
        output = f.read()

    patterns = REQUIRED_PATTERNS.copy()
    if 'smp' in config:
        patterns.extend(SMP_PATTERNS)
    if 'rvv' in config:
        patterns.extend(RVV_PATTERNS)

    failed = []
    for pattern in patterns:
        if not re.search(pattern, output):
            failed.append(pattern)

    if failed:
        print(f"FAIL: Missing patterns: {failed}")
        sys.exit(1)
    else:
        print("PASS: All expected patterns found.")
```

### 6.2 Cross-Platform Output Comparison

```bash
# scripts/compare-outputs.sh
# Compare outputs from QEMU and Spike to ensure functional equivalence

diff <(grep '^\[TEST\]' qemu-output.log | sort) \
     <(grep '^\[TEST\]' spike-output.log | sort)
```

---

## 7. Artifact Management

### 7.1 Build Artifacts

Each build produces:
- `app.elf` -- The ELF binary
- `app.dump` -- Disassembly listing
- Build log

### 7.2 Simulation Artifacts

Each simulation run produces:
- `output.log` -- Console output
- Validation result (PASS/FAIL)

### 7.3 Retention

| Artifact Type | Retention |
|--------------|-----------|
| ELF binaries | 30 days |
| Simulation logs | 30 days |
| gem5 statistics | 90 days |

---

## 8. gem5 CI Considerations

gem5 is expensive to build and slow to simulate. Options:

1. **Separate workflow**: Run gem5 tests only on main branch merges, not PRs
2. **Scheduled**: Run gem5 tests nightly
3. **Manual**: Trigger gem5 tests manually via `workflow_dispatch`

Recommended: Option 1 (on merge to main) + Option 3 (manual trigger).

```yaml
# In ci-gem5.yml
on:
  push:
    branches: [main]
  workflow_dispatch:
    inputs:
      cpu_type:
        description: 'gem5 CPU type'
        default: 'AtomicSimpleCPU'
        type: choice
        options:
          - AtomicSimpleCPU
          - TimingSimpleCPU
          - MinorCPU
```

---

## 9. Branch Protection Rules (Recommended)

| Rule | Setting |
|------|---------|
| Require status checks | `lint`, `build`, `simulate` jobs must pass |
| Require branches up to date | Yes |
| Require PR reviews | 1 reviewer minimum |
| Dismiss stale reviews | Yes |

---

## 10. Future CI Enhancements

1. **Performance regression tracking**: Store gem5 cycle counts and alert on regressions
2. **Coverage analysis**: Track which ISA instructions are exercised by our tests
3. **Nightly builds**: Build from latest QEMU/Spike/gem5 source to catch upstream changes
4. **Multi-arch testing**: Add RV32 build targets
5. **Badge reporting**: Display build status and simulation results as badges

---

## 11. Open Questions

1. **Should CI use the devcontainer image or install tools directly?** Both approaches have merit. Using the devcontainer image ensures consistency but adds Docker overhead.
2. **How to handle gem5's long build time in CI?** Pre-built gem5 docker image or weekly cache rebuild.
3. **Should we publish ELF artifacts to GitHub Releases?** Useful for users who want to test without building.
4. **Do we need integration tests that cross-validate QEMU vs Spike output?** Yes, this is valuable for correctness.

---

*This concludes the design proposal series. See [00-project-overview.md](00-project-overview.md) for the project overview and roadmap.*
