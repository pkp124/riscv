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

    /* Platform-specific initialization can go here */
}

const char *platform_get_name(void)
{
    return PLATFORM_NAME;
}
