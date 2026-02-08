/**
 * @file uart.h
 * @brief UART driver for NS16550A (QEMU virt, gem5, Renode)
 * 
 * Simple polled UART driver for bare-metal applications.
 * No interrupts, no buffering - direct register access.
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* =============================================================================
 * UART API
 * ============================================================================= */

/**
 * @brief Initialize the UART
 */
void uart_init(void);

/**
 * @brief Write a single character to UART
 * @param c Character to write
 */
void uart_putc(char c);

/**
 * @brief Write a null-terminated string to UART
 * @param s String to write
 */
void uart_puts(const char *s);

/**
 * @brief Write a buffer to UART
 * @param buf Buffer to write
 * @param len Length of buffer
 */
void uart_write(const char *buf, size_t len);

/**
 * @brief Read a single character from UART (blocking)
 * @return Character read
 */
char uart_getc(void);

/**
 * @brief Check if a character is available to read
 * @return true if character available, false otherwise
 */
bool uart_can_read(void);

#endif /* UART_H */
