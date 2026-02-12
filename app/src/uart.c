/**
 * @file uart.c
 * @brief UART driver implementation for NS16550A
 *
 * Simple polled UART driver for QEMU virt machine, gem5, and Renode.
 * The NS16550A is a standard UART controller.
 *
 * Note: This file is only compiled for platforms with MMIO UART.
 * Spike uses HTIF instead (will be implemented in Phase 3).
 */

#include "uart.h"

#include "platform.h"

#include <stdbool.h>
#include <stdint.h>

/* Only compile for platforms that have MMIO UART.
 * gem5 SE mode does NOT have UART (uses syscall-based I/O instead). */
#if defined(PLATFORM_QEMU_VIRT) || (defined(PLATFORM_GEM5) && !defined(GEM5_MODE_SE)) ||           \
    defined(PLATFORM_RENODE)

/* =============================================================================
 * NS16550A Register Offsets
 * ============================================================================= */

#define UART_RBR_OFFSET 0 /* Receiver Buffer Register (read) */
#define UART_THR_OFFSET 0 /* Transmitter Holding Register (write) */
#define UART_DLL_OFFSET 0 /* Divisor Latch Low (when DLAB=1) */
#define UART_IER_OFFSET 1 /* Interrupt Enable Register */
#define UART_DLH_OFFSET 1 /* Divisor Latch High (when DLAB=1) */
#define UART_IIR_OFFSET 2 /* Interrupt Identification Register (read) */
#define UART_FCR_OFFSET 2 /* FIFO Control Register (write) */
#define UART_LCR_OFFSET 3 /* Line Control Register */
#define UART_MCR_OFFSET 4 /* Modem Control Register */
#define UART_LSR_OFFSET 5 /* Line Status Register */
#define UART_MSR_OFFSET 6 /* Modem Status Register */
#define UART_SCR_OFFSET 7 /* Scratch Register */

/* =============================================================================
 * NS16550A Register Bit Definitions
 * ============================================================================= */

/* Line Control Register (LCR) */
#define UART_LCR_DLAB (1 << 7) /* Divisor Latch Access Bit */
#define UART_LCR_8N1 0x03      /* 8 data bits, no parity, 1 stop bit */

/* Line Status Register (LSR) */
#define UART_LSR_DR (1 << 0)   /* Data Ready */
#define UART_LSR_THRE (1 << 5) /* Transmitter Holding Register Empty */

/* FIFO Control Register (FCR) */
#define UART_FCR_ENABLE (1 << 0) /* Enable FIFO */
#define UART_FCR_CLEAR (0x06)    /* Clear RX and TX FIFOs */

/* =============================================================================
 * Register Access Macros
 * ============================================================================= */

#define UART_REG(offset) (*(volatile uint8_t *) (UART_BASE + (offset)))

/* =============================================================================
 * UART Implementation
 * ============================================================================= */

void uart_init(void)
{
    /* Set baud rate divisor (115200 baud at 1.8432 MHz clock) */
    /* Divisor = clock / (16 * baud_rate) */
    /* For QEMU, baud rate is not critical (simulated) */

    /* Set DLAB to access divisor registers */
    UART_REG(UART_LCR_OFFSET) = UART_LCR_DLAB;

    /* Set divisor to 1 (doesn't matter in simulation) */
    UART_REG(UART_DLL_OFFSET) = 0x01;
    UART_REG(UART_DLH_OFFSET) = 0x00;

    /* Clear DLAB and set 8N1 mode (8 data bits, no parity, 1 stop bit) */
    UART_REG(UART_LCR_OFFSET) = UART_LCR_8N1;

    /* Enable and clear FIFOs */
    UART_REG(UART_FCR_OFFSET) = UART_FCR_ENABLE | UART_FCR_CLEAR;

    /* Disable interrupts (we're using polling) */
    UART_REG(UART_IER_OFFSET) = 0x00;
}

void uart_putc(char c)
{
    /* Wait until THR is empty (ready to transmit) */
    while ((UART_REG(UART_LSR_OFFSET) & UART_LSR_THRE) == 0) {
        /* Busy wait */
    }

    /* Write character to THR */
    UART_REG(UART_THR_OFFSET) = (uint8_t) c;
}

void uart_puts(const char *s)
{
    if (s == NULL) {
        return;
    }

    while (*s) {
        /* Convert \n to \r\n for proper line endings */
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

void uart_write(const char *buf, size_t len)
{
    if (buf == NULL) {
        return;
    }

    for (size_t i = 0; i < len; i++) {
        uart_putc(buf[i]);
    }
}

char uart_getc(void)
{
    /* Wait until data is ready */
    while ((UART_REG(UART_LSR_OFFSET) & UART_LSR_DR) == 0) {
        /* Busy wait */
    }

    /* Read character from RBR */
    return (char) UART_REG(UART_RBR_OFFSET);
}

bool uart_can_read(void)
{
    return (UART_REG(UART_LSR_OFFSET) & UART_LSR_DR) != 0;
}

#else /* Spike, gem5 SE, or other platforms without MMIO UART */

/* Stub implementations for platforms without UART (e.g., Spike uses HTIF, gem5 SE uses syscalls) */
void uart_init(void)
{
    /* No UART on this platform */
}

void uart_putc(char c)
{
    (void) c; /* Unused */
    /* UART not available - use platform-specific I/O (HTIF for Spike) */
}

void uart_puts(const char *s)
{
    (void) s; /* Unused */
    /* UART not available - use platform-specific I/O (HTIF for Spike) */
}

void uart_write(const char *buf, size_t len)
{
    (void) buf; /* Unused */
    (void) len; /* Unused */
    /* UART not available - use platform-specific I/O (HTIF for Spike) */
}

char uart_getc(void)
{
    /* UART not available */
    return '\0';
}

bool uart_can_read(void)
{
    /* UART not available */
    return false;
}

#endif /* PLATFORM_QEMU_VIRT || (PLATFORM_GEM5 && !GEM5_MODE_SE) || PLATFORM_RENODE */
