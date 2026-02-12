/**
 * @file vec_saxpy.c
 * @brief SAXPY operation using RVV: y[i] = a * x[i] + y[i]
 *
 * Level 2/3: Classic BLAS Level-1 operation.
 * Demonstrates: vfmacc.vf (fused multiply-accumulate, vector-scalar)
 *
 * vfmacc.vf vd, rs1, vs2 computes: vd[i] = rs1 * vs2[i] + vd[i]
 * This maps directly to the SAXPY pattern.
 */

#include "rvv/rvv_common.h"

void rvv_saxpy(float a, const float *x, float *y, size_t n)
{
    size_t vl;

    __asm__ __volatile__(
        "1:\n\t"
        "vsetvli  %[vl], %[n], e32, m1, ta, ma\n\t"
        "vle32.v  v0, (%[x])\n\t"  /* v0 = x[...] */
        "vle32.v  v1, (%[y])\n\t"  /* v1 = y[...] */
        "vfmacc.vf v1, %[a], v0\n\t" /* v1 = a * v0 + v1 */
        "vse32.v  v1, (%[y])\n\t"  /* y[...] = v1 */
        "slli     t0, %[vl], 2\n\t"
        "add      %[x], %[x], t0\n\t"
        "add      %[y], %[y], t0\n\t"
        "sub      %[n], %[n], %[vl]\n\t"
        "bnez     %[n], 1b\n\t"
        : [vl] "=&r"(vl), [x] "+r"(x), [y] "+r"(y), [n] "+r"(n)
        : [a] "f"(a)
        : "t0", "v0", "v1", "memory");
}

void scalar_saxpy(float a, const float *x, float *y, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        y[i] = a * x[i] + y[i];
    }
}
