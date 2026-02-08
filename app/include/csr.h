/**
 * @file csr.h
 * @brief RISC-V Control and Status Register (CSR) access macros
 *
 * This header provides inline assembly macros for reading and writing
 * RISC-V CSRs. Supports all standard machine-mode CSRs.
 */

#ifndef CSR_H
#define CSR_H

#include <stdint.h>

/* =============================================================================
 * CSR Access Macros
 * ============================================================================= */

/**
 * @brief Read a CSR
 * @param reg CSR name (e.g., mhartid, mstatus)
 * @return CSR value
 */
#define read_csr(reg)                                                                              \
    ({                                                                                             \
        unsigned long __tmp;                                                                       \
        __asm__ __volatile__("csrr %0, " #reg : "=r"(__tmp));                                      \
        __tmp;                                                                                     \
    })

/**
 * @brief Write a CSR
 * @param reg CSR name
 * @param val Value to write
 */
#define write_csr(reg, val) ({ __asm__ __volatile__("csrw " #reg ", %0" ::"r"(val)); })

/**
 * @brief Set bits in a CSR (read-modify-write)
 * @param reg CSR name
 * @param bits Bits to set
 * @return Previous CSR value
 */
#define set_csr(reg, bits)                                                                         \
    ({                                                                                             \
        unsigned long __tmp;                                                                       \
        __asm__ __volatile__("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "r"(bits));                  \
        __tmp;                                                                                     \
    })

/**
 * @brief Clear bits in a CSR (read-modify-write)
 * @param reg CSR name
 * @param bits Bits to clear
 * @return Previous CSR value
 */
#define clear_csr(reg, bits)                                                                       \
    ({                                                                                             \
        unsigned long __tmp;                                                                       \
        __asm__ __volatile__("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "r"(bits));                  \
        __tmp;                                                                                     \
    })

/* =============================================================================
 * Machine-Mode CSR Register Numbers
 * ============================================================================= */

/* Machine Information Registers */
#define CSR_MVENDORID 0xF11
#define CSR_MARCHID 0xF12
#define CSR_MIMPID 0xF13
#define CSR_MHARTID 0xF14

/* Machine Trap Setup */
#define CSR_MSTATUS 0x300
#define CSR_MISA 0x301
#define CSR_MEDELEG 0x302
#define CSR_MIDELEG 0x303
#define CSR_MIE 0x304
#define CSR_MTVEC 0x305
#define CSR_MCOUNTEREN 0x306

/* Machine Trap Handling */
#define CSR_MSCRATCH 0x340
#define CSR_MEPC 0x341
#define CSR_MCAUSE 0x342
#define CSR_MTVAL 0x343
#define CSR_MIP 0x344

/* Machine Counter/Timers */
#define CSR_MCYCLE 0xB00
#define CSR_MINSTRET 0xB02
#define CSR_MCYCLEH 0xB80
#define CSR_MINSTRETH 0xB82

/* User-mode CSRs (accessible in M-mode) */
#define CSR_CYCLE 0xC00
#define CSR_TIME 0xC01
#define CSR_INSTRET 0xC02
#define CSR_CYCLEH 0xC80
#define CSR_TIMEH 0xC81
#define CSR_INSTRETH 0xC82

/* =============================================================================
 * mstatus Register Bits
 * ============================================================================= */

#define MSTATUS_UIE (1UL << 0)   /* User Interrupt Enable */
#define MSTATUS_SIE (1UL << 1)   /* Supervisor Interrupt Enable */
#define MSTATUS_MIE (1UL << 3)   /* Machine Interrupt Enable */
#define MSTATUS_UPIE (1UL << 4)  /* User Previous Interrupt Enable */
#define MSTATUS_SPIE (1UL << 5)  /* Supervisor Previous Interrupt Enable */
#define MSTATUS_MPIE (1UL << 7)  /* Machine Previous Interrupt Enable */
#define MSTATUS_SPP (1UL << 8)   /* Supervisor Previous Privilege */
#define MSTATUS_MPP (3UL << 11)  /* Machine Previous Privilege */
#define MSTATUS_FS (3UL << 13)   /* Floating-point Status */
#define MSTATUS_XS (3UL << 15)   /* Extension Status */
#define MSTATUS_MPRV (1UL << 17) /* Modify Privilege */
#define MSTATUS_SUM (1UL << 18)  /* Supervisor User Memory access */
#define MSTATUS_MXR (1UL << 19)  /* Make eXecutable Readable */
#define MSTATUS_TVM (1UL << 20)  /* Trap Virtual Memory */
#define MSTATUS_TW (1UL << 21)   /* Timeout Wait */
#define MSTATUS_TSR (1UL << 22)  /* Trap SRET */
#define MSTATUS_SD (1UL << 63)   /* State Dirty (RV64 only) */

/* Privilege modes */
#define PRV_U 0UL /* User mode */
#define PRV_S 1UL /* Supervisor mode */
#define PRV_M 3UL /* Machine mode */

/* =============================================================================
 * mie/mip Register Bits (Interrupt Enable/Pending)
 * ============================================================================= */

#define MIE_USIE (1UL << 0)  /* User Software Interrupt Enable */
#define MIE_SSIE (1UL << 1)  /* Supervisor Software Interrupt Enable */
#define MIE_MSIE (1UL << 3)  /* Machine Software Interrupt Enable */
#define MIE_UTIE (1UL << 4)  /* User Timer Interrupt Enable */
#define MIE_STIE (1UL << 5)  /* Supervisor Timer Interrupt Enable */
#define MIE_MTIE (1UL << 7)  /* Machine Timer Interrupt Enable */
#define MIE_UEIE (1UL << 8)  /* User External Interrupt Enable */
#define MIE_SEIE (1UL << 9)  /* Supervisor External Interrupt Enable */
#define MIE_MEIE (1UL << 11) /* Machine External Interrupt Enable */

/* =============================================================================
 * mcause Register Values
 * ============================================================================= */

/* Exception codes (mcause[XLEN-1] = 0) */
#define CAUSE_MISALIGNED_FETCH 0
#define CAUSE_FETCH_ACCESS 1
#define CAUSE_ILLEGAL_INSTRUCTION 2
#define CAUSE_BREAKPOINT 3
#define CAUSE_MISALIGNED_LOAD 4
#define CAUSE_LOAD_ACCESS 5
#define CAUSE_MISALIGNED_STORE 6
#define CAUSE_STORE_ACCESS 7
#define CAUSE_USER_ECALL 8
#define CAUSE_SUPERVISOR_ECALL 9
#define CAUSE_MACHINE_ECALL 11
#define CAUSE_FETCH_PAGE_FAULT 12
#define CAUSE_LOAD_PAGE_FAULT 13
#define CAUSE_STORE_PAGE_FAULT 15

/* Interrupt codes (mcause[XLEN-1] = 1) */
#define CAUSE_INTERRUPT (1UL << 63)
#define IRQ_S_SOFT 1
#define IRQ_M_SOFT 3
#define IRQ_S_TIMER 5
#define IRQ_M_TIMER 7
#define IRQ_S_EXT 9
#define IRQ_M_EXT 11

/* =============================================================================
 * Helper Functions
 * ============================================================================= */

/**
 * @brief Get current hart ID
 * @return Hart ID (0 for single-core, 0-N for multi-core)
 */
static inline uint64_t csr_read_hartid(void)
{
    return read_csr(mhartid);
}

/**
 * @brief Get current cycle count
 * @return Cycle counter value
 */
static inline uint64_t csr_read_cycle(void)
{
    return read_csr(mcycle);
}

/**
 * @brief Get current instruction count
 * @return Instruction counter value
 */
static inline uint64_t csr_read_instret(void)
{
    return read_csr(minstret);
}

/**
 * @brief Get current time
 * @return Time counter value
 */
static inline uint64_t csr_read_time(void)
{
    return read_csr(time);
}

/**
 * @brief Enable machine-mode interrupts
 */
static inline void csr_enable_interrupts(void)
{
    set_csr(mstatus, MSTATUS_MIE);
}

/**
 * @brief Disable machine-mode interrupts
 */
static inline void csr_disable_interrupts(void)
{
    clear_csr(mstatus, MSTATUS_MIE);
}

#endif /* CSR_H */
