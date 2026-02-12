/**
 * @file vec_dotprod.c
 * @brief Vector dot product using RVV
 *
 * Level 2: Float32 dot product: result = sum(a[i] * b[i])
 * Demonstrates: vfmul.vv, vfredosum (ordered FP reduction)
 *
 * The reduction accumulates partial sums across the vector register
 * into a scalar result using the ordered floating-point reduction
 * instruction (vfredosum).
 */

#include "rvv/rvv_common.h"

float rvv_dot_product_f32(const float *a, const float *b, size_t n)
{
    float result = 0.0f;
    size_t vl;

    /*
     * Strategy:
     * 1. Initialize accumulator vector to 0
     * 2. For each chunk: load a, load b, multiply, accumulate via reduction
     * 3. Extract final scalar sum
     *
     * We use a loop that multiplies chunks and reduces them into a
     * running scalar sum held in v4[0].
     */
    __asm__ __volatile__(
        /* Initialize v4 to hold the running sum (scalar 0.0) */
        "fmv.w.x    ft0, zero\n\t"
        "vsetvli    zero, %[n], e32, m1, ta, ma\n\t"
        "vfmv.s.f   v4, ft0\n\t" /* v4[0] = 0.0 */
        "1:\n\t"
        "vsetvli    %[vl], %[n], e32, m1, ta, ma\n\t"
        "vle32.v    v0, (%[a])\n\t"
        "vle32.v    v1, (%[b])\n\t"
        "vfmul.vv   v2, v0, v1\n\t"
        "vfredosum.vs v4, v2, v4\n\t" /* v4[0] += sum(v2) */
        "slli       t0, %[vl], 2\n\t"
        "add        %[a], %[a], t0\n\t"
        "add        %[b], %[b], t0\n\t"
        "sub        %[n], %[n], %[vl]\n\t"
        "bnez       %[n], 1b\n\t"
        /* Extract result from v4[0] */
        "vfmv.f.s   %[result], v4\n\t"
        : [vl] "=&r"(vl), [a] "+r"(a), [b] "+r"(b), [n] "+r"(n),
          [result] "=f"(result)
        :
        : "t0", "ft0", "v0", "v1", "v2", "v4", "memory");

    return result;
}

float scalar_dot_product_f32(const float *a, const float *b, size_t n)
{
    float sum = 0.0f;
    for (size_t i = 0; i < n; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}
