/**
 * @file rvv_detect.h
 * @brief RISC-V Vector Extension (RVV) runtime detection
 *
 * Provides functions to detect RVV availability, query hardware
 * parameters (VLEN, VLENB, ELEN), and enable the vector unit
 * in mstatus.VS.
 */

#ifndef RVV_DETECT_H
#define RVV_DETECT_H

#include "csr.h"

#include <stdbool.h>
#include <stdint.h>

/* =============================================================================
 * mstatus Vector State (VS) Field
 * ============================================================================= */

/** mstatus.VS field: bits [10:9] control vector unit state */
#define MSTATUS_VS_SHIFT 9
#define MSTATUS_VS_MASK (3UL << MSTATUS_VS_SHIFT)
#define MSTATUS_VS_OFF (0UL << MSTATUS_VS_SHIFT)     /* Vector unit disabled */
#define MSTATUS_VS_INITIAL (1UL << MSTATUS_VS_SHIFT)  /* Vector unit initial */
#define MSTATUS_VS_CLEAN (2UL << MSTATUS_VS_SHIFT)    /* Vector unit clean */
#define MSTATUS_VS_DIRTY (3UL << MSTATUS_VS_SHIFT)    /* Vector unit dirty */

/** misa bit for V extension */
#define MISA_V_BIT (1UL << ('V' - 'A'))

/* =============================================================================
 * RVV Detection Functions
 * ============================================================================= */

/**
 * @brief Check if RVV is available by reading misa
 * @return true if V extension bit is set in misa
 */
static inline bool rvv_available(void)
{
    uint64_t misa = read_csr(misa);
    return (misa & MISA_V_BIT) != 0;
}

/**
 * @brief Enable the vector unit by setting mstatus.VS = Initial
 *
 * Must be called before executing any vector instructions.
 * Without this, vector instructions will trap as illegal.
 */
static inline void rvv_enable(void)
{
    /* Clear VS field, then set to Initial (01) */
    unsigned long mstatus = read_csr(mstatus);
    mstatus &= ~MSTATUS_VS_MASK;
    mstatus |= MSTATUS_VS_INITIAL;
    write_csr(mstatus, mstatus);
}

/**
 * @brief Get VLENB (vector register length in bytes)
 * @return VLEN/8 (number of bytes per vector register)
 */
static inline uint64_t rvv_get_vlenb(void)
{
    uint64_t vlenb;
    __asm__ __volatile__("csrr %0, vlenb" : "=r"(vlenb));
    return vlenb;
}

/**
 * @brief Get VLEN (vector register length in bits)
 * @return VLEN in bits (e.g., 128, 256, 512)
 */
static inline uint64_t rvv_get_vlen(void)
{
    return rvv_get_vlenb() * 8;
}

/**
 * @brief Print RVV hardware information to console
 *
 * Prints VLEN, VLENB, and VL for various SEW/LMUL configurations.
 */
void rvv_print_info(void);

#endif /* RVV_DETECT_H */
