/**
 * @file platform.h
 * @brief Platform abstraction layer for RISC-V bare-metal applications
 *
 * This header provides platform-specific definitions and abstractions
 * for different RISC-V simulation platforms (QEMU, Spike, gem5, Renode).
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* =============================================================================
 * Platform Detection
 * ============================================================================= */

#if defined(PLATFORM_QEMU_VIRT)
#define PLATFORM_NAME "QEMU virt"
#elif defined(PLATFORM_SPIKE)
#define PLATFORM_NAME "Spike"
#elif defined(PLATFORM_GEM5)
#define PLATFORM_NAME "gem5"
#elif defined(PLATFORM_RENODE)
#define PLATFORM_NAME "Renode"
#else
#error "No platform defined! Use -DPLATFORM_QEMU_VIRT or similar"
#endif

/* =============================================================================
 * Memory Map
 * ============================================================================= */

/* RAM base address (standard RISC-V) */
#define RAM_BASE 0x80000000UL
#define RAM_SIZE (128 * 1024 * 1024) /* 128 MB */

/* UART base addresses (platform-specific) */
#if defined(PLATFORM_QEMU_VIRT) || defined(PLATFORM_GEM5) || defined(PLATFORM_RENODE)
#define UART_BASE 0x10000000UL /* NS16550A UART */
#elif defined(PLATFORM_SPIKE)
/* Spike uses HTIF (Host-Target Interface), not MMIO UART.
 * The tohost/fromhost symbols are defined in the linker script (spike.ld)
 * and referenced directly in htif.c as extern variables.
 * Spike finds them by name in the ELF symbol table. */
#endif

/* QEMU virt machine: sifive_test device for clean exit */
#if defined(PLATFORM_QEMU_VIRT)
#define VIRT_TEST_BASE 0x100000UL
#define VIRT_TEST_FINISHER_PASS 0x5555
#define VIRT_TEST_FINISHER_FAIL 0x3333
#endif

/* CLINT (Core-Local Interruptor) */
#define CLINT_BASE 0x02000000UL
#define CLINT_MSIP (CLINT_BASE + 0x0000)     /* Machine Software Interrupt Pending */
#define CLINT_MTIMECMP (CLINT_BASE + 0x4000) /* Machine Time Compare */
#define CLINT_MTIME (CLINT_BASE + 0xBFF8)    /* Machine Time */

/* PLIC (Platform-Level Interrupt Controller) */
#define PLIC_BASE 0x0C000000UL

/* =============================================================================
 * Hart (Hardware Thread) Configuration
 * ============================================================================= */

#ifndef NUM_HARTS
#define NUM_HARTS 1
#endif

#if NUM_HARTS > 1
#define IS_SMP 1
#else
#define IS_SMP 0
#endif

/* =============================================================================
 * Platform Initialization
 * ============================================================================= */

/**
 * @brief Initialize platform-specific hardware
 */
void platform_init(void);

/**
 * @brief Clean shutdown (platform-specific)
 *
 * On Spike: sends HTIF poweroff command
 * On QEMU/gem5/Renode: enters infinite WFI loop
 *
 * @param exit_code Exit code (0 = success)
 */
void platform_exit(int exit_code);

/**
 * @brief Get platform name string
 * @return Platform name
 */
const char *platform_get_name(void);

/* =============================================================================
 * Utility Macros
 * ============================================================================= */

/* Memory barriers */
#define mb() __asm__ __volatile__("fence" ::: "memory")
#define rmb() __asm__ __volatile__("fence r,r" ::: "memory")
#define wmb() __asm__ __volatile__("fence w,w" ::: "memory")

/* Compiler barriers */
#define barrier() __asm__ __volatile__("" ::: "memory")

/* Wait for interrupt */
#define wfi() __asm__ __volatile__("wfi")

/* =============================================================================
 * Linker Symbols
 * ============================================================================= */

/* Provided by linker script */
extern char __text_start[];
extern char __text_end[];
extern char __rodata_start[];
extern char __rodata_end[];
extern char __data_start[];
extern char __data_end[];
extern char __bss_start[];
extern char __bss_end[];
extern char __heap_start[];
extern char __heap_end[];
extern char __stack_start[];
extern char __stack_end[];
extern char __stack_top[];

#endif /* PLATFORM_H */
