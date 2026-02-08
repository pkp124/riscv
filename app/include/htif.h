/**
 * @file htif.h
 * @brief HTIF (Host-Target Interface) driver for Spike
 *
 * HTIF is Spike's I/O mechanism using tohost/fromhost registers.
 * This provides basic console output for Spike simulator.
 */

#ifndef HTIF_H
#define HTIF_H

#include <stddef.h>

/**
 * @brief Initialize HTIF
 */
void htif_init(void);

/**
 * @brief Write a single character via HTIF
 * @param c Character to write
 */
void htif_putc(char c);

/**
 * @brief Write a null-terminated string via HTIF
 * @param s String to write
 */
void htif_puts(const char *s);

/**
 * @brief Write a buffer via HTIF
 * @param buf Buffer to write
 * @param len Length of buffer
 */
void htif_write(const char *buf, size_t len);

/**
 * @brief Shutdown simulator via HTIF
 * @param exit_code Exit code (0 = success)
 */
void htif_poweroff(int exit_code);

#endif /* HTIF_H */
