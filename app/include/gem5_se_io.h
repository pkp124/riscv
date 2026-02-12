/**
 * @file gem5_se_io.h
 * @brief gem5 Syscall Emulation (SE) mode I/O interface
 *
 * In gem5 SE mode, there is no MMIO UART. Instead, I/O is performed
 * using Linux-style syscalls (ecall). gem5 intercepts these and
 * emulates them on the host.
 *
 * Syscall numbers (RISC-V Linux ABI):
 *   - write(fd, buf, len):  a7 = 64
 *   - exit(code):           a7 = 93
 *   - exit_group(code):     a7 = 94
 *
 * This module is only active when GEM5_MODE_SE is defined.
 */

#ifndef GEM5_SE_IO_H
#define GEM5_SE_IO_H

#include <stddef.h>

/**
 * @brief Initialize gem5 SE I/O (no-op, SE handles setup)
 */
void gem5_se_init(void);

/**
 * @brief Write a single character to stdout via syscall
 * @param c Character to write
 */
void gem5_se_putc(char c);

/**
 * @brief Write a null-terminated string to stdout via syscall
 * @param s String to write
 */
void gem5_se_puts(const char *s);

/**
 * @brief Write a buffer to stdout via syscall
 * @param buf Buffer to write
 * @param len Length of buffer
 */
void gem5_se_write(const char *buf, size_t len);

/**
 * @brief Exit the simulation via exit syscall
 * @param exit_code Exit code (0 = success)
 */
void gem5_se_exit(int exit_code);

#endif /* GEM5_SE_IO_H */
