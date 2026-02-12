/**
 * @file smp.h
 * @brief Symmetric Multi-Processing (SMP) support for RISC-V
 *
 * Provides multi-hart boot protocol, synchronization primitives,
 * and inter-hart coordination for SMP configurations.
 *
 * Boot Protocol:
 *   1. All harts enter _start
 *   2. Hart 0 clears BSS, initializes platform, calls main()
 *   3. Secondary harts set up per-hart stacks, spin on smp_hart_release
 *   4. main() calls smp_release_harts() when ready
 *   5. Secondary harts call smp_secondary_entry(hartid)
 *
 * Synchronization:
 *   - Spinlocks using LR/SC (Load-Reserved / Store-Conditional)
 *   - Barriers using centralized counting with generation
 *   - Atomic operations via AMO instructions (see atomic.h)
 */

#ifndef SMP_H
#define SMP_H

#include "atomic.h"
#include "platform.h"

#include <stdbool.h>
#include <stdint.h>

/* =============================================================================
 * Configuration
 * ============================================================================= */

#ifndef NUM_HARTS
#define NUM_HARTS 1
#endif

#define MAX_HARTS 8

/* =============================================================================
 * Spinlock
 * ============================================================================= */

/**
 * @brief Spinlock type using LR/SC for mutual exclusion
 */
typedef struct {
    volatile uint32_t lock;
} spinlock_t;

/** Static initializer for spinlock */
#define SPINLOCK_INIT                                                                              \
    {                                                                                              \
        0                                                                                          \
    }

/**
 * @brief Acquire a spinlock (blocking)
 *
 * Uses LR/SC loop with acquire-release ordering.
 * Spins until the lock is acquired.
 *
 * @param lock Pointer to the spinlock
 */
static inline void spin_lock(spinlock_t *lock)
{
    uint32_t tmp;
    __asm__ __volatile__("1:\n\t"
                         "lr.w   %0, (%1)\n\t"
                         "bnez   %0, 1b\n\t"
                         "li     %0, 1\n\t"
                         "sc.w   %0, %0, (%1)\n\t"
                         "bnez   %0, 1b\n\t"
                         "fence  rw, rw\n\t"
                         : "=&r"(tmp)
                         : "r"(&lock->lock)
                         : "memory");
}

/**
 * @brief Release a spinlock
 *
 * Uses release fence before store.
 *
 * @param lock Pointer to the spinlock
 */
static inline void spin_unlock(spinlock_t *lock)
{
    __asm__ __volatile__("fence rw, rw" ::: "memory");
    lock->lock = 0;
}

/**
 * @brief Try to acquire a spinlock (non-blocking)
 *
 * @param lock Pointer to the spinlock
 * @return true if lock acquired, false if already held
 */
static inline bool spin_trylock(spinlock_t *lock)
{
    uint32_t tmp;
    uint32_t result;
    __asm__ __volatile__("lr.w   %0, (%2)\n\t"
                         "bnez   %0, 1f\n\t"
                         "li     %0, 1\n\t"
                         "sc.w   %1, %0, (%2)\n\t"
                         "bnez   %1, 1f\n\t"
                         "fence  rw, rw\n\t"
                         "li     %1, 1\n\t"
                         "j      2f\n\t"
                         "1:\n\t"
                         "li     %1, 0\n\t"
                         "2:\n\t"
                         : "=&r"(tmp), "=&r"(result)
                         : "r"(&lock->lock)
                         : "memory");
    return result != 0;
}

/* =============================================================================
 * Barrier
 * ============================================================================= */

/**
 * @brief Centralized barrier for synchronizing all harts
 *
 * Uses a generation counter to support reuse across multiple
 * barrier_wait() calls.
 */
typedef struct {
    volatile uint32_t count;
    volatile uint32_t generation;
    uint32_t total;
    spinlock_t lock;
} barrier_t;

/**
 * @brief Initialize a barrier
 *
 * @param bar Pointer to barrier
 * @param total Number of harts that must reach the barrier
 */
void barrier_init(barrier_t *bar, uint32_t total);

/**
 * @brief Wait at a barrier until all harts arrive
 *
 * The last hart to arrive resets the counter and advances the
 * generation, releasing all waiting harts.
 *
 * @param bar Pointer to barrier
 */
void barrier_wait(barrier_t *bar);

/* =============================================================================
 * SMP Boot Protocol
 * ============================================================================= */

/**
 * Release flag for secondary harts.
 * Set to 0 during BSS clear, set to non-zero by smp_release_harts().
 * Secondary harts spin on this in startup.S.
 */
extern volatile uint32_t smp_hart_release;

/**
 * @brief Initialize SMP subsystem
 *
 * Must be called by hart 0 before releasing secondary harts.
 * Initializes barriers and shared state.
 */
void smp_init(void);

/**
 * @brief Release secondary harts from their spin-wait
 *
 * Called by hart 0 after initialization is complete.
 */
void smp_release_harts(void);

/**
 * @brief Get the number of online secondary harts
 * @return Number of secondary harts that have completed boot
 */
uint32_t smp_get_harts_online(void);

/**
 * @brief Get the total configured number of harts
 * @return NUM_HARTS value
 */
uint32_t smp_get_num_harts(void);

/* =============================================================================
 * SMP Test Coordination
 * ============================================================================= */

/**
 * @brief Global print lock for serialized console output
 *
 * Must be held when printing from any hart to avoid interleaved output.
 */
extern spinlock_t smp_print_lock;

/**
 * @brief Global barrier for test phase synchronization
 *
 * Used to synchronize all harts between test phases.
 */
extern barrier_t smp_test_barrier;

/**
 * @brief Shared counter for spinlock test
 */
extern volatile uint32_t smp_lock_counter;

/**
 * @brief Shared lock for spinlock test
 */
extern spinlock_t smp_test_lock;

/**
 * @brief Shared counter for atomic test
 */
extern volatile uint32_t smp_atomic_counter;

/**
 * @brief Entry point for secondary harts (called from startup.S)
 *
 * Secondary harts call this after being released. It coordinates
 * with hart 0 to participate in SMP tests.
 *
 * @param hartid The hart ID of the calling hart
 */
void smp_secondary_entry(uint64_t hartid);

#endif /* SMP_H */
