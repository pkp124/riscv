/**
 * @file amp.h
 * @brief Asymmetric multi-processing helpers and Phase 8 tests
 *
 * Clusters are contiguous hart ranges described by AMP_CLUSTER*_ macros
 * generated from the YAML platform description at CMake configure time.
 */

#ifndef AMP_H
#define AMP_H

#include "platform.h"

#include <stdbool.h>
#include <stdint.h>

#ifndef AMP_NUM_CLUSTERS
#define AMP_NUM_CLUSTERS 2
#endif

#ifndef AMP_CLUSTER0_HARTS
#define AMP_CLUSTER0_HARTS 1
#endif
#ifndef AMP_CLUSTER1_HARTS
#define AMP_CLUSTER1_HARTS 1
#endif
#ifndef AMP_CLUSTER2_HARTS
#define AMP_CLUSTER2_HARTS 0
#endif
#ifndef AMP_CLUSTER3_HARTS
#define AMP_CLUSTER3_HARTS 0
#endif
#ifndef AMP_CLUSTER4_HARTS
#define AMP_CLUSTER4_HARTS 0
#endif
#ifndef AMP_CLUSTER5_HARTS
#define AMP_CLUSTER5_HARTS 0
#endif
#ifndef AMP_CLUSTER6_HARTS
#define AMP_CLUSTER6_HARTS 0
#endif
#ifndef AMP_CLUSTER7_HARTS
#define AMP_CLUSTER7_HARTS 0
#endif

#ifndef AMP_CLUSTER0_HARTID_BASE
#define AMP_CLUSTER0_HARTID_BASE 0
#endif
#ifndef AMP_CLUSTER1_HARTID_BASE
#define AMP_CLUSTER1_HARTID_BASE AMP_CLUSTER0_HARTS
#endif
#ifndef AMP_CLUSTER2_HARTID_BASE
#define AMP_CLUSTER2_HARTID_BASE 0
#endif
#ifndef AMP_CLUSTER3_HARTID_BASE
#define AMP_CLUSTER3_HARTID_BASE 0
#endif
#ifndef AMP_CLUSTER4_HARTID_BASE
#define AMP_CLUSTER4_HARTID_BASE 0
#endif
#ifndef AMP_CLUSTER5_HARTID_BASE
#define AMP_CLUSTER5_HARTID_BASE 0
#endif
#ifndef AMP_CLUSTER6_HARTID_BASE
#define AMP_CLUSTER6_HARTID_BASE 0
#endif
#ifndef AMP_CLUSTER7_HARTID_BASE
#define AMP_CLUSTER7_HARTID_BASE 0
#endif

#ifndef AMP_SHARED_SRAM_BASE
#define AMP_SHARED_SRAM_BASE 0x30000000UL
#endif

#ifndef AMP_CLUSTER0_SCRATCH_BASE
#define AMP_CLUSTER0_SCRATCH_BASE 0x20000000UL
#endif
#ifndef AMP_CLUSTER1_SCRATCH_BASE
#define AMP_CLUSTER1_SCRATCH_BASE 0x22000000UL
#endif
#ifndef AMP_CLUSTER2_SCRATCH_BASE
#define AMP_CLUSTER2_SCRATCH_BASE 0x0UL
#endif
#ifndef AMP_CLUSTER3_SCRATCH_BASE
#define AMP_CLUSTER3_SCRATCH_BASE 0x0UL
#endif
#ifndef AMP_CLUSTER4_SCRATCH_BASE
#define AMP_CLUSTER4_SCRATCH_BASE 0x0UL
#endif
#ifndef AMP_CLUSTER5_SCRATCH_BASE
#define AMP_CLUSTER5_SCRATCH_BASE 0x0UL
#endif
#ifndef AMP_CLUSTER6_SCRATCH_BASE
#define AMP_CLUSTER6_SCRATCH_BASE 0x0UL
#endif
#ifndef AMP_CLUSTER7_SCRATCH_BASE
#define AMP_CLUSTER7_SCRATCH_BASE 0x0UL
#endif

#define AMP_MAILBOX_CMD_PING 0x00000A11U
#define AMP_MAILBOX_RESP_PONG 0x00000B22U
#define AMP_SCRATCH_WORDS 16
#define AMP_SPIN_LIMIT 50000000ULL

/**
 * @brief Look up which AMP cluster owns a hart
 */
static inline uint32_t amp_cluster_of(uint64_t hartid)
{
    const uint32_t bases[8] = {
        AMP_CLUSTER0_HARTID_BASE, AMP_CLUSTER1_HARTID_BASE, AMP_CLUSTER2_HARTID_BASE,
        AMP_CLUSTER3_HARTID_BASE, AMP_CLUSTER4_HARTID_BASE, AMP_CLUSTER5_HARTID_BASE,
        AMP_CLUSTER6_HARTID_BASE, AMP_CLUSTER7_HARTID_BASE,
    };
    const uint32_t counts[8] = {
        AMP_CLUSTER0_HARTS, AMP_CLUSTER1_HARTS, AMP_CLUSTER2_HARTS, AMP_CLUSTER3_HARTS,
        AMP_CLUSTER4_HARTS, AMP_CLUSTER5_HARTS, AMP_CLUSTER6_HARTS, AMP_CLUSTER7_HARTS,
    };
    uint32_t i;

    for (i = 0; i < AMP_NUM_CLUSTERS && i < 8U; i++) {
        if (counts[i] == 0) {
            continue;
        }
        if (hartid >= (uint64_t) bases[i] && hartid < (uint64_t) bases[i] + counts[i]) {
            return i;
        }
    }
    return 0;
}

/**
 * @brief True if this hart is the first hart of its cluster
 */
static inline bool amp_is_cluster_leader(uint64_t hartid)
{
    const uint32_t bases[8] = {
        AMP_CLUSTER0_HARTID_BASE, AMP_CLUSTER1_HARTID_BASE, AMP_CLUSTER2_HARTID_BASE,
        AMP_CLUSTER3_HARTID_BASE, AMP_CLUSTER4_HARTID_BASE, AMP_CLUSTER5_HARTID_BASE,
        AMP_CLUSTER6_HARTID_BASE, AMP_CLUSTER7_HARTID_BASE,
    };
    uint32_t cluster = amp_cluster_of(hartid);
    return hartid == (uint64_t) bases[cluster];
}

/**
 * @brief Scratchpad base for a cluster (0 if none)
 */
static inline uint64_t amp_scratch_base(uint32_t cluster)
{
    const uint64_t bases[8] = {
        AMP_CLUSTER0_SCRATCH_BASE, AMP_CLUSTER1_SCRATCH_BASE, AMP_CLUSTER2_SCRATCH_BASE,
        AMP_CLUSTER3_SCRATCH_BASE, AMP_CLUSTER4_SCRATCH_BASE, AMP_CLUSTER5_SCRATCH_BASE,
        AMP_CLUSTER6_SCRATCH_BASE, AMP_CLUSTER7_SCRATCH_BASE,
    };
    if (cluster >= 8U) {
        return 0;
    }
    return bases[cluster];
}

void run_phase8_amp_tests(void);
void amp_secondary_entry(uint64_t hartid);

#endif /* AMP_H */
