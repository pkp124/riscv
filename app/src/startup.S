/**
 * =============================================================================
 * RISC-V Bare-Metal Startup Code
 * =============================================================================
 * This is the entry point for the bare-metal application.
 *
 * Boot Protocol:
 *   Hart 0 (Primary):
 *     1. Clear BSS section
 *     2. Set up stack pointer
 *     3. Disable interrupts, set trap handler
 *     4. Call platform_init()
 *     5. Call main()
 *     6. Call platform_exit()
 *
 *   Secondary Harts (SMP, NUM_HARTS > 1):
 *     1. Set up per-hart stack pointer
 *     2. Disable interrupts, set trap handler
 *     3. Spin on smp_hart_release flag (in BSS, cleared by Hart 0)
 *     4. When released, call smp_secondary_entry(hartid)
 *     5. Enter WFI loop (should not return)
 * =============================================================================
 */

.section .text.start
.global _start
.type _start, @function

_start:
    /* =========================================================================
     * Read Hart ID
     * ========================================================================= */
#if defined(PLATFORM_GEM5) && defined(GEM5_MODE_SE)
    /* gem5 SE runs in user mode; mhartid not accessible. Use 0 (single-threaded). */
    li      a0, 0
#else
    csrr    a0, mhartid             # Read hart ID into a0
#endif

#if NUM_HARTS > 1
    /* In SMP mode, only hart 0 does initialization */
    bnez    a0, .Lsecondary_hart    # If not hart 0, jump to secondary path
#endif

    /* =========================================================================
     * Primary Hart (Hart 0) - System Initialization
     * ========================================================================= */

.Lprimary_hart:
    /* Step 1: Clear BSS section (uninitialized data must be zeroed) */
    la      t0, __bss_start         # t0 = start of BSS
    la      t1, __bss_end           # t1 = end of BSS

.Lclear_bss:
    bge     t0, t1, .Lbss_done      # if t0 >= t1, BSS is cleared
    sd      zero, 0(t0)             # *t0 = 0 (clear 8 bytes)
    addi    t0, t0, 8               # t0 += 8
    j       .Lclear_bss             # loop

.Lbss_done:
    /* Step 2: Set up stack pointer for hart 0 */
#if NUM_HARTS > 1
    /* SMP: Hart 0 gets the first stack slot [__stack_start, __stack_start + STACK_SIZE)
     * Each hart's sp = __stack_start + (hartid + 1) * STACK_SIZE */
    la      sp, __stack_start
    li      t1, 0x10000             # STACK_SIZE (64 KB, must match linker)
    add     sp, sp, t1              # sp = __stack_start + STACK_SIZE
    andi    sp, sp, -16             # Align sp to 16-byte boundary (ABI requirement)
#else
    la      sp, __stack_top         # Single-core: sp = top of stack (grows down)
#endif

#if !(defined(PLATFORM_GEM5) && defined(GEM5_MODE_SE))
    /* Step 3: Disable interrupts and set trap handler (M-mode only) */
    csrci   mstatus, 0x8            # Clear MIE bit (bit 3)
    la      t0, _trap_handler       # Load address of trap handler
    csrw    mtvec, t0               # Set mtvec = trap handler address
#endif

    /* Step 4: Call platform_init() - C function */
    call    platform_init

    /* Step 5: Call main() - C function */
    call    main

    /* Step 6: If main returns, do platform-specific exit */
    /* a0 contains main()'s return value (exit code) */
    call    platform_exit

    /* Fallback: loop forever (should not reach here) */
.Lprimary_exit:
    wfi                             # Wait for interrupt (low power)
    j       .Lprimary_exit          # Infinite loop

    /* =========================================================================
     * Secondary Hart Initialization (SMP only)
     * ========================================================================= */
#if NUM_HARTS > 1
.Lsecondary_hart:
    /* Secondary harts: set up stack, wait for release, then run */

    /* Step 1: Calculate per-hart stack pointer */
    /* Each hart gets STACK_SIZE (64 KB) bytes of stack.
     * Hart N's sp = __stack_start + (N + 1) * STACK_SIZE
     * (Stack grows downward, so sp points to top of allocated region)
     */
    la      t0, __stack_start       # t0 = base of all stacks
    li      t1, 0x10000             # t1 = STACK_SIZE (64 KB, must match linker)
    addi    t2, a0, 1               # t2 = hartid + 1
    mul     t2, t2, t1              # t2 = (hartid + 1) * STACK_SIZE
    add     sp, t0, t2              # sp = stack_start + (hartid + 1) * STACK_SIZE
    andi    sp, sp, -16             # Align sp to 16-byte boundary (ABI requirement)

    /* Step 2: Disable interrupts and set trap handler */
    csrci   mstatus, 0x8            # Clear MIE bit (bit 3)
    la      t0, _trap_handler       # Load address of trap handler
    csrw    mtvec, t0               # Set mtvec = trap handler address

    /* Step 3: Spin on smp_hart_release flag */
    /* This flag is in BSS, zeroed by hart 0. When hart 0 calls
     * smp_release_harts(), it sets this to non-zero. */
    la      t0, smp_hart_release    # Load address of release flag
.Lwait_release:
    lw      t1, 0(t0)              # Load release flag
    bnez    t1, .Lreleased         # If non-zero, proceed
    j       .Lwait_release         # Keep polling
.Lreleased:
    fence   rw, rw                  # Memory fence after observing release

    /* Step 4: Call smp_secondary_entry(hartid) */
    /* a0 still contains mhartid from the csrr at _start */
    call    smp_secondary_entry

    /* Step 5: Should not return, but if it does, loop forever */
.Lsecondary_exit:
    wfi
    j       .Lsecondary_exit
#endif

/**
 * =============================================================================
 * Default Trap Handler
 * =============================================================================
 * Simple trap handler that reads diagnostic CSRs and loops.
 * Later phases will implement proper trap handling.
 * =============================================================================
 */
.align 4
.global _trap_handler
.type _trap_handler, @function

_trap_handler:
    /* Save diagnostic CSRs (for debugging) */
    csrr    t0, mcause              # Read trap cause
    csrr    t1, mepc                # Read exception PC
    csrr    t2, mtval               # Read trap value

    /* For now, just loop forever */
.Ltrap_loop:
    wfi
    j       .Ltrap_loop

.size _trap_handler, . - _trap_handler
.size _start, . - _start
