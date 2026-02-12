/**
 * @file console.h
 * @brief Console abstraction layer
 *
 * Provides platform-independent console I/O macros.
 * Maps to UART for QEMU/gem5/Renode and HTIF for Spike.
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>

/* Platform-specific console mapping */
#if defined(PLATFORM_QEMU_VIRT) || defined(PLATFORM_GEM5) || defined(PLATFORM_RENODE)
#include "uart.h"
#define console_puts uart_puts
#define console_putc uart_putc
#elif defined(PLATFORM_SPIKE)
#include "htif.h"
#define console_puts htif_puts
#define console_putc htif_putc
#else
#error "No platform defined for console output"
#endif

/**
 * @brief Simple integer to string conversion (decimal)
 * @param value Value to convert
 * @param buf Output buffer
 * @param buf_size Size of output buffer
 */
static inline void console_put_dec(uint64_t value, char *buf, int buf_size)
{
    int i = 0;
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    /* Convert digits in reverse */
    char temp[32];
    while (value > 0 && i < 31) {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    }
    temp[i] = '\0';

    /* Reverse the string */
    int len = i;
    for (int j = 0; j < len && j < buf_size - 1; j++) {
        buf[j] = temp[len - 1 - j];
    }
    buf[len < buf_size ? len : buf_size - 1] = '\0';
}

/**
 * @brief Print a uint64_t value in hex
 * @param value Value to print
 */
static inline void console_put_hex(uint64_t value)
{
    console_puts("0x");
    char hex[17];
    const char hexchars[] = "0123456789ABCDEF";

    for (int i = 15; i >= 0; i--) {
        hex[15 - i] = hexchars[(value >> (i * 4)) & 0xF];
    }
    hex[16] = '\0';

    /* Skip leading zeros */
    int start = 0;
    while (start < 15 && hex[start] == '0') {
        start++;
    }

    console_puts(&hex[start]);
}

#endif /* CONSOLE_H */
