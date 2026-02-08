/**
 * @file platform.c
 * @brief Platform initialization and utilities
 */

#include "platform.h"
#include "uart.h"
#include "csr.h"

void platform_init(void) {
    /* Initialize UART */
    uart_init();
    
    /* Platform-specific initialization can go here */
    /* For QEMU virt machine, UART is all we need for Phase 2 */
}

const char* platform_get_name(void) {
    return PLATFORM_NAME;
}
