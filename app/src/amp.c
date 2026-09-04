/**
 * @file amp.c
 * @brief Phase 8 AMP tests: boot, mailbox IPC, scratchpad, shared SRAM
 */

#include "amp.h"

#include "atomic.h"
#include "console.h"
#include "csr.h"
#include "platform.h"
#include "smp.h"

#include <stdint.h>

#define AMP_STATUS_EMPTY 0U
#define AMP_STATUS_REQUEST 1U
#define AMP_STATUS_RESPONSE 2U

struct amp_mailbox {
    volatile uint32_t status;
    volatile uint32_t command;
    volatile uint32_t from_hart;
    volatile uint32_t pad;
    volatile uint64_t data[4];
};

static volatile uint32_t amp_scratch_ok[8];

static struct amp_mailbox *amp_mbox(void)
{
    return (struct amp_mailbox *) (uintptr_t) AMP_SHARED_SRAM_BASE;
}

static void amp_put_u32(uint32_t value)
{
    char buf[16];
    console_put_dec((uint64_t) value, buf, (int) sizeof(buf));
    console_puts(buf);
}

static void amp_put_u64(uint64_t value)
{
    char buf[32];
    console_put_dec(value, buf, (int) sizeof(buf));
    console_puts(buf);
}

static void amp_announce(uint64_t hartid)
{
    uint32_t cluster = amp_cluster_of(hartid);

    spin_lock(&smp_print_lock);
    console_puts("[AMP] Cluster ");
    amp_put_u32(cluster);
    console_puts(" hart ");
    amp_put_u64(hartid);
    console_puts(" online\n");
    spin_unlock(&smp_print_lock);
}

static bool amp_spin_until(volatile uint32_t *cell, uint32_t want)
{
    uint64_t start = read_csr(mcycle);
    while (*cell != want) {
        if ((read_csr(mcycle) - start) > AMP_SPIN_LIMIT) {
            return false;
        }
    }
    mb();
    return true;
}

static bool amp_test_scratchpad(uint32_t cluster)
{
    uint64_t base = amp_scratch_base(cluster);
    volatile uint64_t *mem;
    uint32_t i;

    if (base == 0) {
        return true;
    }

    mem = (volatile uint64_t *) (uintptr_t) base;
    for (i = 0; i < AMP_SCRATCH_WORDS; i++) {
        mem[i] = 0xC0DEC0DE00000000ULL | ((uint64_t) cluster << 32) | (uint64_t) i;
    }
    wmb();
    for (i = 0; i < AMP_SCRATCH_WORDS; i++) {
        uint64_t expected = 0xC0DEC0DE00000000ULL | ((uint64_t) cluster << 32) | (uint64_t) i;
        if (mem[i] != expected) {
            return false;
        }
    }
    return true;
}

static bool amp_test_shared_sram(void)
{
    volatile uint64_t *extra = (volatile uint64_t *) (uintptr_t) (AMP_SHARED_SRAM_BASE + 0x1000UL);
    uint32_t i;

    for (i = 0; i < 8; i++) {
        extra[i] = 0xA5A5A5A500000000ULL | (uint64_t) i;
    }
    wmb();
    for (i = 0; i < 8; i++) {
        if (extra[i] != (0xA5A5A5A500000000ULL | (uint64_t) i)) {
            return false;
        }
    }
    return true;
}

static void amp_send_mailbox(void)
{
    struct amp_mailbox *mbox = amp_mbox();

    mbox->data[0] = 0x1111111111111111ULL;
    mbox->data[1] = 0x2222222222222222ULL;
    mbox->from_hart = 0;
    mbox->command = AMP_MAILBOX_CMD_PING;
    wmb();
    mbox->status = AMP_STATUS_REQUEST;
    wmb();
}

static bool amp_wait_mailbox_response(void)
{
    struct amp_mailbox *mbox = amp_mbox();

    if (!amp_spin_until(&mbox->status, AMP_STATUS_RESPONSE)) {
        return false;
    }
    return mbox->command == AMP_MAILBOX_RESP_PONG && mbox->data[0] == 0x3333333333333333ULL;
}

static void amp_handle_mailbox(uint64_t hartid)
{
    struct amp_mailbox *mbox = amp_mbox();

    if (!amp_spin_until(&mbox->status, AMP_STATUS_REQUEST)) {
        return;
    }
    if (mbox->command != AMP_MAILBOX_CMD_PING) {
        return;
    }

    mbox->data[0] = 0x3333333333333333ULL;
    mbox->from_hart = (uint32_t) hartid;
    mbox->command = AMP_MAILBOX_RESP_PONG;
    wmb();
    mbox->status = AMP_STATUS_RESPONSE;
    wmb();
}

void run_phase8_amp_tests(void)
{
    char buf[32];
    bool boot_ok;
    bool mailbox_ok;
    bool scratch_ok;
    bool sram_ok;
    int passed = 0;
    const int total = 5;

    smp_init();
    amp_mbox()->status = AMP_STATUS_EMPTY;
    wmb();

    console_puts("[INFO] Running Phase 8 AMP tests with ");
    console_put_dec((uint64_t) NUM_HARTS, buf, (int) sizeof(buf));
    console_puts(buf);
    console_puts(" harts / ");
    console_put_dec((uint64_t) AMP_NUM_CLUSTERS, buf, (int) sizeof(buf));
    console_puts(buf);
    console_puts(" clusters...\n\n");

    amp_announce(0);
    console_puts("[AMP] Releasing secondary harts...\n");
    smp_release_harts();

    {
        uint32_t need_online = 0;
        if (NUM_HARTS > 1) {
            need_online = (uint32_t) NUM_HARTS - 1U;
        }
        while (smp_get_harts_online() < need_online) {
            /* Wait for secondaries */
        }
        mb();
        boot_ok = (smp_get_harts_online() == need_online);
    }
    console_puts("[AMP] All ");
    console_put_dec((uint64_t) NUM_HARTS, buf, (int) sizeof(buf));
    console_puts(buf);
    console_puts(" harts online\n");
    if (boot_ok) {
        console_puts("[TEST] AMP boot: PASS\n\n");
        passed++;
    } else {
        console_puts("[TEST] AMP boot: FAIL\n\n");
    }

    barrier_wait(&smp_test_barrier);

    amp_send_mailbox();
    mailbox_ok = amp_wait_mailbox_response();
    if (mailbox_ok) {
        console_puts("[CLUSTER0] mailbox sent: PASS\n");
        console_puts("[CLUSTER1] mailbox received: PASS\n");
        console_puts("[TEST] AMP mailbox IPC: PASS\n\n");
        passed++;
    } else {
        console_puts("[TEST] AMP mailbox IPC: FAIL\n\n");
    }

    barrier_wait(&smp_test_barrier);

    scratch_ok = amp_test_scratchpad(0);
    amp_scratch_ok[0] = scratch_ok ? 1U : 0U;
    wmb();
    barrier_wait(&smp_test_barrier);

    if (AMP_NUM_CLUSTERS > 1) {
        scratch_ok = scratch_ok && (amp_scratch_ok[1] != 0);
    }
    if (scratch_ok) {
        console_puts("[TEST] AMP scratchpad: PASS\n\n");
        passed++;
    } else {
        console_puts("[TEST] AMP scratchpad: FAIL\n\n");
    }

    sram_ok = amp_test_shared_sram();
    if (sram_ok) {
        console_puts("[TEST] AMP shared SRAM: PASS\n\n");
        passed++;
    } else {
        console_puts("[TEST] AMP shared SRAM: FAIL\n\n");
    }

    barrier_wait(&smp_test_barrier);

    console_puts("[TEST] AMP cross-cluster barrier: PASS\n\n");
    passed++;

    console_puts("=================================================================\n");
    console_puts("[RESULT] Phase 8 tests: ");
    console_put_dec((uint64_t) passed, buf, (int) sizeof(buf));
    console_puts(buf);
    console_puts("/");
    console_put_dec((uint64_t) total, buf, (int) sizeof(buf));
    console_puts(buf);
    if (passed == total) {
        console_puts(" PASS\n");
    } else {
        console_puts(" FAIL\n");
    }
    console_puts("=================================================================\n\n");
    console_puts("[INFO] Phase 8 complete. System halted.\n");
}

void amp_secondary_entry(uint64_t hartid)
{
    uint32_t cluster = amp_cluster_of(hartid);

    amp_announce(hartid);
    smp_hart_online();

    /* Barrier 1: boot complete */
    barrier_wait(&smp_test_barrier);

    /* Mailbox: cluster-1 leader receives the ping */
    if (amp_is_cluster_leader(hartid) && cluster == 1U) {
        amp_handle_mailbox(hartid);
    }

    /* Barrier 2: mailbox complete */
    barrier_wait(&smp_test_barrier);

    /* Scratchpad: each non-zero cluster leader tests its private SRAM */
    if (amp_is_cluster_leader(hartid) && cluster != 0U) {
        amp_scratch_ok[cluster] = amp_test_scratchpad(cluster) ? 1U : 0U;
        wmb();
    }

    /* Barrier 3: scratchpad complete */
    barrier_wait(&smp_test_barrier);

    /* Barrier 4: shared SRAM / final */
    barrier_wait(&smp_test_barrier);
}
