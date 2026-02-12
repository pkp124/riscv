/**
 * @file vec_memcpy.c
 * @brief Vectorized memory copy using RVV
 *
 * Level 1: Byte-granular memory copy using e8, m8 for maximum throughput.
 * Demonstrates: vle8, vse8, LMUL=8 for wide loads/stores.
 *
 * The LMUL=8 configuration groups 8 vector registers together,
 * allowing each iteration to copy 8*VLEN/8 = VLEN bytes.
 */

#include "rvv/rvv_common.h"

void rvv_memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *) dst;
    const uint8_t *s = (const uint8_t *) src;
    size_t vl;

    __asm__ __volatile__(
        "1:\n\t"
        "vsetvli %[vl], %[n], e8, m8, ta, ma\n\t"
        "vle8.v  v0, (%[s])\n\t"
        "vse8.v  v0, (%[d])\n\t"
        "add     %[s], %[s], %[vl]\n\t"
        "add     %[d], %[d], %[vl]\n\t"
        "sub     %[n], %[n], %[vl]\n\t"
        "bnez    %[n], 1b\n\t"
        : [vl] "=&r"(vl), [s] "+r"(s), [d] "+r"(d), [n] "+r"(n)
        :
        : "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "memory");
}

void scalar_memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *) dst;
    const uint8_t *s = (const uint8_t *) src;

    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}
