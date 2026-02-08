/**
 * @file platform.c
 * @brief Platform initialization and utilities
 */

#include "platform.h"

#include "csr.h"
#include "uart.h"

void platform_init(void)
{
    /* Initialize UART */
    uart_init();

    /* Platform-specific initialization can go here */
    /* For QEMU virt machine, UART is all we need for Phase 2 */
}

const char *platform_get_name(void)
{
    return PLATFORM_NAME;
}
