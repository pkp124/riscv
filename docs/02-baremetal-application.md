# Design Proposal 02: Bare-Metal RISC-V Application Architecture

## 1. Overview

This document describes the design of a bare-metal RISC-V application written in C (with assembly startup code). The application is designed to:

- Run without any OS or runtime
- Work across multiple simulation platforms (QEMU, Spike, gem5)
- Demonstrate progressively complex features: basic I/O, interrupts, SMP, and RVV
- Serve as a learning vehicle for RISC-V system architecture

---

## 2. Application Feature Set

The bare-metal application will include the following modules, each demonstrating a specific RISC-V concept:

### Module 1: Boot and Hello World
- Reset vector and startup assembly
- Stack initialization
- BSS clearing
- Jump to `main()`
- Print "Hello from RISC-V" via UART/HTIF

### Module 2: CSR and Privilege Levels
- Read `mhartid`, `mstatus`, `misa`
- Print ISA extension detection (which extensions are available)
- Demonstrate M-mode operation

### Module 3: Trap and Interrupt Handling
- Set up `mtvec` (trap vector)
- Handle synchronous exceptions (illegal instruction, ecall)
- Timer interrupt via CLINT (QEMU) or HTIF (Spike)

### Module 4: Memory Operations
- Simple memory read/write tests
- Atomic operations (AMO instructions from A extension)
- Fence instructions

### Module 5: SMP / Multi-Hart
- Secondary hart wake-up
- Per-hart stack allocation
- Spin-lock implementation using `lr`/`sc`
- Barrier synchronization
- Parallel workload distribution

### Module 6: RVV Vector Workloads
- Vector addition (basic RVV)
- Vector dot product
- Vector matrix multiplication
- Vector memcpy
- VLEN detection and adaptation

### Module 7: Benchmarks
- Dhrystone (integer performance)
- Simple memory bandwidth test
- RVV vs scalar comparison

---

## 3. Boot Sequence Design

```
Power-On / Reset
       │
       ▼
┌─────────────────┐
│  startup.S      │  (Assembly)
│  - Set mtvec    │
│  - Read mhartid │
│  - If hart 0:   │
│    - Clear BSS  │
│    - Init stack  │
│    - Jump main() │
│  - If hart N:   │
│    - Park (WFI) │
│    - Wait for   │
│      IPI signal │
│    - Jump to    │
│      secondary()│
└─────────────────┘
       │
       ▼
┌─────────────────┐
│  main()         │  (C)
│  - Init UART    │
│  - Print banner │
│  - Run modules  │
│  - Wake harts   │
│  - Run SMP test │
│  - Run RVV test │
│  - Print results│
│  - Halt / Loop  │
└─────────────────┘
```

### Startup Assembly (startup.S) Detailed Design

```asm
# Reset vector - all harts start here
_start:
    # Disable interrupts
    csrw    mie, zero

    # Read hart ID
    csrr    t0, mhartid

    # Hart 0 is the boot hart
    bnez    t0, .secondary_hart

.primary_hart:
    # Set up stack pointer (top of RAM - small offset)
    la      sp, _stack_top

    # Clear BSS section
    la      t0, _bss_start
    la      t1, _bss_end
.clear_bss:
    bgeu    t0, t1, .bss_done
    sd      zero, 0(t0)
    addi    t0, t0, 8
    j       .clear_bss
.bss_done:

    # Set up trap vector
    la      t0, _trap_handler
    csrw    mtvec, t0

    # Jump to C main
    call    main

    # If main returns, halt
.halt:
    wfi
    j       .halt

.secondary_hart:
    # Set per-hart stack
    # sp = _stack_top - (hartid * STACK_SIZE)
    la      sp, _stack_top
    li      t1, STACK_SIZE_PER_HART    # e.g., 4096
    mul     t1, t0, t1
    sub     sp, sp, t1

    # Park: wait for IPI from hart 0
    # Spin on a flag in shared memory
    la      t1, _hart_boot_flag
.wait_ipi:
    lw      t2, 0(t1)
    beqz    t2, .wait_ipi
    fence

    # Jump to secondary entry point
    call    secondary_main

    # Halt
    wfi
    j       .halt + 0   # Loop on WFI
```

---

## 4. Memory Map and Linker Script Design

### QEMU virt Machine Memory Map

```
0x00000000 - 0x00000FFF : Debug (unused)
0x00001000 - 0x00001FFF : Boot ROM (holds reset vector stub)
0x00100000 - 0x00100FFF : Test device
0x02000000 - 0x0200FFFF : CLINT (Core-Local INTerrupt controller)
0x0C000000 - 0x0FFFFFFF : PLIC (Platform-Level Interrupt Controller)
0x10000000 - 0x10000FFF : UART0 (NS16550A)
0x10001000 - 0x10001FFF : virtio devices...
0x80000000 - 0x87FFFFFF : RAM (128 MiB default, configurable)
```

### Linker Script (qemu-virt.ld)

```ld
ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS
{
    .text : {
        _text_start = .;
        *(.text.init)       /* Reset vector first */
        *(.text .text.*)
        _text_end = .;
    } > RAM

    .rodata : {
        *(.rodata .rodata.*)
    } > RAM

    .data : {
        _data_start = .;
        *(.data .data.*)
        _data_end = .;
    } > RAM

    .bss : {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        _bss_end = .;
    } > RAM

    /* Stack: 64 KiB total (8 harts x 8 KiB each) */
    . = ALIGN(16);
    _stack_bottom = .;
    . = . + 64K;
    _stack_top = .;

    /* Heap (remaining RAM) */
    _heap_start = .;
    _heap_end = ORIGIN(RAM) + LENGTH(RAM);
}
```

---

## 5. Platform Abstraction Layer (HAL)

The HAL provides a uniform interface across simulators:

### platform.h

```c
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

/* Platform selection (compile-time) */
#if defined(PLATFORM_QEMU_VIRT)
    #define UART_BASE       0x10000000UL
    #define UART_TYPE_NS16550A
    #define CLINT_BASE      0x02000000UL
    #define RAM_BASE        0x80000000UL
    #define RAM_SIZE        0x08000000UL   /* 128 MiB */
    #define HAS_UART        1
    #define HAS_CLINT       1
#elif defined(PLATFORM_SPIKE)
    #define RAM_BASE        0x80000000UL
    #define RAM_SIZE        0x80000000UL   /* 2 GiB */
    #define HTIF_TOHOST     /* defined by Spike runtime */
    #define HAS_HTIF        1
    #define HAS_CLINT       1
    #define CLINT_BASE      0x02000000UL
#elif defined(PLATFORM_GEM5)
    #define UART_BASE       0x10000000UL
    #define UART_TYPE_NS16550A
    #define RAM_BASE        0x80000000UL
    #define RAM_SIZE        0x40000000UL   /* 1 GiB */
    #define HAS_UART        1
#else
    #error "No platform defined. Use -DPLATFORM_QEMU_VIRT, -DPLATFORM_SPIKE, or -DPLATFORM_GEM5"
#endif

/* Common functions */
void platform_init(void);
void platform_putchar(char c);
void platform_puts(const char *s);
void platform_put_hex(uint64_t val);
void platform_put_dec(int64_t val);
void platform_halt(int exit_code);

/* Hart management */
uint64_t platform_get_hartid(void);
uint64_t platform_get_num_harts(void);

#endif /* PLATFORM_H */
```

### UART Driver (NS16550A for QEMU/gem5)

```c
/* NS16550A register offsets */
#define UART_RBR    0x00    /* Receive Buffer Register */
#define UART_THR    0x00    /* Transmit Holding Register */
#define UART_IER    0x01    /* Interrupt Enable Register */
#define UART_FCR    0x02    /* FIFO Control Register */
#define UART_LCR    0x03    /* Line Control Register */
#define UART_MCR    0x04    /* Modem Control Register */
#define UART_LSR    0x05    /* Line Status Register */
#define UART_LSR_TX_EMPTY  0x20

void uart_init(void) {
    volatile uint8_t *uart = (volatile uint8_t *)UART_BASE;
    uart[UART_IER] = 0x00;    /* Disable interrupts */
    uart[UART_LCR] = 0x80;    /* Enable DLAB */
    uart[UART_RBR] = 0x03;    /* Divisor low byte (38400 baud) */
    uart[UART_IER] = 0x00;    /* Divisor high byte */
    uart[UART_LCR] = 0x03;    /* 8 bits, no parity, 1 stop */
    uart[UART_FCR] = 0x07;    /* Enable and clear FIFOs */
    uart[UART_MCR] = 0x00;
}

void uart_putchar(char c) {
    volatile uint8_t *uart = (volatile uint8_t *)UART_BASE;
    while (!(uart[UART_LSR] & UART_LSR_TX_EMPTY))
        ;
    uart[UART_THR] = c;
}
```

### HTIF Driver (for Spike)

```c
/* HTIF interface for Spike */
extern volatile uint64_t tohost;
extern volatile uint64_t fromhost;

void htif_putchar(char c) {
    /* HTIF encoding: device=1 (console), cmd=1 (putchar), data=char */
    tohost = ((uint64_t)1 << 56) | ((uint64_t)1 << 48) | (uint8_t)c;
    while (fromhost == 0)
        ;
    fromhost = 0;
}
```

---

## 6. CSR Access Helpers

```c
/* csr.h - CSR access macros */
#ifndef CSR_H
#define CSR_H

#define CSR_READ(csr, val)  \
    __asm__ volatile("csrr %0, " #csr : "=r"(val))

#define CSR_WRITE(csr, val) \
    __asm__ volatile("csrw " #csr ", %0" : : "r"(val))

#define CSR_SET(csr, val)   \
    __asm__ volatile("csrs " #csr ", %0" : : "r"(val))

#define CSR_CLEAR(csr, val) \
    __asm__ volatile("csrc " #csr ", %0" : : "r"(val))

/* Common CSR addresses */
#define CSR_MHARTID     0xF14
#define CSR_MSTATUS     0x300
#define CSR_MISA        0x301
#define CSR_MIE         0x304
#define CSR_MTVEC       0x305
#define CSR_MSCRATCH    0x340
#define CSR_MEPC        0x341
#define CSR_MCAUSE      0x342
#define CSR_MTVAL       0x343
#define CSR_MIP         0x344
#define CSR_MCYCLE      0xB00
#define CSR_MINSTRET    0xB02

/* Read mhartid */
static inline uint64_t read_mhartid(void) {
    uint64_t val;
    CSR_READ(mhartid, val);
    return val;
}

/* Read misa - ISA extension bitfield */
static inline uint64_t read_misa(void) {
    uint64_t val;
    CSR_READ(misa, val);
    return val;
}

/* Read cycle counter */
static inline uint64_t read_mcycle(void) {
    uint64_t val;
    CSR_READ(mcycle, val);
    return val;
}

#endif /* CSR_H */
```

---

## 7. Main Application Flow

```c
/* main.c */
#include "platform.h"
#include "csr.h"
#include "smp.h"
#include "rvv.h"

/* Module selection (can be enabled/disabled) */
#define MODULE_HELLO    1
#define MODULE_CSR      1
#define MODULE_TRAP     1
#define MODULE_MEMORY   1
#define MODULE_SMP      1   /* Requires multi-hart build */
#define MODULE_RVV      1   /* Requires V extension */

void print_banner(void) {
    platform_puts("\n");
    platform_puts("========================================\n");
    platform_puts("  RISC-V Bare-Metal System Explorer\n");
    platform_puts("  Hart: "); platform_put_dec(platform_get_hartid());
    platform_puts("\n");
    platform_puts("========================================\n\n");
}

void detect_isa(void) {
    uint64_t misa = read_misa();
    platform_puts("ISA Extensions: ");
    for (int i = 0; i < 26; i++) {
        if (misa & (1UL << i)) {
            platform_putchar('A' + i);
        }
    }
    platform_puts("\n");
    /* Specifically check for V */
    if (misa & (1UL << ('V' - 'A'))) {
        platform_puts("  Vector extension (V) detected!\n");
    }
}

int main(void) {
    platform_init();
    print_banner();

#if MODULE_CSR
    detect_isa();
#endif

#if MODULE_SMP
    smp_boot_secondary_harts();
    smp_run_parallel_test();
#endif

#if MODULE_RVV
    if (rvv_available()) {
        rvv_run_tests();
    } else {
        platform_puts("RVV not available, skipping vector tests\n");
    }
#endif

    platform_puts("\nAll tests complete.\n");
    platform_halt(0);
    return 0;
}
```

---

## 8. Compilation Strategy

### Compiler and Flags

```makefile
# Toolchain
CC = riscv64-unknown-elf-gcc
AS = riscv64-unknown-elf-as
LD = riscv64-unknown-elf-ld
OBJCOPY = riscv64-unknown-elf-objcopy
OBJDUMP = riscv64-unknown-elf-objdump

# Base flags (always)
CFLAGS = -march=rv64gc -mabi=lp64d -mcmodel=medany \
         -nostdlib -nostartfiles -ffreestanding \
         -O2 -Wall -Wextra -g

# RVV flags (when building with V extension)
CFLAGS_RVV = -march=rv64gcv -mabi=lp64d

# Platform selection
CFLAGS += -DPLATFORM_QEMU_VIRT  # or -DPLATFORM_SPIKE or -DPLATFORM_GEM5

# Linker flags
LDFLAGS = -T linker/qemu-virt.ld -nostdlib
```

### Build Targets

```makefile
# Default: QEMU virt, single-core, no RVV
make

# QEMU with SMP support
make PLATFORM=qemu SMP=1 NUM_HARTS=4

# QEMU with RVV
make PLATFORM=qemu RVV=1

# Spike
make PLATFORM=spike

# gem5
make PLATFORM=gem5

# All platforms
make all-platforms
```

---

## 9. Output Format

The application will produce structured output to UART/HTIF for easy parsing and comparison across platforms:

```
========================================
  RISC-V Bare-Metal System Explorer
  Hart: 0
========================================

[INFO] Platform: QEMU virt
[INFO] ISA Extensions: ACDFIMSUV
[INFO] Vector extension (V) detected!
[INFO] VLEN = 128 bits

[TEST] CSR Read Test .............. PASS
[TEST] Trap Handler Test ......... PASS
[TEST] Atomic Operations Test .... PASS

[SMP] Booting 4 harts...
[SMP] Hart 1 online
[SMP] Hart 2 online
[SMP] Hart 3 online
[SMP] Parallel increment test .... PASS (expected: 4000, got: 4000)

[RVV] Vector Addition (1024 elements) ... PASS
[RVV]   Scalar cycles: 5120
[RVV]   Vector cycles: 320
[RVV]   Speedup: 16.0x
[RVV] Vector MatMul (32x32) ............ PASS
[RVV] Vector Memcpy (4096 bytes) ....... PASS

All tests complete. Exiting with code 0.
```

---

## 10. File Structure

```
app/
├── src/
│   ├── startup.S           # Reset vector, hart parking, BSS clear
│   ├── main.c              # Application entry point and test orchestration
│   ├── platform.c          # Platform abstraction implementation
│   ├── uart.c              # NS16550A UART driver
│   ├── htif.c              # HTIF driver (Spike)
│   ├── trap.c              # Trap handler
│   ├── smp.c               # Multi-hart boot and synchronization
│   ├── printf.c            # Minimal printf implementation (no libc)
│   └── rvv/
│       ├── rvv_detect.c    # Runtime RVV detection and VLEN query
│       ├── vec_add.c       # Vector addition
│       ├── vec_dotprod.c   # Vector dot product
│       ├── vec_matmul.c    # Vector matrix multiply
│       └── vec_memcpy.c    # Vector memcpy
├── include/
│   ├── platform.h          # Platform abstraction header
│   ├── uart.h              # UART driver header
│   ├── htif.h              # HTIF header
│   ├── csr.h               # CSR access macros
│   ├── smp.h               # SMP primitives
│   ├── rvv.h               # RVV helpers and intrinsics
│   ├── trap.h              # Trap handling
│   └── printf.h            # printf header
├── linker/
│   ├── qemu-virt.ld        # QEMU virt machine linker script
│   ├── spike.ld            # Spike linker script
│   └── gem5.ld             # gem5 linker script
└── Makefile
```

---

## 11. Open Questions

1. **Printf implementation**: Use a minimal custom printf, or pull in a lightweight library (e.g., `mini-printf`)?
2. **Timer interrupt source**: Use CLINT `mtimecmp` directly, or go through SBI? (Recommendation: CLINT directly for bare-metal purity)
3. **RVV intrinsics vs inline assembly**: Use GCC RVV intrinsics (more portable) or inline assembly (more educational)?
   - Recommendation: Both -- intrinsics for functional tests, inline asm for learning exercises
4. **Should we include a simple memory allocator** (bump allocator) for dynamic allocation demos?

---

*Next: See [03-platform-configurations.md](03-platform-configurations.md) for multi-core/multi-processor configuration designs.*
