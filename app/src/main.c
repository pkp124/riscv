/**
 * @file main.c
 * @brief Bare-metal application main entry point
 *
 * Phase 2 (NUM_HARTS == 1): Single-core tests
 *   - Console output, CSR access, memory operations, function calls
 *
 * Phase 4 (NUM_HARTS > 1): Multi-core SMP tests
 *   - SMP boot verification (all harts online)
 *   - Spinlock correctness
 *   - Atomic operations
 *   - Barrier synchronization
 *
 * Designed to pass Phase 2 and Phase 4 CTest test cases.
 */

#include "console.h"
#include "csr.h"
#include "platform.h"

#include <stdbool.h>
#include <stdint.h>

#if NUM_HARTS > 1
#include "smp.h"
#endif

/* =============================================================================
 * Forward Declarations
 * ============================================================================= */

static void print_banner(void);

/* Test results tracking */
static int tests_passed;
static int tests_total;

/* =============================================================================
 * Helper Functions
 * ============================================================================= */

/**
 * @brief Simple integer to string conversion
 */
static void int_to_str(uint64_t value, char *buf, int buf_size)
{
    console_put_dec(value, buf, buf_size);
}

#if NUM_HARTS <= 1
/**
 * @brief Print a uint64_t value in hex (used in Phase 2 CSR test)
 */
static void print_hex(uint64_t value)
{
    console_put_hex(value);
}
#endif

/**
 * @brief Record test result
 */
static void record_test(const char *name, bool passed)
{
    tests_total++;
    if (passed) {
        tests_passed++;
        console_puts("[TEST] ");
        console_puts(name);
        console_puts(": PASS\n");
    } else {
        console_puts("[TEST] ");
        console_puts(name);
        console_puts(": FAIL\n");
    }
}

/**
 * @brief Print test summary and phase completion
 */
static void print_summary(int phase)
{
    char buf[32];

    console_puts("=================================================================\n");
    console_puts("[RESULT] Phase ");
    int_to_str((uint64_t) phase, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" tests: ");
    int_to_str((uint64_t) tests_passed, buf, sizeof(buf));
    console_puts(buf);
    console_puts("/");
    int_to_str((uint64_t) tests_total, buf, sizeof(buf));
    console_puts(buf);
    if (tests_passed == tests_total) {
        console_puts(" PASS\n");
    } else {
        console_puts(" FAIL\n");
    }
    console_puts("=================================================================\n");
    console_puts("\n");

    console_puts("[INFO] Phase ");
    int_to_str((uint64_t) phase, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" complete. System halted.\n");
}

/* =============================================================================
 * Phase 2: Single-Core Tests
 * ============================================================================= */

#if NUM_HARTS <= 1

static void test_csr(void)
{
    uint64_t hartid = read_csr(mhartid);
    console_puts("[CSR] Hart ID: ");
    char buf[32];
    int_to_str(hartid, buf, sizeof(buf));
    console_puts(buf);
    console_puts("\n");

    uint64_t mstatus = read_csr(mstatus);
    console_puts("[CSR] mstatus: ");
    print_hex(mstatus);
    console_puts("\n");

    record_test("CSR Hart ID", hartid == 0);
    record_test("CSR mstatus", mstatus != 0);
}

static void test_uart(void)
{
    console_puts("[UART] Character output: ");
    console_putc('P');
    console_putc('A');
    console_putc('S');
    console_putc('S');
    console_puts("\n");
    record_test("UART output", true);
}

static void test_memory(void)
{
    volatile uint64_t test_data[8];

    for (int i = 0; i < 8; i++) {
        test_data[i] = 0xDEADBEEF00000000ULL | (uint64_t) i;
    }

    bool passed = true;
    for (int i = 0; i < 8; i++) {
        uint64_t expected = 0xDEADBEEF00000000ULL | (uint64_t) i;
        if (test_data[i] != expected) {
            passed = false;
            break;
        }
    }

    record_test("Memory operations", passed);
}

static uint64_t helper_function(uint64_t a, uint64_t b)
{
    return a + b + 0x42;
}

static uint64_t nested_function(uint64_t x)
{
    if (x > 0) {
        return x + nested_function(x - 1);
    }
    return 0;
}

static void test_function_calls(void)
{
    uint64_t result1 = helper_function(10, 20);
    bool test1 = (result1 == (10 + 20 + 0x42));

    uint64_t result2 = nested_function(5);
    bool test2 = (result2 == 15);

    bool passed = test1 && test2;
    record_test("Function calls", passed);
}

static void run_phase2_tests(void)
{
    console_puts("[INFO] Running Phase 2 tests...\n");
    console_puts("\n");

    test_csr();
    console_puts("\n");

    test_uart();
    console_puts("\n");

    test_memory();
    console_puts("\n");

    test_function_calls();
    console_puts("\n");
}

#endif /* NUM_HARTS <= 1 */

/* =============================================================================
 * Phase 4: SMP Tests
 * ============================================================================= */

#if NUM_HARTS > 1

/**
 * @brief Test 1: SMP Boot - all harts come online
 */
static void test_smp_boot(void)
{
    char buf[32];

    /* Initialize SMP subsystem */
    smp_init();

    console_puts("[SMP] Hart 0 online\n");

    /* Release secondary harts */
    console_puts("[SMP] Releasing secondary harts...\n");
    smp_release_harts();

    /* Wait for all secondary harts to come online */
    while (smp_get_harts_online() < (uint32_t) (NUM_HARTS - 1)) {
        /* Spin - secondary harts are incrementing the counter */
    }
    mb(); /* Ensure we see all their writes */

    console_puts("[SMP] All ");
    int_to_str(NUM_HARTS, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" harts online\n");

    record_test("SMP boot", true);
}

/**
 * @brief Test 2: Spinlock correctness
 *
 * All harts increment a shared counter under a spinlock.
 * If spinlock is correct, final count == NUM_HARTS.
 */
static void test_smp_spinlock(void)
{
    /* Reset counter */
    smp_lock_counter = 0;
    wmb();

    /* === Barrier 2: Signal spinlock test start === */
    barrier_wait(&smp_test_barrier);

    /* Hart 0 participates in spinlock test */
    spin_lock(&smp_test_lock);
    smp_lock_counter++;
    spin_unlock(&smp_test_lock);

    /* === Barrier 3: Wait for all harts to complete === */
    barrier_wait(&smp_test_barrier);

    bool passed = (smp_lock_counter == (uint32_t) NUM_HARTS);
    console_puts("[SMP] Spinlock counter: ");
    char buf[32];
    int_to_str(smp_lock_counter, buf, sizeof(buf));
    console_puts(buf);
    console_puts("/");
    int_to_str(NUM_HARTS, buf, sizeof(buf));
    console_puts(buf);
    console_puts("\n");

    record_test("Spinlock", passed);
}

/**
 * @brief Test 3: Atomic operations
 *
 * All harts atomically increment a shared counter using amoadd.
 * If atomic ops work correctly, final count == NUM_HARTS.
 */
static void test_smp_atomic(void)
{
    /* Reset counter */
    smp_atomic_counter = 0;
    wmb();

    /* === Barrier 4: Signal atomic test start === */
    barrier_wait(&smp_test_barrier);

    /* Hart 0 participates in atomic test */
    atomic_add_u32(&smp_atomic_counter, 1);

    /* === Barrier 5: Wait for all harts to complete === */
    barrier_wait(&smp_test_barrier);

    bool passed = (smp_atomic_counter == (uint32_t) NUM_HARTS);
    console_puts("[SMP] Atomic counter: ");
    char buf[32];
    int_to_str(smp_atomic_counter, buf, sizeof(buf));
    console_puts(buf);
    console_puts("/");
    int_to_str(NUM_HARTS, buf, sizeof(buf));
    console_puts(buf);
    console_puts("\n");

    record_test("Atomic operations", passed);
}

/**
 * @brief Test 4: Barrier synchronization
 *
 * If all previous barriers succeeded (which they must have for us
 * to reach here), barrier synchronization is working correctly.
 * Do one final barrier to confirm.
 */
static void test_smp_barrier(void)
{
    /* === Barrier 6: Final barrier (all harts must reach this) === */
    barrier_wait(&smp_test_barrier);

    /* If we got here, all 6 barriers worked correctly */
    record_test("Barrier synchronization", true);
}

static void run_phase4_tests(void)
{
    char buf[32];

    console_puts("[INFO] Running Phase 4 SMP tests with ");
    int_to_str(NUM_HARTS, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" harts...\n");
    console_puts("\n");

    /* Test 1: SMP Boot */
    test_smp_boot();
    console_puts("\n");

    /* === Barrier 1: Boot complete (synchronize all harts) === */
    barrier_wait(&smp_test_barrier);

    /* Test 2: Spinlock */
    test_smp_spinlock();
    console_puts("\n");

    /* Test 3: Atomic operations */
    test_smp_atomic();
    console_puts("\n");

    /* Test 4: Barrier synchronization */
    test_smp_barrier();
    console_puts("\n");
}

#endif /* NUM_HARTS > 1 */

/* =============================================================================
 * Banner
 * ============================================================================= */

static void print_banner(void)
{
    char buf[32];

    console_puts("\n");
    console_puts("=================================================================\n");
    console_puts("RISC-V Bare-Metal System Explorer\n");
    console_puts("=================================================================\n");
    console_puts("Platform: ");
    console_puts(platform_get_name());
    console_puts("\n");

#if NUM_HARTS > 1
    console_puts("Phase: 4 - Multi-Core SMP (");
    int_to_str(NUM_HARTS, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" harts)\n");
#else
    (void) buf;
    console_puts("Phase: 2 - Single-Core Bare-Metal\n");
#endif

    console_puts("=================================================================\n");
    console_puts("\n");
}

/* =============================================================================
 * Main Entry Point
 * ============================================================================= */

int main(void)
{
    /* Print banner */
    print_banner();

    /* Print hello message (common to all phases) */
    console_puts("Hello RISC-V\n");
    console_puts("\n");

    /* Run phase-appropriate tests */
#if NUM_HARTS > 1
    run_phase4_tests();
    print_summary(4);
#else
    run_phase2_tests();
    print_summary(2);
#endif

    /* Clean shutdown */
    platform_exit(0);

    return 0;
}
