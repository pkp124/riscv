# Design Proposal 01: RISC-V Simulation & Emulation Platform Assessment

## 1. Executive Summary

This document provides a comprehensive assessment of available RISC-V simulation and emulation platforms. Each platform is evaluated on: functional correctness, performance, ISA extension support (especially RVV), multi-core/multi-processor capability, ease of use, and suitability for our learning goals.

---

## 2. Platform Catalog

### 2.1 QEMU (Quick EMUlator)

| Attribute | Details |
|-----------|---------|
| **Type** | Dynamic binary translation (DBT) emulator |
| **URL** | https://www.qemu.org/ |
| **License** | GPLv2 |
| **RISC-V Support** | Full-system (`qemu-system-riscv64/32`) and user-mode (`qemu-riscv64`) |
| **ISA Extensions** | RV32/64, IMAFDC, V (1.0), H, Zb*, Zicsr, Zifencei, and many more |
| **RVV Support** | Yes -- RVV 1.0 (since QEMU 7.0+). Configurable via `-cpu rv64,v=true,vlen=256` |
| **Multi-Core** | Yes -- `-smp N` flag, up to 512 harts in virt machine |
| **Multi-Processor** | Partially -- multiple NUMA nodes possible, but limited AMP |
| **Machine Models** | `virt` (generic), `sifive_u`, `sifive_e`, `microchip-icicle-kit`, `spike` |
| **GDB Support** | Yes -- built-in GDB stub (`-s -S`) |
| **Performance** | Fast (DBT), ~100-500 MIPS depending on workload |
| **Tracing** | TCG plugins, `-d` flags for instruction/register trace |
| **Ease of Setup** | Excellent -- available via `apt install qemu-system-riscv64` |

**Strengths:**
- Fastest functional simulation
- Excellent RVV 1.0 support with configurable VLEN
- Rich machine models (especially `virt` for bare-metal)
- GDB debugging support
- Active development, strong community

**Weaknesses:**
- Not cycle-accurate (no micro-architecture modeling)
- Limited cache/memory hierarchy visibility
- No performance counters or pipeline statistics

**Our Use:**
- **Primary development platform** for fast iteration
- All bare-metal code will be validated on QEMU first
- SMP testing with `-smp` flag
- RVV workloads with configurable vector length

**Key QEMU Commands:**
```bash
# Single-core bare-metal
qemu-system-riscv64 -machine virt -nographic -bios none -kernel app.elf

# Multi-core (4 harts)
qemu-system-riscv64 -machine virt -smp 4 -nographic -bios none -kernel app.elf

# With RVV, VLEN=256
qemu-system-riscv64 -machine virt -cpu rv64,v=true,vlen=256 \
    -nographic -bios none -kernel app.elf

# With GDB
qemu-system-riscv64 -machine virt -nographic -bios none -kernel app.elf -s -S
```

---

### 2.2 Spike (RISC-V ISA Simulator)

| Attribute | Details |
|-----------|---------|
| **Type** | Functional ISA simulator (reference implementation) |
| **URL** | https://github.com/riscv-software-src/riscv-isa-sim |
| **License** | BSD 3-Clause |
| **RISC-V Support** | RV32/64/128 |
| **ISA Extensions** | All ratified extensions including V, H, Zb*, Zk*, custom extensions |
| **RVV Support** | Yes -- reference implementation of RVV 1.0. `--isa=rv64gcv --varch=vlen:256,elen:64` |
| **Multi-Core** | Yes -- `-p N` for N harts |
| **Multi-Processor** | Limited -- all harts share same config |
| **GDB Support** | Yes -- via `--rbb-port` or OpenOCD integration |
| **Performance** | Moderate (~10-100 MIPS) |
| **Tracing** | Excellent -- `--log` flag with commit log, instruction trace |
| **Ease of Setup** | Moderate -- must build from source (no apt package typically) |

**Strengths:**
- **Official RISC-V reference simulator** -- definitive ISA correctness
- Supports ALL ratified and draft extensions
- Excellent instruction-level tracing and logging
- Simple and lightweight
- Can serve as "golden reference" for correctness checking

**Weaknesses:**
- No cycle accuracy
- No peripheral/device modeling (HTIF only)
- Bare-metal programs need HTIF (Host-Target Interface) or proxy kernel (pk)
- Limited machine model (no UART, no virtio)

**Our Use:**
- **Reference simulator** for ISA correctness validation
- Instruction trace comparison across platforms
- RVV extension testing with reference behavior
- Multi-hart testing

**Key Spike Commands:**
```bash
# Single core
spike --isa=rv64gc app.elf

# Multi-core (4 harts)
spike --isa=rv64gc -p4 app.elf

# With RVV
spike --isa=rv64gcv --varch=vlen:256,elen:64 app.elf

# With instruction log
spike --isa=rv64gc --log-commits app.elf

# Interactive debug
spike -d --isa=rv64gc app.elf
```

**Note on Spike I/O:**
Spike uses HTIF (Host-Target Interface) for I/O, not memory-mapped UART. Our bare-metal app must either:
1. Use HTIF syscalls for output (Spike-specific), or
2. Use the `pk` (proxy kernel) which provides syscall emulation

We will implement a thin platform abstraction that handles both HTIF (Spike) and MMIO UART (QEMU/gem5).

---

### 2.3 gem5

| Attribute | Details |
|-----------|---------|
| **Type** | Cycle-accurate micro-architectural simulator |
| **URL** | https://www.gem5.org/ |
| **License** | BSD 3-Clause |
| **RISC-V Support** | Yes -- SE (syscall emulation) and FS (full system) modes |
| **ISA Extensions** | RV64GC (V extension support is experimental/in-progress) |
| **RVV Support** | Partial/Experimental -- community patches exist, not fully upstream |
| **Multi-Core** | Yes -- configurable via Python scripts |
| **Multi-Processor** | Yes -- full heterogeneous configurations possible |
| **CPU Models** | AtomicSimpleCPU, TimingSimpleCPU, MinorCPU, O3CPU (out-of-order) |
| **Cache Hierarchy** | Full: L1I/L1D, L2, L3, directory/MOESI coherence |
| **GDB Support** | Yes |
| **Performance** | Slow (~0.1-10 KIPS for detailed O3, ~1 MIPS for atomic) |
| **Tracing** | Extensive -- instruction trace, pipeline trace, cache statistics |
| **Ease of Setup** | Complex -- must build from source, Python configuration |

**Strengths:**
- **Cycle-accurate** micro-architecture simulation
- Detailed cache hierarchy and memory system modeling
- Pipeline visualization and performance analysis
- Multiple CPU models (in-order, out-of-order)
- Research-grade, used in academic computer architecture
- Full heterogeneous multi-core support

**Weaknesses:**
- Very slow simulation speed
- Complex setup and configuration
- RVV support is not fully mature
- RISC-V FS mode requires careful configuration
- Steep learning curve

**Our Use:**
- **Performance analysis platform** for micro-architectural exploration
- Cache behavior analysis
- Pipeline stall and hazard analysis
- Multi-core coherence studies
- Comparison of in-order vs out-of-order execution

**gem5 CPU Model Comparison:**

| Model | Type | Speed | Fidelity | Use Case |
|-------|------|-------|----------|----------|
| AtomicSimpleCPU | Functional | Fast | Low | Quick functional testing |
| TimingSimpleCPU | Timing | Medium | Medium | Memory system timing |
| MinorCPU | In-order pipeline | Slow | High | In-order core analysis |
| O3CPU | Out-of-order | Very slow | Very high | OoO core analysis |

**Key gem5 Usage:**
```bash
# SE mode (syscall emulation) - simpler
build/RISCV/gem5.opt configs/example/se.py \
    --cpu-type=MinorCPU --cmd=app.elf

# FS mode (full system) - what we want
build/RISCV/gem5.opt configs/riscv_fs.py \
    --cpu-type=TimingSimpleCPU --num-cpus=4 \
    --kernel=app.elf

# With cache hierarchy
build/RISCV/gem5.opt configs/riscv_fs.py \
    --cpu-type=MinorCPU \
    --l1d_size=32kB --l1i_size=32kB \
    --l2_size=256kB --num-cpus=2 \
    --kernel=app.elf
```

---

### 2.4 Renode

| Attribute | Details |
|-----------|---------|
| **Type** | Functional emulator with peripheral modeling |
| **URL** | https://renode.io/ |
| **License** | MIT |
| **RISC-V Support** | RV32/64 |
| **ISA Extensions** | IMAFDC (V extension support limited) |
| **RVV Support** | Limited/Experimental |
| **Multi-Core** | Yes -- multi-hart and multi-machine |
| **Multi-Processor** | Yes -- can model multiple independent CPUs with interconnects |
| **GDB Support** | Yes |
| **Performance** | Moderate |
| **Tracing** | Good -- logging framework, Wireshark integration for network |
| **Ease of Setup** | Good -- distributed as .NET portable package or via apt |

**Strengths:**
- Excellent **peripheral and device modeling** (UART, SPI, I2C, Ethernet, etc.)
- **Multi-machine** simulation (model entire SoCs and boards)
- **Platform description files** (.repl) for SoC modeling
- Good for hardware/software co-development
- Robot Framework integration for automated testing
- Models real hardware platforms (SiFive, LiteX, etc.)

**Weaknesses:**
- Not cycle-accurate
- RVV support is behind QEMU/Spike
- Smaller community than QEMU for RISC-V
- .NET runtime dependency

**Our Use:**
- **SoC/platform modeling** for realistic peripheral interaction
- Multi-processor (AMP) configurations
- Automated test framework integration
- Alternative validation platform

---

### 2.5 TinyEMU (RISC-V Emulator by Fabrice Bellard)

| Attribute | Details |
|-----------|---------|
| **Type** | Lightweight system emulator |
| **URL** | https://bellard.org/tinyemu/ |
| **License** | MIT |
| **RISC-V Support** | RV32/64 |
| **ISA Extensions** | IMAFDC |
| **RVV Support** | No |
| **Multi-Core** | Limited |
| **GDB Support** | No |
| **Performance** | Fast (JIT-based) |
| **Ease of Setup** | Easy -- single C file |

**Strengths:**
- Extremely lightweight and fast
- Can run in browser (via Emscripten)
- Good for quick Linux boot experiments

**Weaknesses:**
- No RVV support
- Limited extensibility
- No GDB support
- Not maintained actively

**Our Use:**
- Optional / for reference only
- Interesting for understanding emulator internals (small codebase)

---

### 2.6 RARS (RISC-V Assembler and Runtime Simulator)

| Attribute | Details |
|-----------|---------|
| **Type** | Educational ISA simulator (Java-based) |
| **URL** | https://github.com/TheThirdOne/rars |
| **License** | MIT |
| **RISC-V Support** | RV32/64 (subset) |
| **RVV Support** | No |
| **Multi-Core** | No |

**Our Use:** Not suitable for our goals (educational only, no bare-metal support).

---

### 2.7 SAIL RISC-V Model

| Attribute | Details |
|-----------|---------|
| **Type** | Formal ISA specification model |
| **URL** | https://github.com/riscv/sail-riscv |
| **License** | BSD 2-Clause |
| **Purpose** | Formal specification, not simulation |
| **RVV Support** | Yes (formal spec) |

**Our Use:** Reference for understanding ISA semantics. Not a simulation target.

---

### 2.8 FireSim

| Attribute | Details |
|-----------|---------|
| **Type** | FPGA-accelerated cycle-accurate simulation |
| **URL** | https://fires.im/ |
| **License** | BSD 3-Clause |
| **RISC-V Support** | Full (based on BOOM/Rocket cores via Chipyard) |
| **RVV Support** | Yes (via Ara or Saturn vector units in Chipyard) |
| **Multi-Core** | Yes (full SoC configurations) |
| **Performance** | ~10-100 MHz cycle-accurate simulation |

**Strengths:**
- Cycle-accurate at near-FPGA speeds
- Models real RISC-V cores (Rocket, BOOM)
- Integrates with Chipyard ecosystem

**Weaknesses:**
- Requires AWS F1 FPGA instances (costly) or local FPGA boards
- Very complex setup
- Not suitable for quick iteration

**Our Use:** Out of scope for initial phases. Documented for completeness and future exploration.

---

### 2.9 Dromajo

| Attribute | Details |
|-----------|---------|
| **Type** | RISC-V reference model for co-simulation |
| **URL** | https://github.com/chipsalliance/dromajo |
| **License** | Apache 2.0 |
| **Purpose** | RTL co-simulation / trace-driven verification |
| **RVV Support** | Partial |
| **Multi-Core** | Yes |

**Our Use:** Interesting for RTL verification workflows. Lower priority for our goals.

---

## 3. Comparative Matrix

| Feature | QEMU | Spike | gem5 | Renode | TinyEMU | FireSim |
|---------|------|-------|------|--------|---------|---------|
| **Simulation Type** | DBT | Functional | Cycle-accurate | Functional | DBT/JIT | FPGA-accelerated |
| **Speed** | Very Fast | Fast | Very Slow | Moderate | Fast | Fast (cycle-accurate) |
| **RV64GC** | Yes | Yes | Yes | Yes | Yes | Yes |
| **RVV 1.0** | Yes | Yes | Partial | Limited | No | Yes |
| **Multi-Core** | Yes | Yes | Yes | Yes | Limited | Yes |
| **Multi-Processor** | Limited | No | Yes | Yes | No | Yes |
| **Cycle Accuracy** | No | No | Yes | No | No | Yes |
| **Cache Modeling** | No | No | Yes | No | No | Yes |
| **Peripheral Model** | Good | HTIF only | Basic | Excellent | Basic | Full SoC |
| **GDB** | Yes | Yes | Yes | Yes | No | Yes |
| **Ease of Setup** | Easy | Moderate | Hard | Moderate | Easy | Very Hard |
| **CI Friendly** | Yes | Yes | Possible | Yes | Yes | No |
| **Cost** | Free | Free | Free | Free | Free | FPGA $$ |

---

## 4. Recommended Platform Strategy

### Tier 1: Primary Platforms (Must Have)

1. **QEMU** -- Primary development and testing platform
   - Fastest iteration cycle
   - Best RVV support
   - Excellent for SMP testing
   - CI-friendly

2. **Spike** -- Reference/golden model
   - ISA correctness verification
   - Instruction trace generation
   - Official RISC-V Foundation reference

### Tier 2: Analysis Platforms (Should Have)

3. **gem5** -- Micro-architectural analysis
   - Pipeline and cache behavior
   - Performance modeling
   - Multi-core coherence studies

### Tier 3: Extended Platforms (Nice to Have)

4. **Renode** -- SoC/peripheral modeling
   - Multi-processor configurations
   - Realistic peripheral interaction

5. **TinyEMU** -- Lightweight alternative (reference only)

### Tier 4: Future Exploration (Out of Scope for Now)

6. **FireSim** -- FPGA-accelerated simulation
7. **Dromajo** -- RTL co-simulation

---

## 5. Platform-Specific Considerations for Bare-Metal

### Memory Maps

Each simulator expects different memory layouts:

| Platform | RAM Base | RAM Size (default) | UART Base | Boot Address |
|----------|----------|-------------------|-----------|-------------|
| QEMU virt | 0x80000000 | 128 MiB | 0x10000000 (NS16550A) | 0x80000000 |
| Spike | 0x80000000 | 2 GiB | HTIF (not MMIO) | 0x80000000 |
| gem5 (FS) | 0x80000000 | Configurable | 0x10000000 | 0x80000000 |
| Renode | Configurable | Configurable | Configurable | Configurable |

The common base address of `0x80000000` across QEMU, Spike, and gem5 simplifies our linker scripts significantly.

### I/O Abstraction

- **QEMU virt**: NS16550A UART at `0x10000000`
- **Spike**: HTIF tohost/fromhost mechanism
- **gem5**: NS16550A or 8250 UART (configurable)
- **Renode**: Platform-dependent

Our platform abstraction layer will handle these differences at compile time via `#ifdef` guards.

---

## 6. Installation Complexity Assessment

| Platform | Install Method | Time to Setup | Dependencies |
|----------|---------------|---------------|-------------|
| QEMU | `apt install` or build from source | 5 min (apt) / 30 min (source) | Standard build tools |
| Spike | Build from source | 15 min | device-tree-compiler, boost |
| gem5 | Build from source | 30-60 min | Python 3, SCons, protobuf, many C++ deps |
| Renode | .NET package or apt (on supported distros) | 10 min | .NET runtime |
| Toolchain | Download prebuilt or build | 5 min (prebuilt) / 2+ hrs (source) | Standard build tools |

All of these will be automated in our Dockerfile and setup scripts.

---

## 7. Open Questions

1. Should we invest in gem5 RVV support (may require patching gem5)?
2. Is Renode worth including in CI given its limited RVV support?
3. Should we build QEMU from source (latest RVV fixes) or use distro packages?
4. Do we want to include `riscv-tests` suite as a validation baseline?

---

*Next: See [02-baremetal-application.md](02-baremetal-application.md) for the bare-metal application design.*
