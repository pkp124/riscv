/**
 * @file gem5_se_io.c
 * @brief gem5 Syscall Emulation (SE) mode I/O implementation
 *
 * Uses RISC-V semihosting (ARM ABI) for console output and exit.
 * RiscvBareMetal + RiscvSemihosting in gem5 SE mode handles the ebreak
 * trap sequence and emulates SYS_WRITE0 / SYS_EXIT.
 *
 * Semihosting trap sequence (riscv-semihosting spec):
 *   slli x0, x0, 0x1f  ; 0x01f01013 (prefix)
 *   ebreak             ; 0x00100073
 *   srai x0, x0, 7     ; 0x40705013 (suffix)
 *
 * Registers: a0 = operation, a1 = parameter
 *
 * This is only compiled when PLATFORM_GEM5 and GEM5_MODE_SE are defined.
 */

#include "gem5_se_io.h"

#include <stddef.h>
#include <stdint.h>

/* Only compile for gem5 SE mode */
#if defined(PLATFORM_GEM5) && defined(GEM5_MODE_SE)

/* =============================================================================
 * RISC-V Semihosting Operation Numbers (ARM ABI)
 * ============================================================================= */

#define SYS_WRITE0 0x04   /* Write null-terminated string to debug channel */
#define SYS_EXIT   0x18   /* Application exit */

/* ADP_Stopped_ApplicationExit - report normal application exit */
#define SEMI_EXIT_TYPE 0x20026ULL

/* =============================================================================
 * Semihosting Parameter Block for SYS_EXIT
 * ============================================================================= */

static struct {
    uint64_t type;
    uint64_t subcode;
} semi_exit_block;

/* =============================================================================
 * Semihosting Trap Interface
 * ============================================================================= */

/**
 * @brief Perform a RISC-V semihosting call
 *
 * a0 = operation number, a1 = parameter (pointer or value)
 * Emits the 3-instruction trap sequence that gem5 recognizes.
 */
static inline void semi_call(uint32_t op, uint64_t arg)
{
    register uint32_t a0_val __asm__("a0") = op;
    register uint64_t a1_val __asm__("a1") = arg;

    __asm__ __volatile__(
        ".word 0x01f01013\n" /* slli x0, x0, 0x1f (prefix) */
        ".word 0x00100073\n" /* ebreak */
        ".word 0x40705013\n" /* srai x0, x0, 7 (suffix) */
        : "+r"(a0_val), "+r"(a1_val)
        :
        : "memory"
    );
}

/* =============================================================================
 * gem5 SE I/O Implementation
 * ============================================================================= */

void gem5_se_init(void)
{
    /* No initialization needed for semihosting */
}

void gem5_se_putc(char c)
{
    /* SYS_WRITE0 expects pointer to null-terminated string */
    char buf[2] = {c, '\0'};
    semi_call(SYS_WRITE0, (uint64_t)(uintptr_t)buf);
}

void gem5_se_puts(const char *s)
{
    if (s == (void *)0) {
        return;
    }
    semi_call(SYS_WRITE0, (uint64_t)(uintptr_t)s);
}

void gem5_se_write(const char *buf, size_t len)
{
    if (buf == (void *)0 || len == 0) {
        return;
    }
    /* SYS_WRITE0 writes until null; use SYS_WRITEC in a loop for binary data,
     * or write in chunks. For text we can copy to a temp buffer with null. */
    for (size_t i = 0; i < len; i++) {
        gem5_se_putc(buf[i]);
    }
}

void gem5_se_exit(int exit_code)
{
    semi_exit_block.type = SEMI_EXIT_TYPE;
    semi_exit_block.subcode = (uint64_t)(unsigned int)exit_code;
    semi_call(SYS_EXIT, (uint64_t)(uintptr_t)&semi_exit_block);

    /* Should not reach here; loop if semihosting did not exit */
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
