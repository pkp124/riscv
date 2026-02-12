/**
 * @file gem5_se_io.c
 * @brief gem5 Syscall Emulation (SE) mode I/O implementation
 *
 * Uses RISC-V Linux syscalls via ecall for console output and exit.
 * gem5 SE mode intercepts ecall instructions and emulates them.
 *
 * This is only compiled when PLATFORM_GEM5 and GEM5_MODE_SE are defined.
 */

#include "gem5_se_io.h"

#include <stddef.h>
#include <stdint.h>

/* Only compile for gem5 SE mode */
#if defined(PLATFORM_GEM5) && defined(GEM5_MODE_SE)

/* =============================================================================
 * RISC-V Linux Syscall Numbers
 * ============================================================================= */

#define SYS_write 64
#define SYS_exit 93
#define SYS_exit_group 94

/* File descriptors */
#define STDOUT_FD 1

/* =============================================================================
 * Syscall Interface
 * ============================================================================= */

/**
 * @brief Execute a RISC-V syscall with 3 arguments
 *
 * Convention: a7 = syscall number, a0-a2 = arguments, a0 = return value
 */
static inline long syscall3(long number, long arg0, long arg1, long arg2)
{
    register long a7 __asm__("a7") = number;
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;

    __asm__ __volatile__("ecall" : "+r"(a0) : "r"(a7), "r"(a1), "r"(a2) : "memory");
    return a0;
}

/**
 * @brief Execute a RISC-V syscall with 1 argument
 */
static inline long syscall1(long number, long arg0)
{
    register long a7 __asm__("a7") = number;
    register long a0 __asm__("a0") = arg0;

    __asm__ __volatile__("ecall" : "+r"(a0) : "r"(a7) : "memory");
    return a0;
}

/* =============================================================================
 * gem5 SE I/O Implementation
 * ============================================================================= */

void gem5_se_init(void)
{
    /* No initialization needed for SE mode */
}

void gem5_se_putc(char c)
{
    syscall3(SYS_write, STDOUT_FD, (long) &c, 1);
}

void gem5_se_puts(const char *s)
{
    if (s == (void *) 0) {
        return;
    }

    /* Calculate string length */
    size_t len = 0;
    const char *p = s;
    while (*p++) {
        len++;
    }

    if (len > 0) {
        syscall3(SYS_write, STDOUT_FD, (long) s, (long) len);
    }
}

void gem5_se_write(const char *buf, size_t len)
{
    if (buf == (void *) 0 || len == 0) {
        return;
    }

    syscall3(SYS_write, STDOUT_FD, (long) buf, (long) len);
}

void gem5_se_exit(int exit_code)
{
    syscall1(SYS_exit_group, (long) exit_code);

    /* Should not reach here, but loop just in case */
    while (1) {
        __asm__ __volatile__("wfi");
    }
}

#else /* Not gem5 SE mode */

/* Stub implementations for non-SE builds */
void gem5_se_init(void)
{
}
void gem5_se_putc(char c)
{
    (void) c;
}
void gem5_se_puts(const char *s)
{
    (void) s;
}
void gem5_se_write(const char *buf, size_t len)
{
    (void) buf;
    (void) len;
}
void gem5_se_exit(int exit_code)
{
    (void) exit_code;
}

#endif /* PLATFORM_GEM5 && GEM5_MODE_SE */
