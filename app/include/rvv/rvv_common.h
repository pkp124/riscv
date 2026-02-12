/**
 * @file rvv_common.h
 * @brief Common RVV macros and types for vector workloads
 *
 * Provides shared types, constants, and utility macros used
 * across all RVV workload implementations.
 */

#ifndef RVV_COMMON_H
#define RVV_COMMON_H

#include <stddef.h>
#include <stdint.h>

/* =============================================================================
 * Test Data Sizes
 * ============================================================================= */

/** Default array size for vector workloads */
#define RVV_TEST_SIZE 64

/** Small array size for quick tests */
#define RVV_TEST_SIZE_SMALL 16

/** Matrix dimensions for matmul test */
#define RVV_MATRIX_DIM 8

/* =============================================================================
 * Benchmark Result Type
 * ============================================================================= */

/**
 * @brief Result structure for RVV benchmark comparison
 */
typedef struct {
    const char *name;        /**< Workload name */
    uint64_t scalar_cycles;  /**< Cycles for scalar implementation */
    uint64_t vector_cycles;  /**< Cycles for vector implementation */
    int passed;              /**< Correctness check result (1=pass, 0=fail) */
} rvv_bench_result_t;

/* =============================================================================
 * Cycle Counter Helpers
 * ============================================================================= */

/**
 * @brief Read the machine cycle counter
 * @return Current cycle count
 */
static inline uint64_t rvv_read_mcycle(void)
{
    uint64_t cycles;
    __asm__ __volatile__("csrr %0, mcycle" : "=r"(cycles));
    return cycles;
}

/* =============================================================================
 * Float Comparison Helper
 * ============================================================================= */

/**
 * @brief Compare two float values with tolerance
 * @param a First value
 * @param b Second value
 * @param epsilon Maximum allowed difference
 * @return 1 if |a - b| <= epsilon, 0 otherwise
 */
static inline int rvv_float_eq(float a, float b, float epsilon)
{
    float diff = a - b;
    if (diff < 0)
        diff = -diff;
    return diff <= epsilon;
}

/* =============================================================================
 * RVV Workload Function Declarations
 * ============================================================================= */

/* Level 1: Basic Vector Operations */

/**
 * @brief Vector addition (int32): c[i] = a[i] + b[i]
 * Uses RVV inline assembly.
 */
void rvv_vec_add_i32(const int32_t *a, const int32_t *b, int32_t *c, size_t n);

/**
 * @brief Scalar reference: vector addition (int32)
 */
void scalar_vec_add_i32(const int32_t *a, const int32_t *b, int32_t *c, size_t n);

/**
 * @brief Vectorized memory copy using RVV
 */
void rvv_memcpy(void *dst, const void *src, size_t n);

/**
 * @brief Scalar reference: memory copy
 */
void scalar_memcpy(void *dst, const void *src, size_t n);

/* Level 2: Floating-Point Vector Operations */

/**
 * @brief Vector addition (float32): c[i] = a[i] + b[i]
 */
void rvv_vec_add_f32(const float *a, const float *b, float *c, size_t n);

/**
 * @brief Scalar reference: vector addition (float32)
 */
void scalar_vec_add_f32(const float *a, const float *b, float *c, size_t n);

/**
 * @brief Vector dot product (float32): result = sum(a[i] * b[i])
 */
float rvv_dot_product_f32(const float *a, const float *b, size_t n);

/**
 * @brief Scalar reference: dot product (float32)
 */
float scalar_dot_product_f32(const float *a, const float *b, size_t n);

/**
 * @brief SAXPY: y[i] = a * x[i] + y[i]
 */
void rvv_saxpy(float a, const float *x, float *y, size_t n);

/**
 * @brief Scalar reference: SAXPY
 */
void scalar_saxpy(float a, const float *x, float *y, size_t n);

/* Level 3: Advanced Operations */

/**
 * @brief Matrix multiply (float32): C = A * B
 * @param A Input matrix A [m x k]
 * @param B Input matrix B [k x n]
 * @param C Output matrix C [m x n]
 * @param m Rows of A and C
 * @param n Columns of B and C
 * @param k Columns of A / rows of B
 */
void rvv_matmul_f32(const float *A, const float *B, float *C,
                    uint32_t m, uint32_t n, uint32_t k);

/**
 * @brief Scalar reference: matrix multiply
 */
void scalar_matmul_f32(const float *A, const float *B, float *C,
                       uint32_t m, uint32_t n, uint32_t k);

#endif /* RVV_COMMON_H */
