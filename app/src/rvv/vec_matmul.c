/**
 * @file vec_matmul.c
 * @brief Matrix multiplication using RVV: C = A * B
 *
 * Level 3: Matrix multiply (float32)
 * Demonstrates: vfmacc.vf with loop tiling, vectorized inner product
 *
 * Algorithm:
 *   For each row i of A:
 *     For each element k in row i of A:
 *       C[i][0..n-1] += A[i][k] * B[k][0..n-1]   (vectorized)
 *
 * This vectorizes across the columns of B and C, using vfmacc.vf
 * to accumulate A[i][k] * B[k][j] for all j simultaneously.
 */

#include "rvv/rvv_common.h"

void rvv_matmul_f32(const float *A, const float *B, float *C, uint32_t m, uint32_t n, uint32_t k)
{
    /* Zero out C first */
    for (uint32_t i = 0; i < m * n; i++) {
        C[i] = 0.0f;
    }

    /* For each row i of A */
    for (uint32_t i = 0; i < m; i++) {
        /* For each element in the k-dimension */
        for (uint32_t p = 0; p < k; p++) {
            float a_ik = A[i * k + p];

            /* Vectorize across columns of B[p][0..n-1] and C[i][0..n-1] */
            const float *b_row = &B[p * n];
            // cppcheck-suppress constVariablePointer
            float *c_row = &C[i * n];
            size_t remaining = n;
            size_t vl;

            __asm__ __volatile__("1:\n\t"
                                 "vsetvli  %[vl], %[remaining], e32, m1, ta, ma\n\t"
                                 "vle32.v  v0, (%[b_row])\n\t"   /* v0 = B[p][j..] */
                                 "vle32.v  v1, (%[c_row])\n\t"   /* v1 = C[i][j..] */
                                 "vfmacc.vf v1, %[a_ik], v0\n\t" /* v1 += a_ik * v0 */
                                 "vse32.v  v1, (%[c_row])\n\t"   /* C[i][j..] = v1 */
                                 "slli     t0, %[vl], 2\n\t"
                                 "add      %[b_row], %[b_row], t0\n\t"
                                 "add      %[c_row], %[c_row], t0\n\t"
                                 "sub      %[remaining], %[remaining], %[vl]\n\t"
                                 "bnez     %[remaining], 1b\n\t"
                                 : [vl] "=&r"(vl), [b_row] "+r"(b_row), [c_row] "+r"(c_row),
                                   [remaining] "+r"(remaining)
                                 : [a_ik] "f"(a_ik)
                                 : "t0", "v0", "v1", "memory");
        }
    }
}

void scalar_matmul_f32(const float *A, const float *B, float *C, uint32_t m, uint32_t n, uint32_t k)
{
    /* Zero out C */
    for (uint32_t i = 0; i < m * n; i++) {
        C[i] = 0.0f;
    }

    /* Standard triple-loop matrix multiply */
    for (uint32_t i = 0; i < m; i++) {
        for (uint32_t p = 0; p < k; p++) {
            float a_ik = A[i * k + p];
            for (uint32_t j = 0; j < n; j++) {
                C[i * n + j] += a_ik * B[p * n + j];
            }
        }
    }
}
