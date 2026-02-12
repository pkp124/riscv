/**
 * @file htif.c
 * @brief HTIF (Host-Target Interface) implementation for Spike
 *
 * Spike uses HTIF for host-target communication instead of MMIO UART.
 * HTIF uses two special memory locations: tohost and fromhost.
 *
 * For console output, we use the syscall emulation interface.
 */

#include "htif.h"

#include "platform.h"

#include <stdbool.h>
#include <stdint.h>

/* Only compile for Spike platform */
#ifdef PLATFORM_SPIKE

/* =============================================================================
 * HTIF Protocol
 * ============================================================================= */

/* HTIF device and command codes */
#define HTIF_DEV_SYSCALL 0
#define HTIF_DEV_CONSOLE 1

#define HTIF_CMD_WRITE 1
#define HTIF_CMD_READ 0

/* Construct HTIF command */
#define HTIF_CMD(dev, cmd, data)                                                                   \
    (((uint64_t) (dev) << 56) | ((uint64_t) (cmd) << 48) | ((data) & 0xFFFFFFFFFFFFULL))

/*
 * HTIF tohost/fromhost registers.
 * These symbols are defined in the Spike linker script (spike.ld).
 * Spike finds them by name in the ELF symbol table.
 */
extern volatile uint64_t tohost;
extern volatile uint64_t fromhost;

/* =============================================================================
 * HTIF Implementation
 * ============================================================================= */

void htif_init(void)
{
    /* Ensure HTIF is in a clean state */
    tohost = 0;
    fromhost = 0;
}

void htif_putc(char c)
{
    /* Wait for previous command to complete */
    while (tohost != 0) {
        fromhost = 0;
    }

    /* Write character using console device */
    tohost = HTIF_CMD(HTIF_DEV_CONSOLE, HTIF_CMD_WRITE, (uint8_t) c);

    /* Wait for completion */
    while (tohost != 0) {
        fromhost = 0;
    }
}

void htif_puts(const char *s)
{
    if (s == NULL) {
        return;
    }

    while (*s) {
        htif_putc(*s++);
    }
}

void htif_write(const char *buf, size_t len)
{
    if (buf == NULL) {
        return;
    }

    for (size_t i = 0; i < len; i++) {
        htif_putc(buf[i]);
    }
}

void htif_poweroff(int exit_code)
{
    /* Exit command: dev=0 (syscall), cmd=0, data=(code << 1) | 1 */
    tohost = HTIF_CMD(0, 0, (exit_code << 1) | 1);

    /* Should not return */
    while (1) {
        __asm__ __volatile__("wfi");
    }
}

#else /* Not Spike */

/* Stub implementations for platforms without HTIF */
void htif_init(void)
{
}
void htif_putc(char c)
{
    (void) c;
}
void htif_puts(const char *s)
{
    (void) s;
}
void htif_write(const char *buf, size_t len)
{
    (void) buf;
    (void) len;
}
void htif_poweroff(int exit_code)
{
    (void) exit_code;
}

#endif /* PLATFORM_SPIKE */
