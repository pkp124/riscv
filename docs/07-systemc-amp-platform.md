# Design Proposal 07: SystemC AMP Platform with Spike Instances

## 1. Requirements Review and Critical Analysis

### 1.1 Stated Requirements

The request is to build a **SystemC virtual platform** that:

1. Consists of **multiple Spike ISS instances** (RISC-V hart clusters)
2. Has a **bus with multiple levels of memory** (L1/L2 cache hierarchy)
3. Has **shared memory** regions accessible from all processor clusters
4. Has **peripherals** sufficient to test:
   - Interrupt delivery and handling
   - Cross-processor synchronization
   - Multi-level memory access patterns
5. Is **generated from a system description** (device tree or similar)
6. Supports **future extensibility** with additional SystemC components
7. Targets **AMP subsystem development** and associated firmware

### 1.2 Critical Assessment of Requirements

#### What is solid

The core architectural vision is sound and well-motivated:

- **Spike as ISS backend**: Spike is the RISC-V golden reference model. Its instruction semantics are authoritative, it handles all privilege levels correctly, and it has a documented MMIO plugin interface (`abstract_device_t`) and a library mode (`libriscv.so`) that makes embedding feasible.
- **SystemC/TLM-2.0 as integration layer**: TLM-2.0 is the standard for virtual platform assembly. Using it means peripherals, memories, and interconnects are reusable across projects and can interoperate with existing open-source components.
- **Description-driven generation**: This is the right long-term architectural choice. Manually wiring SC_MODULE instantiations is error-prone and makes topology changes costly. A description file decouples the platform topology from the C++ wiring code.
- **AMP firmware development target**: Spike-in-SystemC is better suited to AMP firmware development than raw Spike (`-pN`) because you get per-cluster memory maps, heterogeneous ISA strings per cluster, independent interrupt controllers, and the ability to inject software-controlled stimuli from the SystemC elaboration layer.

#### Where the requirements need refinement

**Requirement: "Multiple Spike instances"**

Spike's internal architecture was designed for SMP (all harts share one `sim_t` context, one physical memory, one CLINT). Running `spike -p4` creates four harts inside one simulation context — not four independent Spike processes. For AMP you need **independent Spike contexts** with separate address spaces and separate peripheral maps, then connected through the SystemC bus. This is not a supported Spike mode out of the box. Two approaches exist:

- **Approach A (Recommended)**: One `sim_t` (Spike simulation context) per processor cluster. Each `sim_t` has its own memory map. The SystemC bus bridges between them by routing transactions from one cluster's address space through the TLM fabric to a shared memory or peripheral model.
- **Approach B**: Use Spike's MMIO plugin interface to intercept all cross-cluster accesses at the Spike level and redirect them into SystemC. This is fragile and bypasses TLM.

The design in this document uses **Approach A**.

**Requirement: "Multiple levels of memories"**

This has two distinct interpretations:

1. **Cache hierarchy modeling** (L1/L2/L3): Spike does not model caches at all — it executes instructions and makes memory requests that hit flat physical memory. True cache hierarchy requires a cycle-accurate simulator (gem5, RTL) or a SystemC cache model sitting between the Spike context and the memory bus. A SystemC LRU cache model can be layered in TLM-2.0 terms as an initiator-target pair, but it adds significant complexity and slows simulation markedly.
2. **Address-space regions at different latencies** (fast scratchpad, slow DRAM): This is straightforward in TLM and is the recommended starting point. Model a fast scratchpad at one address range and slower DRAM at another, each with a configurable TLM delay.

The initial implementation should use **option 2** (latency-differentiated regions). Cache hierarchy modeling can be added as a Phase 9/10 item.

**Requirement: "Generated from device tree"**

Standard Linux device tree (DTS/DTB) is designed to describe a real hardware platform for a bootloader/OS — it does not carry simulation parameters like TLM timing annotations, cache model types, or SystemC quantum sizes. Three practical choices exist:

| Format | Pros | Cons |
|--------|------|------|
| **YAML (custom schema)** | Human-readable, Python-parseable, easy to extend with simulation-specific fields | Not re-using existing tooling |
| **DTS subset + extensions** | Familiar to firmware engineers; Lopper can process it | `dtc` rejects unknown properties; toolchain friction |
| **System Device Tree (OpenAMP spec)** | Standard for AMP domain description; Lopper generates per-domain DTS | Complex spec; currently focused on real hardware, not simulation |

**Recommendation**: Use a **YAML description** with a schema designed to be transpilable to Linux DTS for firmware testing, but with simulation-specific extensions (TLM timing, quantum, cache model configuration). The YAML is processed by a Python generator that emits C++ SystemC top-level wiring code.

**Requirement: "Future extensibility with more SystemC components"**

This is achievable if the initial design follows a component registry pattern: each component type (memory, UART, PLIC, Spike cluster, etc.) registers itself with the generator, and the YAML description references component types by name. Adding a new component requires only:
1. A new `SC_MODULE` implementing the TLM interface
2. A registration entry in the generator
3. New YAML properties documented in the schema

---

## 2. Existing Open Source Projects Survey

### 2.1 Directly Relevant Projects

#### spike-vp (TommyWu-fdgkhdkgh)
- **URL**: https://github.com/TommyWu-fdgkhdkgh/spike-vp
- **What it does**: Wraps Spike inside a VCML (Virtual Components Modeling Library) `sc_module`. Connects Spike's memory access callbacks to VCML's TLM-2.0 socket infrastructure. Fast enough to boot Linux (25 minutes → 160 seconds after DMI optimization).
- **Relevance**: The closest existing implementation to what is needed. Single-processor only. No platform description. No AMP topology.
- **License**: Apache-2.0
- **Assessment**: A strong reference implementation for the Spike↔TLM integration layer. The memory access interception strategy from this project should be adopted.

#### VCML (MachineWare GmbH)
- **URL**: https://github.com/machineware-gmbh/vcml
- **What it does**: A SystemC/TLM-2.0 productivity library. Provides: TLM initiator/target sockets with built-in tracing, interrupt port abstractions, register model framework, ready-to-use peripherals (NS16550A UART, ARM PL011, GIC, PLIC, CLINT, SPI, I2C, Ethernet, etc.), a hierarchical bus/crossbar model, and a quantum keeper.
- **Relevance**: This is the component library to build on. It eliminates the need to write PLIC, CLINT, UART, and memory models from scratch. All models are validated and used in production virtual platforms.
- **License**: Apache-2.0
- **Assessment**: **Use this as the platform library.** It has CLINT and PLIC implementations that are directly usable.

#### riscv-vp++ (ICS JKU Linz)
- **URL**: https://github.com/ics-jku/riscv-vp-plusplus
- **What it does**: Full SystemC/TLM-2.0 RISC-V virtual platform with its own custom ISS (not Spike). Supports RV32 and RV64, CLINT, PLIC, RVV. Can run Linux.
- **Relevance**: Alternative to Spike-based approach. Has the full platform topology we need, but the ISS is less authoritative than Spike. Good reference for TLM wiring patterns.
- **License**: MIT
- **Assessment**: Useful as a topology reference but the ISS is not Spike, so it does not fully meet the requirements.

#### or1kmvp (Jan Weinstock)
- **URL**: https://github.com/janweinstock/or1kmvp
- **What it does**: OpenRISC 1000 **multi-core** virtual platform using VCML. Demonstrates the pattern of multiple CPU TLM modules connected through a shared crossbar to memory and peripherals.
- **Relevance**: The multicore VCML topology wiring pattern is directly transferable to RISC-V / Spike. This is the key architectural reference for the multi-instance design.
- **License**: Apache-2.0
- **Assessment**: **Study this carefully** for multicore topology patterns.

#### AVP64 (aut0)
- **URL**: https://github.com/aut0/avp64
- **What it does**: ARMv8 multicore virtual platform using QEMU as ISS and VCML as the component library. Demonstrates running Xen + Linux on a multi-CPU virtual platform.
- **Relevance**: Shows VCML at scale with a heavyweight ISS (QEMU). Architectural patterns translate to the Spike use case.
- **License**: MIT

#### Spike MMIO Plugin Interface
- **URL**: https://github.com/riscv-software-src/riscv-isa-sim (PR #186, vexingcodes)
- **What it does**: `abstract_device_t` interface allowing shared-object plugins to be inserted into Spike's memory map at any address range.
- **Relevance**: This is the mechanism by which a SystemC TLM target can be made visible inside Spike's address space without modifying Spike source. Each TLM peripheral gets a thin Spike plugin adapter that forwards `load()`/`store()` calls into TLM `b_transport()`.
- **Assessment**: Critical integration primitive.

### 2.2 Platform Generation / Description Tools

#### OpenAMP Lopper
- **URL**: https://github.com/OpenAMP/lopper
- **What it does**: Processes **System Device Trees** (a superset of standard DTS that describes multiple processing domains, their memory maps, and inter-processor communication resources) to produce per-domain DTS files, OpenAMP configuration headers, and remoteproc/rpmsg setup.
- **Relevance**: The System Device Tree format is a natural description language for AMP platforms and is supported by the broader embedded Linux ecosystem. A Lopper "assist" plugin could be written to emit SystemC C++ wiring code in addition to DTS.
- **License**: BSD-3-Clause
- **Assessment**: Worth adopting as the **description format** because firmware engineers already use it and it defines the vocabulary for AMP memory maps and domains.

#### Arch2Code
- **URL**: https://arch2code.org
- **What it does**: Reads YAML architecture descriptions and generates SystemC, SystemVerilog, and C++ code. Supports register maps and interfaces.
- **Relevance**: Could serve as the code-generation backend for the platform generator.
- **Assessment**: Heavyweight dependency. A simpler Python Jinja2 template approach is sufficient for this project's scope.

### 2.3 Gap Analysis

| Capability Needed | Available? | Source |
|-------------------|-----------|--------|
| Spike-in-TLM wrapper (single core) | Yes | spike-vp |
| VCML component library | Yes | machineware-gmbh/vcml |
| VCML CLINT model | Yes | vcml |
| VCML PLIC model | Yes | vcml |
| VCML NS16550A UART model | Yes | vcml |
| VCML crossbar/bus | Yes | vcml |
| Multicore TLM topology pattern | Yes | or1kmvp (OpenRISC) |
| AMP topology (independent clusters) | **No** | Must build |
| Spike multi-context AMP mode | **No** | Must build |
| Platform description → C++ generator | **No** | Must build |
| YAML/DTS description schema | **No** | Must design |

---

## 3. Architecture Design

### 3.1 Conceptual Platform Topology

```
  ┌───────────────────────────────────────────────────────────────────┐
  │                    SystemC Top Module                              │
  │                                                                    │
  │  ┌─────────────────────┐    ┌─────────────────────┐              │
  │  │   Spike Cluster 0   │    │   Spike Cluster 1   │              │
  │  │  ┌───┐ ┌───┐ ┌───┐  │    │  ┌───┐ ┌───┐       │              │
  │  │  │H0 │ │H1 │ │H2 │  │    │  │H3 │ │H4 │       │              │
  │  │  │rv64gc  rv64gc    │    │  │rv64gcv   rv64gcv │              │
  │  │  └─┬─┘ └─┬─┘ └─┬─┘  │    │  └─┬─┘ └─┬─┘       │              │
  │  │    │     │     │      │    │    │     │           │              │
  │  │  ┌─┴─────┴─────┴──┐  │    │  ┌─┴─────┴───────┐  │              │
  │  │  │  sim_t context  │  │    │  │ sim_t context  │  │              │
  │  │  └────────┬────────┘  │    │  └───────┬────────┘  │              │
  │  │  ┌────────┴────────┐  │    │  ┌───────┴────────┐  │              │
  │  │  │  TLM Initiator  │  │    │  │  TLM Initiator │  │              │
  │  │  └────────┬────────┘  │    │  └───────┬────────┘  │              │
  │  └───────────┼───────────┘    └──────────┼───────────┘              │
  │              │                           │                           │
  │  ┌───────────┴───────────────────────────┴──────────────────┐       │
  │  │                 TLM-2.0 Crossbar / Router                │       │
  │  └──┬──────────┬──────────┬───────────┬──────────┬──────────┘       │
  │     │          │          │           │          │                   │
  │  ┌──┴──┐  ┌───┴──┐  ┌────┴────┐  ┌───┴──┐  ┌───┴──┐               │
  │  │Scr- │  │ DRAM │  │ Shared  │  │ PLIC │  │ CLINT│               │
  │  │atch │  │      │  │  SRAM   │  │      │  │      │               │
  │  │fast │  │ slow │  │(mailbox)│  │      │  │      │               │
  │  │     │  │      │  │         │  │      │  │      │               │
  │  │ L1-like   L2-like  IPC region  intr   timer   │               │
  │  └─────┘  └──────┘  └─────────┘  └──────┘  └──────┘               │
  │                                                                    │
  │  ┌──────────────────────────────────────────────────────────┐     │
  │  │  Additional Peripherals (per platform description)        │     │
  │  │  UART0  UART1  GPIO  SPI  MailboxReg  WDT  ...           │     │
  │  └──────────────────────────────────────────────────────────┘     │
  └───────────────────────────────────────────────────────────────────┘
```

### 3.2 Key Design Decisions

#### Decision 1: One `sim_t` per cluster, not one per hart

Each Spike cluster is an independent `sim_t` instance with:
- Its own hart list (one or more `processor_t` objects)
- Its own `bus_t` memory map (internal to Spike)
- An HTIF or CLINT within its own memory map for hart-local timer/IPI

Cross-cluster transactions (shared memory, inter-cluster interrupts) are **not** routed through Spike's internal bus — they are routed through the SystemC TLM crossbar. This requires the Spike MMIO plugin interface: every cross-cluster address range visible inside a cluster's address map has a thin plugin adapter that forwards to TLM.

#### Decision 2: VCML as the peripheral and bus library

All non-processor components use VCML models:
- `vcml::generic::crossbar` — the system bus
- `vcml::riscv::clint` — CLINT per cluster (hart-local timers and IPI)
- `vcml::riscv::plic` — global PLIC for external interrupt routing
- `vcml::generic::memory` — scratchpad, SRAM, DRAM regions
- `vcml::serial::uart8250` — NS16550A-compatible UART

#### Decision 3: YAML description with Python code generator

A YAML file describes the platform topology. A Python script (`platforms/systemc/generate.py`) reads the YAML and emits a C++ `top.cpp` that instantiates and wires the described components. The YAML schema is a superset of what could be expressed in a System Device Tree, with simulation-specific extensions.

#### Decision 4: Spike TLM wrapper module

A `spike_cluster_t` SC_MODULE owns a `sim_t`, manages its execution thread, and exposes:
- A TLM-2.0 initiator socket for outgoing memory transactions
- An interrupt input port for incoming PLIC interrupts

The MMIO plugin mechanism is used to redirect Spike's accesses to the TLM fabric for any address range not served by Spike's own internal bus.

---

## 4. Component Breakdown

### 4.1 `spike_cluster_t` — Spike ISS SystemC Wrapper

**File**: `platforms/systemc/src/spike_cluster.h / .cpp`

**Responsibilities**:
- Instantiate and own a Spike `sim_t` with a configurable number of harts and ISA string
- Run the Spike simulation loop in a dedicated `sc_thread`
- Intercept all memory accesses from Spike that target cross-cluster addresses and forward them as TLM transactions on the initiator socket
- Receive TLM transactions targeting cluster-private peripherals (CLINT, local SRAM) by serving them from Spike's internal bus

**Interface**:
```cpp
SC_MODULE(spike_cluster_t) {
    tlm_utils::simple_initiator_socket<spike_cluster_t> bus_out;
    sc_core::sc_in<bool> irq_in[MAX_HARTS];   // from PLIC
    sc_core::sc_out<bool> irq_out;             // to PLIC (hart online)
    sc_core::sc_port<sc_core::sc_signal_in_if<bool>> reset;

    SC_HAS_PROCESS(spike_cluster_t);
    spike_cluster_t(sc_core::sc_module_name, const cluster_config_t&);
};
```

**Thread model**: Spike's `run()` loop is not thread-safe relative to SystemC's kernel. The wrapper uses a SystemC thread that calls `sim_t::run(steps)` in a loop, yielding back to the SystemC scheduler via `wait()` at each quantum boundary. This is the same pattern used by spike-vp and VCML-wrapped QEMU (avp64).

### 4.2 `spike_mmio_bridge_t` — MMIO Plugin Forwarding to TLM

**File**: `platforms/systemc/src/spike_mmio_bridge.cpp`

Implements `abstract_device_t`. Each address range that maps to the SystemC fabric gets one bridge instance. On `load()` / `store()` from Spike, it calls `b_transport()` on the cluster's initiator socket with the appropriate payload. This is the key integration primitive.

### 4.3 Platform Generator

**File**: `platforms/systemc/generate.py`

Input: `platforms/systemc/platforms/<name>.yaml`
Output: `platforms/systemc/gen/<name>/top.cpp`, `<name>/platform.h`

The generator:
1. Parses the YAML description
2. Validates the schema (required fields, address range overlaps, interrupt assignments)
3. Emits C++ `sc_main()` with all module instantiations and `bind()` calls
4. Emits a platform header with address map macros usable by firmware

### 4.4 YAML Platform Description Schema

```yaml
# platforms/systemc/platforms/amp_2cluster.yaml
platform:
  name: amp_2cluster
  description: "2-cluster AMP: RV64GC application + RV64GCV accelerator"

  # Simulation parameters (SystemC / TLM extensions, not in standard DTS)
  simulation:
    quantum_ns: 100         # TLM-2.0 loosely-timed quantum
    default_bus_latency_ns: 10

  # Processing clusters (each becomes one sim_t + spike_cluster_t)
  clusters:
    - name: cluster0
      isa: rv64gc
      harts: 2
      reset_pc: 0x80000000
      private_memory:
        - name: sram0
          base: 0x20000000
          size: 0x10000      # 64 KiB private scratchpad
          latency_ns: 2
      clint_base: 0x02000000
      uart_base: 0x10000000

    - name: cluster1
      isa: rv64gcv
      vlen: 256
      harts: 2
      reset_pc: 0x82000000   # different entry point for AMP firmware
      private_memory:
        - name: sram1
          base: 0x22000000
          size: 0x10000      # 64 KiB private scratchpad
          latency_ns: 2
      clint_base: 0x02010000
      uart_base: 0x10010000

  # Shared memory (visible to all clusters)
  shared_memory:
    - name: shared_sram
      base: 0x30000000
      size: 0x80000          # 512 KiB shared SRAM (mailbox region)
      latency_ns: 5

    - name: dram
      base: 0x80000000
      size: 0x8000000        # 128 MiB DRAM (code + heap)
      latency_ns: 50

  # Global interrupt controller
  plic:
    base: 0x0C000000
    num_sources: 32
    num_targets: 4           # 2 harts per cluster × 2 clusters

  # Peripherals (registered by type name → component class)
  peripherals:
    - type: uart8250
      name: uart0
      base: 0x10000000
      irq: 1
    - type: uart8250
      name: uart1
      base: 0x10010000
      irq: 2
    - type: mailbox
      name: mbox01
      base: 0x30000000      # first 4 KiB of shared SRAM as HW mailbox
      irq_to_cluster: [0, 1]  # raises PLIC interrupt to both clusters

  # Memory access map per cluster (for linker script / firmware header generation)
  firmware:
    cluster0:
      code_base: 0x80000000
      stack_top: 0x80040000
      private_sram: 0x20000000
      shared_sram: 0x30000000
      mailbox: 0x30000000
    cluster1:
      code_base: 0x82000000
      stack_top: 0x82040000
      private_sram: 0x22000000
      shared_sram: 0x30000000
      mailbox: 0x30000000
```

---

## 5. Implementation Plan

### Phase 8.1: SystemC Build Infrastructure (Foundation)

**Goal**: Prove that Spike can be built as a library and linked into a SystemC program in the existing build environment.

**Tasks**:
1. Add SystemC 2.3.3 and VCML to the development environment
   - Add to `.devcontainer/Dockerfile`
   - Add to `scripts/setup-simulators.sh`
2. Build Spike as a shared library (`libriscv.so`)
3. Write a minimal C++ program (`platforms/systemc/hello_spike.cpp`) that instantiates `sim_t` directly (no SystemC), loads an ELF, runs 1000 steps, and exits
4. Add CMake target for SystemC platforms: `cmake --preset systemc`

**Test**: `hello_spike` executes the existing Phase 2 QEMU ELF via Spike's library API and produces the expected HTIF output.

**Key files changed**:
- `.devcontainer/Dockerfile`
- `scripts/setup-simulators.sh`
- `CMakePresets.json` (new `systemc` preset)
- `platforms/systemc/CMakeLists.txt` (new)
- `platforms/systemc/hello_spike.cpp` (new)

### Phase 8.2: Spike↔TLM Single-Core Wrapper

**Goal**: A single `spike_cluster_t` SC_MODULE wrapping one Spike `sim_t`, connected to a VCML memory and VCML UART via a VCML crossbar.

**Tasks**:
1. Implement `spike_mmio_bridge_t` (Spike `abstract_device_t` → TLM `b_transport`)
2. Implement `spike_cluster_t` SC_MODULE:
   - Owns `sim_t`
   - Registers MMIO bridge for all non-private addresses
   - Runs Spike in SC_THREAD with quantum-based yielding
3. Wire `spike_cluster_t` → VCML crossbar → VCML memory + VCML UART
4. Write test firmware: bare-metal HTIF-less ELF that writes to UART via MMIO (not HTIF) and halts via a magic write to a VCML `exit` register

**Test** (`phase8_systemc_single_uart`):
```cmake
add_test(
  NAME phase8_systemc_single_uart
  COMMAND platforms/systemc/build/spike_vp_single
          platforms/systemc/platforms/single_core.yaml
          --elf app/build-systemc/app.elf
)
set_tests_properties(phase8_systemc_single_uart PROPERTIES
  PASS_REGULAR_EXPRESSION "Hello from SystemC Spike"
  TIMEOUT 60
  LABELS "phase8;systemc;smoke"
)
```

### Phase 8.3: PLIC and CLINT Integration

**Goal**: Interrupts working. Firmware on Cluster 0 receives a timer interrupt from CLINT and an external interrupt routed through PLIC.

**Tasks**:
1. Add VCML CLINT instance to the single-core platform
2. Wire CLINT timer interrupt output to Spike hart's external interrupt input (via `spike_cluster_t` irq_in port)
3. Add VCML PLIC instance
4. Wire VCML UART interrupt → PLIC → Spike hart M-mode external interrupt
5. Write interrupt test firmware:
   - Enable MTIE (machine timer interrupt enable)
   - Set `mtimecmp` via CLINT MMIO
   - Wait for M-mode timer trap, confirm mip.MTIP set
   - Enable MEIE (machine external interrupt enable)
   - Trigger UART RX interrupt
   - Confirm mip.MEIP set

**Test** (`phase8_systemc_clint_timer`, `phase8_systemc_plic_uart_irq`):
```
PASS_REGULAR_EXPRESSION "\\[TEST\\] CLINT timer.*PASS"
PASS_REGULAR_EXPRESSION "\\[TEST\\] PLIC UART IRQ.*PASS"
```

### Phase 8.4: Two-Cluster AMP Topology

**Goal**: Two independent `spike_cluster_t` instances sharing memory through the TLM crossbar. Each cluster runs separate firmware.

**Tasks**:
1. Instantiate two `spike_cluster_t` modules
2. Add private SRAM per cluster (VCML memory, not shared)
3. Add shared SRAM region (single VCML memory target, routed to both initiators via crossbar)
4. Write AMP test firmware pair:
   - **Cluster 0 firmware**: Writes a marker to shared SRAM mailbox, raises a SW-defined interrupt (write to PLIC pending register), waits for response
   - **Cluster 1 firmware**: Waits for interrupt, reads mailbox, writes response, signals back
5. Test validates that both clusters complete their handshake within timeout

**Test** (`phase8_systemc_amp_mailbox`):
```
PASS_REGULAR_EXPRESSION "\\[CLUSTER0\\].*mailbox sent.*PASS"
PASS_REGULAR_EXPRESSION "\\[CLUSTER1\\].*mailbox received.*PASS"
```

### Phase 8.5: Platform Description Generator

**Goal**: Replace hand-written C++ `sc_main()` with a Python-generated one from the YAML description.

**Tasks**:
1. Define YAML schema (as in Section 4.4 above)
2. Write `platforms/systemc/generate.py`:
   - Schema validation (pydantic or jsonschema)
   - Address overlap detection
   - C++ code generation via Jinja2 templates
   - Firmware header generation (address macros for each cluster)
3. Regenerate the Phase 8.4 platform from YAML; verify tests still pass
4. Add CMake custom_command that runs generate.py and rebuilds if YAML changes

**Test**: Run `generate.py amp_2cluster.yaml`, diff output against golden reference, run existing Phase 8.4 tests with generated platform.

### Phase 8.6: Multi-Level Memory and Synchronization Tests

**Goal**: Demonstrate firmware exercising different memory regions (fast scratchpad vs. slow DRAM) and cross-cluster synchronization patterns.

**Tasks**:
1. Add latency annotations to VCML memory models (TLM `b_transport` delay)
2. Write benchmark firmware that times access to scratchpad vs. DRAM using `mcycle` CSR
3. Implement spinlock in shared SRAM using AMO (`amoadd.w`)
4. Write SMP-style barrier in shared SRAM for cross-cluster rendezvous
5. Test validates timing ratio (scratchpad measurably faster than DRAM) and that barrier completes

**Test** (`phase8_systemc_multilevel_memory`, `phase8_systemc_amp_barrier`):
```
PASS_REGULAR_EXPRESSION "\\[TEST\\] scratchpad.*faster than DRAM.*PASS"
PASS_REGULAR_EXPRESSION "\\[TEST\\] cross-cluster barrier.*PASS"
```

---

## 6. Directory Structure

```
platforms/
└── systemc/
    ├── CMakeLists.txt              # SystemC platform build
    ├── generate.py                 # YAML → C++ platform generator
    ├── schema/
    │   └── platform_schema.yaml    # JSON-schema for YAML validation
    ├── platforms/
    │   ├── single_core.yaml        # Single Spike cluster platform
    │   ├── amp_2cluster.yaml       # 2-cluster AMP platform
    │   └── amp_4cluster.yaml       # 4-cluster AMP platform (future)
    ├── templates/
    │   ├── top.cpp.j2              # Jinja2 template for sc_main
    │   ├── platform.h.j2           # Jinja2 template for firmware header
    │   └── linker.ld.j2            # Jinja2 template for linker script
    ├── src/
    │   ├── spike_cluster.h         # Spike ISS SystemC wrapper header
    │   ├── spike_cluster.cpp       # Spike ISS SystemC wrapper implementation
    │   ├── spike_mmio_bridge.h     # Spike abstract_device_t → TLM bridge
    │   ├── spike_mmio_bridge.cpp
    │   └── main.cpp                # Entry point (loads YAML, calls sc_main)
    ├── gen/                        # Generated files (gitignored)
    │   ├── single_core/
    │   │   ├── top.cpp
    │   │   └── platform.h
    │   └── amp_2cluster/
    │       ├── top.cpp
    │       └── platform.h
    └── tests/
        ├── firmware/               # AMP test firmware (bare-metal, MMIO-based)
        │   ├── cluster0_uart/      # Phase 8.2 test firmware
        │   ├── cluster0_irq/       # Phase 8.3 interrupt test
        │   ├── cluster0_mailbox/   # Phase 8.4 AMP sender
        │   └── cluster1_mailbox/   # Phase 8.4 AMP receiver
        └── CMakeLists.txt
```

---

## 7. Firmware Considerations for AMP

### 7.1 Separate ELFs per Cluster

Each cluster runs a separate ELF binary with its own linker script, entry point, and platform header. The generated `platform.h` provides the correct address macros for each cluster.

```
build/
├── cluster0/
│   ├── app.elf           # cluster0 firmware
│   └── platform.h        # generated, cluster0 address map
└── cluster1/
    ├── app.elf           # cluster1 firmware
    └── platform.h        # generated, cluster1 address map
```

### 7.2 UART Access Without HTIF

The existing Spike test flow uses HTIF for I/O. In the SystemC platform, UART is provided by a VCML UART model at a fixed MMIO address. Firmware must use the MMIO UART driver (`uart.c` with `PLATFORM_SYSTEMC_AMP` define) rather than HTIF. The existing `uart.c` already abstracts this.

### 7.3 Reset Vector and ELF Loading

Each `spike_cluster_t` loads its own ELF file. The reset PC is derived from the ELF entry point, overriding the YAML `reset_pc` if present. For AMP, cluster 0 and cluster 1 receive different ELF paths on the simulator command line:

```bash
platforms/systemc/build/amp_platform \
    platforms/systemc/platforms/amp_2cluster.yaml \
    --cluster0-elf build/cluster0/app.elf \
    --cluster1-elf build/cluster1/app.elf
```

### 7.4 Interrupt Architecture

```
Cluster 0 harts ←─── CLINT0 (mtimecmp, msip per hart, base 0x02000000)
Cluster 1 harts ←─── CLINT1 (mtimecmp, msip per hart, base 0x02010000)

External interrupts → PLIC (base 0x0C000000)
                        ├─ Source 1: UART0 RX
                        ├─ Source 2: UART1 RX
                        ├─ Source 3: Mailbox cluster0→cluster1
                        ├─ Source 4: Mailbox cluster1→cluster0
                        └─ ...
                        │
                        ├─ Context 0: cluster0/hart0 M-mode
                        ├─ Context 1: cluster0/hart1 M-mode
                        ├─ Context 2: cluster1/hart0 M-mode
                        └─ Context 3: cluster1/hart1 M-mode
```

---

## 8. Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Spike `sim_t` not thread-safe with SystemC kernel | High | High | Wrap all Spike calls in a dedicated SC_THREAD; never call Spike from SC_METHOD |
| Spike DMI access bypasses TLM (memory latency invisible) | Medium | Medium | Disable DMI for cross-cluster regions; accept performance cost |
| VCML PLIC wiring incompatible with Spike's IRQ inputs | Medium | Medium | Study spike-vp for IRQ injection pattern; use `processor_t::take_trap()` |
| YAML generator emits subtly wrong C++ | Medium | Low | Golden-file tests on generator output; schema validation |
| Spike library API changes between versions | Low | High | Pin Spike version; document upgrade path |
| TLM quantum too coarse for tight IPC synchronization | Medium | Medium | Make quantum configurable; document trade-off |

---

## 9. Dependencies

| Dependency | Version | Purpose |
|-----------|---------|---------|
| SystemC | 2.3.3 | Simulation kernel |
| VCML | latest main | Peripheral models, bus, TLM utilities |
| Spike (`riscv-isa-sim`) | ≥ 1.1.0 | ISS backend (built as `libriscv.so`) |
| Python | ≥ 3.10 | Platform generator |
| Jinja2 | ≥ 3.1 | C++ code generation templates |
| pydantic | ≥ 2.0 | YAML schema validation |
| riscv64-unknown-elf-gcc | ≥ 13 | AMP firmware compilation |
| CMake | ≥ 3.20 | Build orchestration |

---

## 10. Relationship to Existing Phases

| Existing Phase | Reuse in Phase 8 |
|---------------|-----------------|
| Phase 2: UART driver | `uart.c` reused with new `PLATFORM_SYSTEMC_AMP` guard |
| Phase 3: HTIF driver | Not reused (SystemC platform uses MMIO UART, not HTIF) |
| Phase 4: SMP synchronization | Spinlock/barrier patterns reused for cross-cluster sync in shared SRAM |
| Phase 5: RVV | Cluster 1 can be configured as `rv64gcv`; RVV workloads from Phase 5 run on cluster 1 |
| Phase 6: gem5 | gem5 FS mode is an alternative simulation backend; SystemC platform is complementary |

---

## 11. Future Extensions

Once the base platform is stable, the following components can be added by writing a new VCML SC_MODULE and registering the type name in `generate.py`:

| Extension | Complexity | Purpose |
|-----------|-----------|---------|
| DMA controller | Medium | Autonomous memory-to-memory transfers; test DMA interrupt |
| Hardware semaphore | Low | Faster cross-cluster locking without shared-memory spinloops |
| Watchdog timer | Low | Firmware safety; test WDT reset behavior |
| SPI/I2C peripheral | Medium | Peripheral firmware development |
| Cache model (L1/L2) | High | Cycle-approximate cache miss penalties |
| Custom accelerator stub | Medium | AMP accelerator interface development |
| Lopper integration | Medium | System Device Tree as input format; generate per-cluster DTS |
| Trace/logging layer | Low | Per-transaction bus trace for debug |

---

## 12. Success Criteria

| Criterion | Measurement |
|-----------|-------------|
| Two-cluster AMP platform boots | Both cluster ELFs reach `main()` and print output |
| Interrupts work end-to-end | CLINT timer and PLIC external interrupt tests pass |
| Cross-cluster mailbox IPC | Handshake completes without deadlock, within 10 seconds sim time |
| Memory latency differentiation | Scratchpad measured faster than DRAM in firmware benchmark |
| Platform generation from YAML | Running `generate.py amp_2cluster.yaml` rebuilds platform; all tests pass |
| Extensibility demonstrated | Adding a new peripheral type requires only YAML edit + new SC_MODULE |

---

*Next: See [ROADMAP.md](../ROADMAP.md) for how this phase fits into the overall project trajectory.*
*Implementation begins in `platforms/systemc/` following Phase 8.1 tasks above.*
