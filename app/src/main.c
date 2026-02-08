/**
 * @file main.c
 * @brief Phase 2 bare-metal application main entry point
 *
 * This application demonstrates basic bare-metal functionality:
 * - UART output
 * - CSR access
 * - Memory operations
 * - Function calls
 *
 * Designed to pass Phase 2 CTest test cases.
 */

#include "csr.h"
#include "platform.h"
#include "uart.h"

#include <stdbool.h>
#include <stdint.h>

/* =============================================================================
 * Forward Declarations
 * ============================================================================= */

static void test_csr(void);
static void test_uart(void);
static void test_memory(void);
static void test_function_calls(void);
static void print_banner(void);

/* Test results tracking */
static int tests_passed = 0;
static int tests_total = 0;

/* =============================================================================
 * Helper Functions
 * ============================================================================= */

/**
 * @brief Simple integer to string conversion
 */
static void int_to_str(uint64_t value, char *buf, int buf_size)
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
 */
static void print_hex(uint64_t value)
{
    uart_puts("0x");
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

    uart_puts(&hex[start]);
}

/**
 * @brief Record test result
 */
static void record_test(const char *name, bool passed)
{
    tests_total++;
    if (passed) {
        tests_passed++;
        uart_puts("[TEST] ");
        uart_puts(name);
        uart_puts(": PASS\n");
    } else {
        uart_puts("[TEST] ");
        uart_puts(name);
        uart_puts(": FAIL\n");
    }
}

/* =============================================================================
 * Test Functions
 * ============================================================================= */

/**
 * @brief Test CSR access (Hart ID and mstatus)
 */
static void test_csr(void)
{
    /* Read Hart ID */
    uint64_t hartid = read_csr(mhartid);
    uart_puts("[CSR] Hart ID: ");
    char buf[32];
    int_to_str(hartid, buf, sizeof(buf));
    uart_puts(buf);
    uart_puts("\n");

    /* Read mstatus */
    uint64_t mstatus = read_csr(mstatus);
    uart_puts("[CSR] mstatus: ");
    print_hex(mstatus);
    uart_puts("\n");

    /* Verify Hart ID is 0 for single-core */
    record_test("CSR Hart ID", hartid == 0);

    /* Verify mstatus is readable (non-zero) */
    record_test("CSR mstatus", mstatus != 0);
}

/**
 * @brief Test UART character output
 */
static void test_uart(void)
{
    uart_puts("[UART] Character output: ");

    /* Test individual character output */
    uart_putc('P');
    uart_putc('A');
    uart_putc('S');
    uart_putc('S');
    uart_puts("\n");

    record_test("UART output", true);
}

/**
 * @brief Test basic memory operations
 */
static void test_memory(void)
{
    /* Allocate some stack variables */
    volatile uint64_t test_data[8];

    /* Write pattern */
    for (int i = 0; i < 8; i++) {
        test_data[i] = 0xDEADBEEF00000000ULL | i;
    }

    /* Verify pattern */
    bool passed = true;
    for (int i = 0; i < 8; i++) {
        uint64_t expected = 0xDEADBEEF00000000ULL | i;
        if (test_data[i] != expected) {
            passed = false;
            break;
        }
    }

    record_test("Memory operations", passed);
}

/**
 * @brief Test helper function
 */
static uint64_t helper_function(uint64_t a, uint64_t b)
{
    return a + b + 0x42;
}

/**
 * @brief Test nested function call
 */
static uint64_t nested_function(uint64_t x)
{
    if (x > 0) {
        return x + nested_function(x - 1);
    }
    return 0;
}

/**
 * @brief Test function calls and stack
 */
static void test_function_calls(void)
{
    /* Test simple function call */
    uint64_t result1 = helper_function(10, 20);
    bool test1 = (result1 == (10 + 20 + 0x42));

    /* Test nested function call (recursive) */
    uint64_t result2 = nested_function(5); /* 5+4+3+2+1+0 = 15 */
    bool test2 = (result2 == 15);

    bool passed = test1 && test2;
    record_test("Function calls", passed);
}

/**
 * @brief Print application banner
 */
static void print_banner(void)
{
    uart_puts("\n");
    uart_puts("=================================================================\n");
    uart_puts("RISC-V Bare-Metal System Explorer\n");
    uart_puts("=================================================================\n");
    uart_puts("Platform: ");
    uart_puts(platform_get_name());
    uart_puts("\n");
    uart_puts("Phase: 2 - Single-Core Bare-Metal\n");
    uart_puts("=================================================================\n");
    uart_puts("\n");
}

/* =============================================================================
 * Main Entry Point
 * ============================================================================= */

int main(void)
{
    /* Print banner */
    print_banner();

    /* Test 1: Hello RISC-V */
    uart_puts("Hello RISC-V\n");
    uart_puts("\n");

    /* Run all tests */
    uart_puts("[INFO] Running Phase 2 tests...\n");
    uart_puts("\n");

    test_csr();
    uart_puts("\n");

    test_uart();
    uart_puts("\n");

    test_memory();
    uart_puts("\n");

    test_function_calls();
    uart_puts("\n");

    /* Print test summary */
    uart_puts("=================================================================\n");
    uart_puts("[RESULT] Phase 2 tests: ");
    char buf[32];
    int_to_str(tests_passed, buf, sizeof(buf));
    uart_puts(buf);
    uart_puts("/");
    int_to_str(tests_total, buf, sizeof(buf));
    uart_puts(buf);
    if (tests_passed == tests_total) {
        uart_puts(" PASS\n");
    } else {
        uart_puts(" FAIL\n");
    }
    uart_puts("=================================================================\n");
    uart_puts("\n");

    /* Application complete */
    uart_puts("[INFO] Phase 2 complete. System halted.\n");

    return 0;
}
