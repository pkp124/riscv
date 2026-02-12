/**
 * @file platform.c
 * @brief Platform initialization and utilities
 */

#include "platform.h"

#include "csr.h"

/* Include platform-specific I/O drivers */
#if defined(PLATFORM_QEMU_VIRT) || defined(PLATFORM_GEM5) || defined(PLATFORM_RENODE)
#include "uart.h"
#elif defined(PLATFORM_SPIKE)
#include "htif.h"
#endif

void platform_init(void)
{
    /* Initialize platform-specific I/O */
#if defined(PLATFORM_QEMU_VIRT) || defined(PLATFORM_GEM5) || defined(PLATFORM_RENODE)
    uart_init();
#elif defined(PLATFORM_SPIKE)
    htif_init();
#endif

#if defined(ENABLE_RVV)
    /* Enable the floating-point unit (mstatus.FS = Initial).
     * Required for FP vector instructions which use fcsr rounding mode. */
    {
        unsigned long mstatus_val = read_csr(mstatus);
        mstatus_val |= (1UL << 13); /* Set FS = Initial (01) at bits [14:13] */
        write_csr(mstatus, mstatus_val);
    }
    /* Enable the vector unit by setting mstatus.VS = Initial (01).
     * Without this, any vector instruction will trap as illegal. */
    {
        unsigned long mstatus_val = read_csr(mstatus);
        mstatus_val &= ~(3UL << 9); /* Clear VS field [10:9] */
        mstatus_val |= (1UL << 9);  /* Set VS = Initial (01) */
        write_csr(mstatus, mstatus_val);
    }
#endif

    /* Platform-specific initialization can go here */
}

void platform_exit(int exit_code)
{
#if defined(PLATFORM_SPIKE)
    /* Spike: use HTIF to tell the host to shutdown */
    htif_poweroff(exit_code);
#elif defined(PLATFORM_QEMU_VIRT)
    /* QEMU virt: use sifive_test device for clean exit */
    volatile uint32_t *test_dev = (volatile uint32_t *) VIRT_TEST_BASE;
    if (exit_code == 0) {
        *test_dev = VIRT_TEST_FINISHER_PASS;
    } else {
        *test_dev = VIRT_TEST_FINISHER_FAIL;
    }
#endif
    /* Fallback: enter low-power infinite loop */
    (void) exit_code;
    while (1) {
        __asm__ __volatile__("wfi");
    }
}

const char *platform_get_name(void)
{
    return PLATFORM_NAME;
}
