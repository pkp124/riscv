/**
 * @file vec_add.c
 * @brief Vector addition workloads (integer and floating-point)
 *
 * Level 1: Integer vector add (c[i] = a[i] + b[i])
 * Level 2: Float32 vector add
 *
 * Demonstrates: vsetvli, vle32, vadd/vfadd, vse32
 * All implementations are VLEN-agnostic (work with any VLEN).
 */

#include "rvv/rvv_common.h"

/* =============================================================================
 * Level 1: Integer Vector Addition
 * ============================================================================= */

void rvv_vec_add_i32(const int32_t *a, const int32_t *b, int32_t *c, size_t n)
{
    size_t vl;
    __asm__ __volatile__(
        "1:\n\t"
        "vsetvli %[vl], %[n], e32, m1, ta, ma\n\t"
        "vle32.v v0, (%[a])\n\t"
        "vle32.v v1, (%[b])\n\t"
        "vadd.vv v2, v0, v1\n\t"
        "vse32.v v2, (%[c])\n\t"
        "slli    t0, %[vl], 2\n\t" /* vl * 4 bytes */
        "add     %[a], %[a], t0\n\t"
        "add     %[b], %[b], t0\n\t"
        "add     %[c], %[c], t0\n\t"
        "sub     %[n], %[n], %[vl]\n\t"
        "bnez    %[n], 1b\n\t"
        : [vl] "=&r"(vl), [a] "+r"(a), [b] "+r"(b), [c] "+r"(c), [n] "+r"(n)
        :
        : "t0", "v0", "v1", "v2", "memory");
}

void scalar_vec_add_i32(const int32_t *a, const int32_t *b, int32_t *c, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}

/* =============================================================================
 * Level 2: Float32 Vector Addition
 * ============================================================================= */

void rvv_vec_add_f32(const float *a, const float *b, float *c, size_t n)
{
    size_t vl;
    __asm__ __volatile__(
        "1:\n\t"
        "vsetvli %[vl], %[n], e32, m1, ta, ma\n\t"
        "vle32.v v0, (%[a])\n\t"
        "vle32.v v1, (%[b])\n\t"
        "vfadd.vv v2, v0, v1\n\t"
        "vse32.v v2, (%[c])\n\t"
        "slli    t0, %[vl], 2\n\t"
        "add     %[a], %[a], t0\n\t"
        "add     %[b], %[b], t0\n\t"
        "add     %[c], %[c], t0\n\t"
        "sub     %[n], %[n], %[vl]\n\t"
        "bnez    %[n], 1b\n\t"
        : [vl] "=&r"(vl), [a] "+r"(a), [b] "+r"(b), [c] "+r"(c), [n] "+r"(n)
        :
        : "t0", "v0", "v1", "v2", "memory");
}

void scalar_vec_add_f32(const float *a, const float *b, float *c, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}
