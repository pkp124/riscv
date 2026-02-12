/**
 * @file smp.c
 * @brief Symmetric Multi-Processing (SMP) implementation
 *
 * Implements multi-hart boot protocol, spinlocks, barriers,
 * and test coordination for SMP configurations.
 *
 * Secondary harts spin in startup.S until smp_hart_release is set,
 * then call smp_secondary_entry() to participate in SMP tests.
 */

#include "smp.h"

#include "atomic.h"
#include "console.h"
#include "csr.h"
#include "platform.h"

#include <stdint.h>

/* =============================================================================
 * SMP Global State
 * ============================================================================= */

/**
 * Release flag for secondary harts.
 * Initialized to 0 via BSS clearing by hart 0.
 * Set to 1 by smp_release_harts().
 * Referenced from startup.S.
 */
volatile uint32_t smp_hart_release;

/**
 * Count of secondary harts that have completed boot.
 */
static volatile uint32_t smp_harts_online;

/**
 * Global print lock - serializes console output across harts.
 */
spinlock_t smp_print_lock;

/**
 * Global test barrier - synchronizes harts between test phases.
 */
barrier_t smp_test_barrier;

/**
 * Shared counter for spinlock correctness test.
 */
volatile uint32_t smp_lock_counter;

/**
 * Shared lock for spinlock correctness test.
 */
spinlock_t smp_test_lock;

/**
 * Shared counter for atomic operation test.
 */
volatile uint32_t smp_atomic_counter;

/* =============================================================================
 * Barrier Implementation
 * ============================================================================= */

void barrier_init(barrier_t *bar, uint32_t total)
{
    bar->count = 0;
    bar->generation = 0;
    bar->total = total;
    bar->lock = (spinlock_t) SPINLOCK_INIT;
}

void barrier_wait(barrier_t *bar)
{
    spin_lock(&bar->lock);

    bar->count++;
    uint32_t gen = bar->generation;

    if (bar->count >= bar->total) {
        /* Last hart to arrive: reset and advance generation */
        bar->count = 0;
        mb(); /* Ensure count reset is visible before generation bump */
        bar->generation++;
        spin_unlock(&bar->lock);
    } else {
        spin_unlock(&bar->lock);
        /* Wait for generation to advance */
        while (*(volatile uint32_t *) &bar->generation == gen) {
            /* Spin */
        }
    }

    /* Ensure all memory operations from before the barrier are visible */
    mb();
}

/* =============================================================================
 * SMP Boot Protocol
 * ============================================================================= */

void smp_init(void)
{
    /* Initialize shared state */
    smp_harts_online = 0;
    smp_lock_counter = 0;
    smp_atomic_counter = 0;
    smp_print_lock = (spinlock_t) SPINLOCK_INIT;
    smp_test_lock = (spinlock_t) SPINLOCK_INIT;

    /* Initialize barrier for all harts */
    barrier_init(&smp_test_barrier, NUM_HARTS);

    wmb(); /* Ensure all initializations are visible */
}

void smp_release_harts(void)
{
    /* Ensure all initialization is visible to secondary harts */
    wmb();

    /* Set release flag - secondary harts will see this and proceed */
    smp_hart_release = 1;

    /* Ensure the store is visible */
    wmb();
}

uint32_t smp_get_harts_online(void)
{
    return atomic_load_u32(&smp_harts_online);
}

uint32_t smp_get_num_harts(void)
{
    return NUM_HARTS;
}

/* =============================================================================
 * Secondary Hart Entry Point
 * ============================================================================= */

/**
 * @brief Print hart ID to console (helper)
 */
static void print_hart_id(uint64_t hartid)
{
    char buf[4];
    if (hartid < 10) {
        buf[0] = '0' + (char) hartid;
        buf[1] = '\0';
    } else {
        buf[0] = '0' + (char) (hartid / 10);
        buf[1] = '0' + (char) (hartid % 10);
        buf[2] = '\0';
    }
    console_puts(buf);
}

/**
 * @brief Entry point for secondary harts
 *
 * Called from startup.S after secondary harts are released.
 * Participates in SMP tests coordinated by hart 0 via barriers.
 *
 * Test coordination protocol (synchronized with main.c):
 *   Barrier 1: Boot complete (all harts online)
 *   Barrier 2: Spinlock test start
 *   Barrier 3: Spinlock test end
 *   Barrier 4: Atomic test start
 *   Barrier 5: Atomic test end
 *   Barrier 6: Final barrier (barrier test)
 */
void smp_secondary_entry(uint64_t hartid)
{
    /* Announce this hart is online (with print lock for clean output) */
    spin_lock(&smp_print_lock);
    console_puts("[SMP] Hart ");
    print_hart_id(hartid);
    console_puts(" online\n");
    spin_unlock(&smp_print_lock);

    /* Increment online counter atomically */
    atomic_add_u32(&smp_harts_online, 1);

    /* === Barrier 1: Boot complete === */
    barrier_wait(&smp_test_barrier);

    /* === Barrier 2: Spinlock test start === */
    barrier_wait(&smp_test_barrier);

    /* Spinlock test: increment shared counter under lock */
    spin_lock(&smp_test_lock);
    smp_lock_counter++;
    spin_unlock(&smp_test_lock);

    /* === Barrier 3: Spinlock test end === */
    barrier_wait(&smp_test_barrier);

    /* === Barrier 4: Atomic test start === */
    barrier_wait(&smp_test_barrier);

    /* Atomic test: increment shared counter atomically */
    atomic_add_u32(&smp_atomic_counter, 1);

    /* === Barrier 5: Atomic test end === */
    barrier_wait(&smp_test_barrier);

    /* === Barrier 6: Final barrier (barrier test) === */
    barrier_wait(&smp_test_barrier);

    /* All tests complete - enter low-power wait loop */
    while (1) {
        __asm__ __volatile__("wfi");
    }
}
