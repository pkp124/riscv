/**
 * =============================================================================
 * RISC-V Bare-Metal Startup Code
 * =============================================================================
 * This is the entry point for the bare-metal application.
 * Responsibilities:
 *   1. Clear BSS section (uninitialized data)
 *   2. Set up stack pointer (per-hart for SMP)
 *   3. Call platform_init() (C function)
 *   4. Call main() (C function)
 *   5. Handle return from main() (infinite loop or exit)
 * =============================================================================
 */

.section .text.start
.global _start
.type _start, @function

_start:
    /* =========================================================================
     * Multi-hart handling (SMP support)
     * For single-core: only hart 0 exists
     * For SMP: hart 0 does initialization, others wait
     * ========================================================================= */
    
    csrr    a0, mhartid             # Read hart ID into a0
    
#if NUM_HARTS > 1
    /* In SMP mode, only hart 0 clears BSS */
    bnez    a0, .Lsecondary_hart    # If not hart 0, jump to secondary init
#endif

    /* =========================================================================
     * Primary hart (hart 0) initialization
     * ========================================================================= */

.Lprimary_hart:
    /* Clear BSS section (uninitialized data must be zeroed) */
    la      t0, __bss_start         # t0 = start of BSS
    la      t1, __bss_end           # t1 = end of BSS
    
.Lclear_bss:
    bge     t0, t1, .Lbss_done      # if t0 >= t1, BSS is cleared
    sd      zero, 0(t0)             # *t0 = 0 (clear 8 bytes)
    addi    t0, t0, 8               # t0 += 8
    j       .Lclear_bss             # loop

.Lbss_done:
    /* BSS is now cleared */

    /* Set up stack pointer for hart 0 */
    la      sp, __stack_top         # sp = top of stack (grows down)
    
    /* Jump to common initialization */
    j       .Lcommon_init

    /* =========================================================================
     * Secondary hart initialization (SMP only)
     * ========================================================================= */
#if NUM_HARTS > 1
.Lsecondary_hart:
    /* Calculate stack pointer for this hart */
    /* Each hart gets STACK_SIZE bytes */
    /* Hart N stack: __stack_start + (N * STACK_SIZE) + STACK_SIZE */
    
    la      t0, __stack_start       # t0 = base of all stacks
    li      t1, 0x10000             # t1 = STACK_SIZE (64 KB, must match linker)
    mul     t2, a0, t1              # t2 = hartid * STACK_SIZE
    add     t0, t0, t2              # t0 = stack_start + offset
    add     sp, t0, t1              # sp = stack_start + offset + STACK_SIZE
    
    /* Fall through to common init */
#endif

    /* =========================================================================
     * Common initialization (all harts)
     * ========================================================================= */
.Lcommon_init:
    /* Disable interrupts */
    csrci   mstatus, 0x8            # Clear MIE bit (bit 3)
    
    /* Set up trap vector (for exceptions/interrupts) */
    la      t0, _trap_handler       # Load address of trap handler
    csrw    mtvec, t0               # Set mtvec = trap handler address
    
    /* Call platform_init() - C function */
    call    platform_init
    
    /* Call main() - C function */
    call    main
    
    /* If main returns, loop forever */
.Lexit:
    wfi                             # Wait for interrupt (low power)
    j       .Lexit                  # Infinite loop

/**
 * =============================================================================
 * Default Trap Handler
 * =============================================================================
 * Simple trap handler that just loops forever.
 * In Phase 2, we don't handle traps - just catch them.
 * Later phases will implement proper trap handling.
 * =============================================================================
 */
.align 4
.global _trap_handler
.type _trap_handler, @function

_trap_handler:
    /* Save context (minimal) */
    csrr    t0, mcause              # Read trap cause
    csrr    t1, mepc                # Read exception PC
    csrr    t2, mtval               # Read trap value
    
    /* For now, just loop forever */
    /* In Phase 2, we don't expect any traps */
.Ltrap_loop:
    wfi
    j       .Ltrap_loop

.size _trap_handler, . - _trap_handler
.size _start, . - _start
