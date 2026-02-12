/**
 * @file atomic.h
 * @brief RISC-V atomic memory operations
 *
 * Provides atomic operations using RISC-V 'A' extension instructions:
 * - AMO (Atomic Memory Operations): amoadd, amoswap, amoor, amoand
 * - LR/SC (Load-Reserved / Store-Conditional) for CAS operations
 *
 * All operations use .aqrl (acquire-release) ordering for sequential
 * consistency, which is safest for SMP correctness.
 */

#ifndef ATOMIC_H
#define ATOMIC_H

#include <stdint.h>

/* =============================================================================
 * 32-bit Atomic Operations (using AMO instructions)
 * ============================================================================= */

/**
 * @brief Atomic add (32-bit)
 * @param ptr Pointer to the atomic variable
 * @param val Value to add
 * @return Previous value at *ptr (before addition)
 */
static inline uint32_t atomic_add_u32(volatile uint32_t *ptr, uint32_t val)
{
    uint32_t result;
    __asm__ __volatile__("amoadd.w.aqrl %0, %1, (%2)"
                         : "=r"(result)
                         : "r"(val), "r"(ptr)
                         : "memory");
    return result;
}

/**
 * @brief Atomic swap (32-bit)
 * @param ptr Pointer to the atomic variable
 * @param val Value to swap in
 * @return Previous value at *ptr
 */
static inline uint32_t atomic_swap_u32(volatile uint32_t *ptr, uint32_t val)
{
    uint32_t result;
    __asm__ __volatile__("amoswap.w.aqrl %0, %1, (%2)"
                         : "=r"(result)
                         : "r"(val), "r"(ptr)
                         : "memory");
    return result;
}

/**
 * @brief Atomic OR (32-bit)
 * @param ptr Pointer to the atomic variable
 * @param val Value to OR
 * @return Previous value at *ptr
 */
static inline uint32_t atomic_or_u32(volatile uint32_t *ptr, uint32_t val)
{
    uint32_t result;
    __asm__ __volatile__("amoor.w.aqrl %0, %1, (%2)"
                         : "=r"(result)
                         : "r"(val), "r"(ptr)
                         : "memory");
    return result;
}

/**
 * @brief Atomic AND (32-bit)
 * @param ptr Pointer to the atomic variable
 * @param val Value to AND
 * @return Previous value at *ptr
 */
static inline uint32_t atomic_and_u32(volatile uint32_t *ptr, uint32_t val)
{
    uint32_t result;
    __asm__ __volatile__("amoand.w.aqrl %0, %1, (%2)"
                         : "=r"(result)
                         : "r"(val), "r"(ptr)
                         : "memory");
    return result;
}

/**
 * @brief Atomic load (32-bit) with acquire semantics
 * @param ptr Pointer to the atomic variable
 * @return Current value at *ptr
 */
static inline uint32_t atomic_load_u32(volatile uint32_t *ptr)
{
    uint32_t val;
    __asm__ __volatile__("lw %0, 0(%1)\n\t"
                         "fence r, rw"
                         : "=r"(val)
                         : "r"(ptr)
                         : "memory");
    return val;
}

/**
 * @brief Atomic store (32-bit) with release semantics
 * @param ptr Pointer to the atomic variable
 * @param val Value to store
 */
static inline void atomic_store_u32(volatile uint32_t *ptr, uint32_t val)
{
    __asm__ __volatile__("fence rw, w\n\t"
                         "sw %0, 0(%1)"
                         :
                         : "r"(val), "r"(ptr)
                         : "memory");
}

/**
 * @brief Compare-and-swap (32-bit) using LR/SC
 * @param ptr Pointer to the atomic variable
 * @param expected Expected current value
 * @param desired Value to store if current == expected
 * @return 1 if swap succeeded, 0 if it failed
 */
static inline int atomic_cas_u32(volatile uint32_t *ptr, uint32_t expected, uint32_t desired)
{
    uint32_t tmp;
    int result;
    __asm__ __volatile__("1:\n\t"
                         "lr.w.aqrl %0, (%2)\n\t"
                         "bne       %0, %3, 2f\n\t"
                         "sc.w.rl   %1, %4, (%2)\n\t"
                         "bnez      %1, 1b\n\t"
                         "li        %1, 1\n\t"
                         "j         3f\n\t"
                         "2:\n\t"
                         "li        %1, 0\n\t"
                         "3:\n\t"
                         : "=&r"(tmp), "=&r"(result)
                         : "r"(ptr), "r"(expected), "r"(desired)
                         : "memory");
    return result;
}

/* =============================================================================
 * 64-bit Atomic Operations
 * ============================================================================= */

/**
 * @brief Atomic add (64-bit)
 * @param ptr Pointer to the atomic variable
 * @param val Value to add
 * @return Previous value at *ptr
 */
static inline uint64_t atomic_add_u64(volatile uint64_t *ptr, uint64_t val)
{
    uint64_t result;
    __asm__ __volatile__("amoadd.d.aqrl %0, %1, (%2)"
                         : "=r"(result)
                         : "r"(val), "r"(ptr)
                         : "memory");
    return result;
}

/**
 * @brief Atomic swap (64-bit)
 * @param ptr Pointer to the atomic variable
 * @param val Value to swap in
 * @return Previous value at *ptr
 */
static inline uint64_t atomic_swap_u64(volatile uint64_t *ptr, uint64_t val)
{
    uint64_t result;
    __asm__ __volatile__("amoswap.d.aqrl %0, %1, (%2)"
                         : "=r"(result)
                         : "r"(val), "r"(ptr)
                         : "memory");
    return result;
}

#endif /* ATOMIC_H */
