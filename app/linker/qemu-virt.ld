/**
 * =============================================================================
 * RISC-V Bare-Metal Linker Script for QEMU virt Machine
 * =============================================================================
 * Memory layout for QEMU virt machine:
 *   - RAM starts at 0x80000000 (2 GiB mark, standard RISC-V)
 *   - Stack grows downward from high memory
 *   - Heap is optional (not used in bare-metal)
 * =============================================================================
 */

OUTPUT_ARCH(riscv)
ENTRY(_start)

/* Memory regions for QEMU virt machine */
MEMORY {
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

/* Stack size per hart */
STACK_SIZE = 0x10000;  /* 64 KB per hart */

SECTIONS {
    /* =========================================================================
     * Text section (code)
     * ========================================================================= */
    .text : {
        PROVIDE(__text_start = .);
        *(.text.start)      /* Startup code first */
        *(.text .text.*)    /* All other code */
        *(.gnu.linkonce.t.*)
        PROVIDE(__text_end = .);
    } > RAM

    /* Ensure text section is aligned */
    . = ALIGN(16);

    /* =========================================================================
     * Read-only data section
     * ========================================================================= */
    .rodata : {
        PROVIDE(__rodata_start = .);
        *(.rodata .rodata.*)
        *(.gnu.linkonce.r.*)
        PROVIDE(__rodata_end = .);
    } > RAM

    /* Ensure rodata section is aligned */
    . = ALIGN(16);

    /* =========================================================================
     * Data section (initialized data)
     * ========================================================================= */
    .data : {
        PROVIDE(__data_start = .);
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        *(.sdata .sdata.*)
        *(.gnu.linkonce.s.*)
        PROVIDE(__data_end = .);
    } > RAM

    /* Ensure data section is aligned */
    . = ALIGN(16);

    /* =========================================================================
     * BSS section (uninitialized data - must be zeroed by startup code)
     * ========================================================================= */
    .bss : {
        PROVIDE(__bss_start = .);
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(.sbss .sbss.*)
        *(.gnu.linkonce.sb.*)
        *(COMMON)
        PROVIDE(__bss_end = .);
    } > RAM

    /* Ensure BSS section is aligned */
    . = ALIGN(16);

    /* =========================================================================
     * Heap (optional, not used in bare-metal)
     * ========================================================================= */
    .heap : {
        PROVIDE(__heap_start = .);
        . = . + 0x10000;  /* 64 KB heap (optional) */
        PROVIDE(__heap_end = .);
    } > RAM

    /* Ensure heap is aligned */
    . = ALIGN(16);

    /* =========================================================================
     * Stack
     * Stack grows downward, so we allocate from top of RAM
     * For SMP, we need multiple stacks (allocated per-hart)
     * ========================================================================= */
    .stack : {
        /* Ensure stack starts at 16-byte boundary for ABI compliance */
        . = ALIGN(16);
        PROVIDE(__stack_start = .);
        . = . + STACK_SIZE * NUM_HARTS;  /* NUM_HARTS defined by CMake */
        PROVIDE(__stack_end = .);
    } > RAM

    /* Stack pointer starts at top of stack (grows down) */
    PROVIDE(__stack_top = __stack_end);

    /* =========================================================================
     * End of used RAM
     * ========================================================================= */
    PROVIDE(__heap_size = __heap_end - __heap_start);
    PROVIDE(__bss_size = __bss_end - __bss_start);
    
    /* Sanity check: ensure we fit in RAM */
    ASSERT(__stack_end <= 0x80000000 + 128M, "Program too large! Exceeds RAM.")

    /* =========================================================================
     * Debug sections (not loaded into RAM)
     * ========================================================================= */
    .comment 0 : { *(.comment) }
    .debug 0 : { *(.debug) }
    .debug_abbrev 0 : { *(.debug_abbrev) }
    .debug_aranges 0 : { *(.debug_aranges) }
    .debug_frame 0 : { *(.debug_frame) }
    .debug_info 0 : { *(.debug_info) }
    .debug_line 0 : { *(.debug_line) }
    .debug_loc 0 : { *(.debug_loc) }
    .debug_macinfo 0 : { *(.debug_macinfo) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    .debug_pubtypes 0 : { *(.debug_pubtypes) }
    .debug_ranges 0 : { *(.debug_ranges) }
    .debug_str 0 : { *(.debug_str) }
    .debug_types 0 : { *(.debug_types) }

    /* Discard unnecessary sections */
    /DISCARD/ : {
        *(.eh_frame)
        *(.note.GNU-stack)
    }
}
