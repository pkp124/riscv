# Design Proposal 03: Multi-Core and Multi-Processor Platform Configurations

## 1. Overview

This document describes how to configure and run RISC-V simulations across various topologies:

- **Single-core**: One hart, simplest configuration
- **Multi-core (SMP)**: Multiple harts sharing memory, symmetric multiprocessing
- **Multi-processor (AMP)**: Multiple independent processors, potentially heterogeneous

Each configuration is detailed for every supported simulation platform.

---

## 2. Terminology

| Term | Definition |
|------|-----------|
| **Hart** | Hardware thread -- the RISC-V term for a logical CPU core |
| **Core** | Physical core (may contain multiple harts if SMT is supported) |
| **SMP** | Symmetric Multi-Processing -- all harts are identical and share memory |
| **AMP** | Asymmetric Multi-Processing -- harts may have different capabilities |
| **CLINT** | Core-Local Interruptor -- provides timer and IPI per hart |
| **PLIC** | Platform-Level Interrupt Controller -- routes external interrupts to harts |
| **IPI** | Inter-Processor Interrupt -- used to wake/signal other harts |

---

## 3. Configuration Matrix

| Configuration | Harts | ISA | Memory Model | Simulators |
|--------------|-------|-----|-------------|------------|
| `single-rv64gc` | 1 | RV64GC | Single address space | QEMU, Spike, gem5, Renode |
| `smp-2-rv64gc` | 2 | RV64GC | Shared memory, coherent | QEMU, Spike, gem5 |
| `smp-4-rv64gc` | 4 | RV64GC | Shared memory, coherent | QEMU, Spike, gem5 |
| `smp-8-rv64gc` | 8 | RV64GC | Shared memory, coherent | QEMU, gem5 |
| `single-rv64gcv` | 1 | RV64GCV | Single address space | QEMU, Spike |
| `smp-4-rv64gcv` | 4 | RV64GCV | Shared memory, coherent | QEMU, Spike |
| `amp-2x2-rv64gc` | 2+2 | RV64GC | Shared + private regions | gem5, Renode |
| `amp-rv64gc-rv32imc` | 1+1 | Mixed | Shared + private regions | gem5, Renode |

---

## 4. Single-Core Configuration

### 4.1 Design

The simplest configuration: one hart running in M-mode.

```
┌─────────────────────────────────────┐
│            Single Hart              │
│  ┌──────────┐   ┌───────────────┐  │
│  │  Hart 0  │───│  L1 I-Cache   │  │
│  │  RV64GC  │   │  L1 D-Cache   │  │
│  └──────────┘   └───────────────┘  │
│        │                            │
│  ┌─────┴─────────────────────────┐  │
│  │          Memory Bus           │  │
│  └─────┬──────────┬──────────┬───┘  │
│   ┌────┴───┐ ┌────┴───┐ ┌───┴───┐  │
│   │  RAM   │ │  UART  │ │ CLINT │  │
│   │128 MiB │ │NS16550A│ │       │  │
│   └────────┘ └────────┘ └───────┘  │
└─────────────────────────────────────┘
```

### 4.2 Simulator Commands

```bash
# QEMU
qemu-system-riscv64 \
    -machine virt \
    -smp 1 \
    -m 128M \
    -nographic \
    -bios none \
    -kernel build/app-qemu.elf

# Spike
spike --isa=rv64gc -p1 build/app-spike.elf

# gem5 (SE mode for simplicity)
build/RISCV/gem5.opt configs/single_core.py \
    --cpu-type=AtomicSimpleCPU \
    --cmd=build/app-gem5.elf
```

---

## 5. Multi-Core SMP Configuration

### 5.1 Design

Multiple identical harts sharing a unified memory space with hardware coherence.

```
┌──────────────────────────────────────────────────────┐
│                  SMP System (4 harts)                 │
│                                                      │
│  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐    │
│  │ Hart 0 │  │ Hart 1 │  │ Hart 2 │  │ Hart 3 │    │
│  │ RV64GC │  │ RV64GC │  │ RV64GC │  │ RV64GC │    │
│  └───┬────┘  └───┬────┘  └───┬────┘  └───┬────┘    │
│      │           │           │           │          │
│  ┌───┴──┐    ┌───┴──┐    ┌───┴──┐    ┌───┴──┐      │
│  │L1 I/D│    │L1 I/D│    │L1 I/D│    │L1 I/D│      │
│  └───┬──┘    └───┬──┘    └───┬──┘    └───┬──┘      │
│      │           │           │           │          │
│  ┌───┴───────────┴───────────┴───────────┴──┐       │
│  │         Shared L2 Cache (256 KiB)        │       │
│  └──────────────────┬───────────────────────┘       │
│                     │                                │
│  ┌──────────────────┴────────────────────────┐      │
│  │              Memory Bus / Interconnect     │      │
│  └────┬──────────┬──────────┬────────┬───────┘      │
│  ┌────┴───┐ ┌────┴───┐ ┌───┴───┐ ┌──┴────┐         │
│  │  RAM   │ │  UART  │ │ CLINT │ │ PLIC  │         │
│  │128 MiB │ │NS16550A│ │       │ │       │         │
│  └────────┘ └────────┘ └───────┘ └───────┘         │
└──────────────────────────────────────────────────────┘
```

### 5.2 Hart Boot Protocol

In SMP mode, all harts start execution at the reset vector simultaneously. We need a protocol to:

1. Designate Hart 0 as the boot hart
2. Park all other harts in a waiting state
3. Have Hart 0 initialize the system
4. Wake secondary harts when ready

**Boot Protocol Options:**

| Method | Description | Platform Support |
|--------|-------------|-----------------|
| **Spin on memory flag** | Secondary harts poll a shared variable | All platforms |
| **CLINT IPI** | Hart 0 writes to CLINT MSIP register | QEMU, gem5 |
| **WFI + IPI** | Secondary harts execute WFI, wake on IPI | QEMU, gem5 |
| **SBI HSM** | Use SBI hart state management | Requires SBI firmware |

**Recommended approach**: Spin on memory flag (most portable), with CLINT IPI as an enhancement.

### 5.3 Memory Consistency

RISC-V uses the **RVWMO** (RISC-V Weak Memory Ordering) model. For SMP correctness:

- Use `fence` instructions for ordering
- Use `lr`/`sc` (load-reserved/store-conditional) for atomic RMW
- Use AMO instructions (`amoadd`, `amoswap`, etc.) for atomic operations
- Use `fence.i` for instruction cache coherence after code modification

### 5.4 SMP Synchronization Primitives

```c
/* Spin-lock using lr/sc */
typedef struct {
    volatile int lock;
} spinlock_t;

static inline void spin_lock(spinlock_t *lock) {
    int tmp;
    __asm__ volatile(
        "1:\n"
        "  lr.w %0, (%1)\n"
        "  bnez %0, 1b\n"
        "  li   %0, 1\n"
        "  sc.w %0, %0, (%1)\n"
        "  bnez %0, 1b\n"
        "  fence rw, rw\n"
        : "=&r"(tmp)
        : "r"(&lock->lock)
        : "memory"
    );
}

static inline void spin_unlock(spinlock_t *lock) {
    __asm__ volatile("fence rw, rw" ::: "memory");
    lock->lock = 0;
}

/* Barrier using atomic counter */
typedef struct {
    volatile int count;
    volatile int total;
    volatile int sense;
} barrier_t;

void barrier_wait(barrier_t *bar);
```

### 5.5 Simulator Commands (SMP)

```bash
# QEMU - 4 harts
qemu-system-riscv64 \
    -machine virt \
    -smp 4 \
    -m 256M \
    -nographic \
    -bios none \
    -kernel build/app-qemu-smp.elf

# Spike - 4 harts
spike --isa=rv64gc -p4 build/app-spike-smp.elf

# gem5 - 4 cores with cache hierarchy
build/RISCV/gem5.opt configs/multi_core.py \
    --cpu-type=TimingSimpleCPU \
    --num-cpus=4 \
    --l1d_size=32kB \
    --l1i_size=32kB \
    --l2_size=256kB \
    --kernel=build/app-gem5-smp.elf
```

---

## 6. Multi-Core SMP with RVV

### 6.1 Design

Same as SMP, but all harts have the Vector extension enabled.

```
┌──────────────────────────────────────────────────┐
│           SMP+RVV System (4 harts)               │
│                                                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ... │
│  │  Hart 0  │  │  Hart 1  │  │  Hart 2  │      │
│  │ RV64GCV  │  │ RV64GCV  │  │ RV64GCV  │      │
│  │ VLEN=256 │  │ VLEN=256 │  │ VLEN=256 │      │
│  └──────────┘  └──────────┘  └──────────┘       │
│       │             │             │              │
│  [Cache hierarchy same as SMP]                   │
└──────────────────────────────────────────────────┘
```

### 6.2 Parallel Vector Workload Distribution

```c
/*
 * Distribute vector workload across harts.
 * Each hart processes a chunk of the data.
 */
void parallel_vec_add(float *a, float *b, float *c, size_t n) {
    uint64_t hartid = read_mhartid();
    uint64_t num_harts = get_num_harts();

    size_t chunk = n / num_harts;
    size_t start = hartid * chunk;
    size_t end = (hartid == num_harts - 1) ? n : start + chunk;

    /* Each hart uses RVV to process its chunk */
    rvv_vec_add(&a[start], &b[start], &c[start], end - start);

    /* Synchronize */
    barrier_wait(&global_barrier);
}
```

### 6.3 Simulator Commands (SMP + RVV)

```bash
# QEMU - 4 harts with RVV, VLEN=256
qemu-system-riscv64 \
    -machine virt \
    -cpu rv64,v=true,vlen=256,elen=64 \
    -smp 4 \
    -m 256M \
    -nographic \
    -bios none \
    -kernel build/app-qemu-smp-rvv.elf

# Spike - 4 harts with RVV
spike --isa=rv64gcv --varch=vlen:256,elen:64 -p4 \
    build/app-spike-smp-rvv.elf
```

---

## 7. Multi-Processor (AMP) Configuration

### 7.1 Design

Asymmetric multi-processing with independent processor clusters.

```
┌─────────────────────────────────────────────────────────┐
│                   AMP System                            │
│                                                         │
│  ┌─────────────────────┐  ┌─────────────────────┐      │
│  │   Cluster 0 (App)   │  │  Cluster 1 (Accel)  │      │
│  │  ┌──────┐ ┌──────┐  │  │  ┌──────┐ ┌──────┐  │      │
│  │  │Hart 0│ │Hart 1│  │  │  │Hart 2│ │Hart 3│  │      │
│  │  │RV64GC│ │RV64GC│  │  │  │RV64GCV││RV64GCV│ │      │
│  │  └──┬───┘ └──┬───┘  │  │  └──┬───┘ └──┬───┘  │      │
│  │     │        │       │  │     │        │       │      │
│  │  ┌──┴────────┴──┐   │  │  ┌──┴────────┴──┐   │      │
│  │  │ Private L2   │   │  │  │ Private L2   │   │      │
│  │  └──────┬───────┘   │  │  └──────┬───────┘   │      │
│  └─────────┼───────────┘  └─────────┼───────────┘      │
│            │                        │                   │
│  ┌─────────┴────────────────────────┴──────────┐        │
│  │           System Interconnect                │        │
│  └───┬──────────┬───────────┬──────────┬───────┘        │
│  ┌───┴───┐  ┌───┴───┐  ┌───┴───┐  ┌───┴───────┐       │
│  │Shared │  │Private│  │Private│  │    I/O     │       │
│  │ RAM   │  │ RAM 0 │  │ RAM 1 │  │ (UART,..) │       │
│  │  1GiB │  │ 64MiB │  │ 64MiB │  │           │       │
│  └───────┘  └───────┘  └───────┘  └───────────┘       │
└─────────────────────────────────────────────────────────┘
```

### 7.2 AMP Use Cases

| Scenario | Cluster 0 | Cluster 1 | Communication |
|----------|-----------|-----------|---------------|
| App + Accelerator | RV64GC (control) | RV64GCV (vector compute) | Shared memory + doorbell interrupt |
| Big.LITTLE | RV64GC (performance) | RV32IMC (efficiency) | Shared memory + mailbox |
| Safety + App | RV64GC (application) | RV64GC (safety monitor) | Shared memory + watchdog |

### 7.3 Inter-Processor Communication

```c
/* Mailbox-based IPC via shared memory */
typedef struct {
    volatile uint32_t status;    /* 0=empty, 1=full */
    volatile uint32_t command;
    volatile uint64_t data[8];
} mailbox_t;

/* Shared memory region visible to both clusters */
mailbox_t *shared_mailbox = (mailbox_t *)SHARED_MEM_BASE;

/* Send command from Cluster 0 to Cluster 1 */
void send_command(uint32_t cmd, uint64_t *data, int count) {
    while (shared_mailbox->status != 0)
        ;  /* Wait for mailbox to be empty */

    for (int i = 0; i < count; i++)
        shared_mailbox->data[i] = data[i];

    fence();  /* Ensure data is visible */
    shared_mailbox->command = cmd;
    shared_mailbox->status = 1;

    /* Optionally trigger IPI to Cluster 1 */
}
```

### 7.4 AMP on gem5

gem5 is the best platform for AMP simulation because it allows fully heterogeneous configurations:

```python
# gem5 AMP configuration (Python)
class AMPSystem(System):
    def __init__(self):
        # Cluster 0: 2x TimingSimpleCPU (application cores)
        self.cluster0 = [TimingSimpleCPU() for _ in range(2)]

        # Cluster 1: 2x MinorCPU (accelerator cores)
        self.cluster1 = [MinorCPU() for _ in range(2)]

        # Shared memory
        self.shared_mem = SimpleMemory(range=AddrRange(0x80000000, size='1GB'))

        # Private memory per cluster
        self.priv_mem0 = SimpleMemory(range=AddrRange(0x70000000, size='64MB'))
        self.priv_mem1 = SimpleMemory(range=AddrRange(0x74000000, size='64MB'))

        # Cache hierarchy per cluster
        for cpu in self.cluster0:
            cpu.addPrivL1Cache(L1DCache(size='32kB'), L1ICache(size='32kB'))
        self.cluster0_l2 = L2Cache(size='256kB')

        for cpu in self.cluster1:
            cpu.addPrivL1Cache(L1DCache(size='32kB'), L1ICache(size='32kB'))
        self.cluster1_l2 = L2Cache(size='256kB')
```

### 7.5 AMP on Renode

Renode natively supports multi-machine simulation:

```
# Renode platform description (.repl)
cluster0_cpu0: CPU.RiscV64 @ sysbus
    cpuType: "rv64gc"
    hartId: 0
    timeProvider: clint

cluster0_cpu1: CPU.RiscV64 @ sysbus
    cpuType: "rv64gc"
    hartId: 1
    timeProvider: clint

cluster1_cpu0: CPU.RiscV64 @ sysbus
    cpuType: "rv64gcv"
    hartId: 2
    timeProvider: clint

shared_mem: Memory.MappedMemory @ sysbus 0x80000000
    size: 0x40000000
```

---

## 8. Configuration Build Matrix

The build system will support all configurations via make targets:

```makefile
# Single-core configurations
make PLATFORM=qemu CONFIG=single            # RV64GC, 1 hart
make PLATFORM=spike CONFIG=single           # RV64GC, 1 hart
make PLATFORM=gem5 CONFIG=single            # RV64GC, 1 hart

# SMP configurations
make PLATFORM=qemu CONFIG=smp NUM_HARTS=4   # RV64GC, 4 harts
make PLATFORM=spike CONFIG=smp NUM_HARTS=4  # RV64GC, 4 harts
make PLATFORM=gem5 CONFIG=smp NUM_HARTS=4   # RV64GC, 4 harts

# RVV configurations
make PLATFORM=qemu CONFIG=rvv              # RV64GCV, 1 hart
make PLATFORM=qemu CONFIG=smp-rvv NUM_HARTS=4  # RV64GCV, 4 harts

# AMP configurations (gem5 only for full support)
make PLATFORM=gem5 CONFIG=amp
```

---

## 9. Stack Allocation Strategy (SMP)

Each hart needs its own stack. Strategy:

```
Memory Layout (SMP, 4 harts):

0x80000000  ┌──────────────────┐  ← RAM start
            │  .text (code)    │
            │  .rodata         │
            │  .data           │
            │  .bss            │
            ├──────────────────┤
            │  Hart 3 Stack    │  8 KiB
            ├──────────────────┤
            │  Hart 2 Stack    │  8 KiB
            ├──────────────────┤
            │  Hart 1 Stack    │  8 KiB
            ├──────────────────┤
            │  Hart 0 Stack    │  8 KiB
            ├──────────────────┤  ← _stack_top
            │                  │
            │  Heap / Free     │
            │                  │
0x87FFFFFF  └──────────────────┘  ← RAM end
```

Each hart's stack pointer is calculated as:
```
sp = _stack_top - (hartid * STACK_SIZE_PER_HART)
```

---

## 10. Testing Strategy per Configuration

| Configuration | Validation |
|--------------|------------|
| Single-core | UART output matches expected string |
| SMP | All harts report online; atomic counter test passes |
| RVV | Vector operation results match scalar reference |
| SMP+RVV | Parallel vector results match serial reference |
| AMP | Both clusters produce expected output; IPC test passes |

---

## 11. Open Questions

1. **Maximum harts to support?** QEMU virt supports up to 512, but 8 is practical. Recommendation: up to 8.
2. **AMP: same binary or separate binaries per cluster?** Recommendation: single binary with runtime cluster detection.
3. **Should we support NUMA topologies in QEMU?** QEMU virt supports `-numa` options.
4. **Cache line size assumption for gem5?** Standard 64 bytes.

---

*Next: See [04-rvv-vector-extension.md](04-rvv-vector-extension.md) for the RVV learning plan.*
