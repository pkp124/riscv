/**
 * @file main.c
 * @brief Bare-metal application main entry point
 *
 * Phase 2 (NUM_HARTS == 1, no RVV): Single-core tests
 *   - Console output, CSR access, memory operations, function calls
 *
 * Phase 4 (NUM_HARTS > 1): Multi-core SMP tests
 *   - SMP boot verification (all harts online)
 *   - Spinlock correctness
 *   - Atomic operations
 *   - Barrier synchronization
 *
 * Phase 5 (NUM_HARTS == 1, ENABLE_RVV): RISC-V Vector Extension tests
 *   - RVV detection (misa V-bit, VLEN/VLENB)
 *   - Vector add (int32, float32)
 *   - Vector memcpy
 *   - Vector dot product
 *   - SAXPY (y = a*x + y)
 *   - Matrix multiply
 *   - Scalar vs vector performance comparison
 *
 * Designed to pass Phase 2, Phase 4, and Phase 5 CTest test cases.
 */

#include "console.h"
#include "csr.h"
#include "platform.h"

#include <stdbool.h>
#include <stdint.h>

#if NUM_HARTS > 1
#include "smp.h"
#endif

#if defined(ENABLE_RVV) && NUM_HARTS <= 1
#include "rvv/rvv_common.h"
#include "rvv/rvv_detect.h"
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

#if NUM_HARTS <= 1 && !defined(ENABLE_RVV)
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

#if NUM_HARTS <= 1 && !defined(ENABLE_RVV)

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

#endif /* NUM_HARTS <= 1 && !ENABLE_RVV */

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
 * Phase 5: RVV Tests
 * ============================================================================= */

#if defined(ENABLE_RVV) && NUM_HARTS <= 1

/**
 * @brief Test 1: RVV Detection - check misa V-bit, print VLEN/VLENB
 */
static void test_rvv_detect(void)
{
    bool available = rvv_available();
    record_test("RVV detection", available);

    if (available) {
        rvv_print_info();
    }
}

/**
 * @brief Test 2: Integer vector add correctness
 */
static void test_rvv_vec_add_i32(void)
{
    static int32_t a[RVV_TEST_SIZE];
    static int32_t b[RVV_TEST_SIZE];
    static int32_t c_scalar[RVV_TEST_SIZE];
    static int32_t c_vector[RVV_TEST_SIZE];
    char buf[32];

    /* Initialize test data */
    for (uint32_t i = 0; i < RVV_TEST_SIZE; i++) {
        a[i] = (int32_t) (i + 1);
        b[i] = (int32_t) (i * 2);
    }

    /* Scalar reference */
    uint64_t start = rvv_read_mcycle();
    scalar_vec_add_i32(a, b, c_scalar, RVV_TEST_SIZE);
    uint64_t scalar_cycles = rvv_read_mcycle() - start;

    /* RVV implementation */
    start = rvv_read_mcycle();
    rvv_vec_add_i32(a, b, c_vector, RVV_TEST_SIZE);
    uint64_t vector_cycles = rvv_read_mcycle() - start;

    /* Verify correctness */
    bool passed = true;
    for (uint32_t i = 0; i < RVV_TEST_SIZE; i++) {
        if (c_scalar[i] != c_vector[i]) {
            passed = false;
            break;
        }
    }

    record_test("Vec add (int32)", passed);

    /* Print cycle comparison */
    console_puts("[RVV] vec_add_i32: scalar=");
    int_to_str(scalar_cycles, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" vec=");
    int_to_str(vector_cycles, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" cycles\n");
}

/**
 * @brief Test 3: Vector memcpy correctness
 */
static void test_rvv_memcpy(void)
{
    static uint8_t src[RVV_TEST_SIZE * 4];
    static uint8_t dst_scalar[RVV_TEST_SIZE * 4];
    static uint8_t dst_vector[RVV_TEST_SIZE * 4];
    char buf[32];
    size_t nbytes = RVV_TEST_SIZE * 4;

    /* Initialize source data */
    for (uint32_t i = 0; i < nbytes; i++) {
        src[i] = (uint8_t) (i & 0xFF);
        dst_scalar[i] = 0;
        dst_vector[i] = 0;
    }

    /* Scalar reference */
    uint64_t start = rvv_read_mcycle();
    scalar_memcpy(dst_scalar, src, nbytes);
    uint64_t scalar_cycles = rvv_read_mcycle() - start;

    /* RVV implementation */
    start = rvv_read_mcycle();
    rvv_memcpy(dst_vector, src, nbytes);
    uint64_t vector_cycles = rvv_read_mcycle() - start;

    /* Verify correctness */
    bool passed = true;
    for (uint32_t i = 0; i < nbytes; i++) {
        if (dst_scalar[i] != dst_vector[i]) {
            passed = false;
            break;
        }
    }

    record_test("Vec memcpy", passed);

    console_puts("[RVV] vec_memcpy: scalar=");
    int_to_str(scalar_cycles, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" vec=");
    int_to_str(vector_cycles, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" cycles\n");
}

/**
 * @brief Test 4: Float32 vector add correctness
 */
static void test_rvv_vec_add_f32(void)
{
    static float a[RVV_TEST_SIZE];
    static float b[RVV_TEST_SIZE];
    static float c_scalar[RVV_TEST_SIZE];
    static float c_vector[RVV_TEST_SIZE];
    char buf[32];

    /* Initialize test data */
    for (uint32_t i = 0; i < RVV_TEST_SIZE; i++) {
        a[i] = (float) (i + 1) * 1.0f;
        b[i] = (float) (i) * 0.5f;
    }

    /* Scalar reference */
    uint64_t start = rvv_read_mcycle();
    scalar_vec_add_f32(a, b, c_scalar, RVV_TEST_SIZE);
    uint64_t scalar_cycles = rvv_read_mcycle() - start;

    /* RVV implementation */
    start = rvv_read_mcycle();
    rvv_vec_add_f32(a, b, c_vector, RVV_TEST_SIZE);
    uint64_t vector_cycles = rvv_read_mcycle() - start;

    /* Verify correctness (with floating-point tolerance) */
    bool passed = true;
    for (uint32_t i = 0; i < RVV_TEST_SIZE; i++) {
        if (!rvv_float_eq(c_scalar[i], c_vector[i], 0.001f)) {
            passed = false;
            break;
        }
    }

    record_test("Vec add (float32)", passed);

    console_puts("[RVV] vec_add_f32: scalar=");
    int_to_str(scalar_cycles, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" vec=");
    int_to_str(vector_cycles, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" cycles\n");
}

/**
 * @brief Test 5: Dot product correctness
 */
static void test_rvv_dot_product(void)
{
    static float a[RVV_TEST_SIZE];
    static float b[RVV_TEST_SIZE];
    char buf[32];

    /* Initialize test data: a[i] = i+1, b[i] = 1.0 */
    for (uint32_t i = 0; i < RVV_TEST_SIZE; i++) {
        a[i] = (float) (i + 1);
        b[i] = 1.0f;
    }

    /* Scalar reference: sum(1..64) = 64*65/2 = 2080 */
    uint64_t start = rvv_read_mcycle();
    float scalar_result = scalar_dot_product_f32(a, b, RVV_TEST_SIZE);
    uint64_t scalar_cycles = rvv_read_mcycle() - start;

    /* RVV implementation */
    start = rvv_read_mcycle();
    float vector_result = rvv_dot_product_f32(a, b, RVV_TEST_SIZE);
    uint64_t vector_cycles = rvv_read_mcycle() - start;

    /* Verify correctness */
    bool passed = rvv_float_eq(scalar_result, vector_result, 0.01f);

    record_test("Dot product (float32)", passed);

    console_puts("[RVV] dot_product: scalar=");
    int_to_str(scalar_cycles, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" vec=");
    int_to_str(vector_cycles, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" cycles\n");
}

/**
 * @brief Test 6: SAXPY correctness
 */
static void test_rvv_saxpy(void)
{
    static float x[RVV_TEST_SIZE];
    static float y_scalar[RVV_TEST_SIZE];
    static float y_vector[RVV_TEST_SIZE];
    char buf[32];
    float a = 2.0f;

    /* Initialize test data */
    for (uint32_t i = 0; i < RVV_TEST_SIZE; i++) {
        x[i] = (float) (i + 1);
        y_scalar[i] = (float) i * 0.5f;
        y_vector[i] = (float) i * 0.5f;
    }

    /* Scalar reference */
    uint64_t start = rvv_read_mcycle();
    scalar_saxpy(a, x, y_scalar, RVV_TEST_SIZE);
    uint64_t scalar_cycles = rvv_read_mcycle() - start;

    /* RVV implementation */
    start = rvv_read_mcycle();
    rvv_saxpy(a, x, y_vector, RVV_TEST_SIZE);
    uint64_t vector_cycles = rvv_read_mcycle() - start;

    /* Verify correctness */
    bool passed = true;
    for (uint32_t i = 0; i < RVV_TEST_SIZE; i++) {
        if (!rvv_float_eq(y_scalar[i], y_vector[i], 0.01f)) {
            passed = false;
            break;
        }
    }

    record_test("SAXPY (float32)", passed);

    console_puts("[RVV] saxpy: scalar=");
    int_to_str(scalar_cycles, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" vec=");
    int_to_str(vector_cycles, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" cycles\n");
}

/**
 * @brief Test 7: Matrix multiply correctness
 */
static void test_rvv_matmul(void)
{
    static float A[RVV_MATRIX_DIM * RVV_MATRIX_DIM];
    static float B[RVV_MATRIX_DIM * RVV_MATRIX_DIM];
    static float C_scalar[RVV_MATRIX_DIM * RVV_MATRIX_DIM];
    static float C_vector[RVV_MATRIX_DIM * RVV_MATRIX_DIM];
    char buf[32];
    uint32_t dim = RVV_MATRIX_DIM;

    /* Initialize test matrices */
    for (uint32_t i = 0; i < dim * dim; i++) {
        A[i] = (float) ((i % dim) + 1);
        B[i] = (float) ((i / dim) + 1);
    }

    /* Scalar reference */
    uint64_t start = rvv_read_mcycle();
    scalar_matmul_f32(A, B, C_scalar, dim, dim, dim);
    uint64_t scalar_cycles = rvv_read_mcycle() - start;

    /* RVV implementation */
    start = rvv_read_mcycle();
    rvv_matmul_f32(A, B, C_vector, dim, dim, dim);
    uint64_t vector_cycles = rvv_read_mcycle() - start;

    /* Verify correctness */
    bool passed = true;
    for (uint32_t i = 0; i < dim * dim; i++) {
        if (!rvv_float_eq(C_scalar[i], C_vector[i], 0.1f)) {
            passed = false;
            break;
        }
    }

    record_test("Matrix multiply (float32)", passed);

    console_puts("[RVV] matmul: scalar=");
    int_to_str(scalar_cycles, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" vec=");
    int_to_str(vector_cycles, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" cycles\n");
}

static void run_phase5_tests(void)
{
    console_puts("[INFO] Running Phase 5 RVV tests...\n");
    console_puts("\n");

    /* Test 1: RVV Detection */
    test_rvv_detect();
    console_puts("\n");

    /* Test 2: Integer vector add */
    test_rvv_vec_add_i32();
    console_puts("\n");

    /* Test 3: Vector memcpy */
    test_rvv_memcpy();
    console_puts("\n");

    /* Test 4: Float32 vector add */
    test_rvv_vec_add_f32();
    console_puts("\n");

    /* Test 5: Dot product */
    test_rvv_dot_product();
    console_puts("\n");

    /* Test 6: SAXPY */
    test_rvv_saxpy();
    console_puts("\n");

    /* Test 7: Matrix multiply */
    test_rvv_matmul();
    console_puts("\n");
}

#endif /* ENABLE_RVV && NUM_HARTS <= 1 */

/* =============================================================================
 * Banner
 * ============================================================================= */

static void print_banner(void)
{
    console_puts("\n");
    console_puts("=================================================================\n");
    console_puts("RISC-V Bare-Metal System Explorer\n");
    console_puts("=================================================================\n");
    console_puts("Platform: ");
    console_puts(platform_get_name());
    console_puts("\n");

#if NUM_HARTS > 1
    {
        char buf[32];
        console_puts("Phase: 4 - Multi-Core SMP (");
        int_to_str(NUM_HARTS, buf, sizeof(buf));
        console_puts(buf);
        console_puts(" harts)\n");
    }
#elif defined(ENABLE_RVV)
    console_puts("Phase: 5 - RISC-V Vector Extension (RVV)\n");
#else
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
#elif defined(ENABLE_RVV)
    run_phase5_tests();
    print_summary(5);
#else
    run_phase2_tests();
    print_summary(2);
#endif

    /* Clean shutdown */
    platform_exit(0);

    return 0;
}
