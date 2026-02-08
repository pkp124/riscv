# =============================================================================
# Simulator Detection and Configuration
# =============================================================================
# This module finds RISC-V simulators and provides helper functions for
# creating run targets.
# =============================================================================

# Find QEMU
find_program(QEMU_SYSTEM_RISCV64 qemu-system-riscv64)
if(QEMU_SYSTEM_RISCV64)
    message(STATUS "Found QEMU: ${QEMU_SYSTEM_RISCV64}")
    execute_process(
        COMMAND ${QEMU_SYSTEM_RISCV64} --version
        OUTPUT_VARIABLE QEMU_VERSION_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REGEX MATCH "version ([0-9]+\\.[0-9]+\\.[0-9]+)" QEMU_VERSION_MATCH "${QEMU_VERSION_OUTPUT}")
    if(QEMU_VERSION_MATCH)
        set(QEMU_VERSION ${CMAKE_MATCH_1})
        message(STATUS "QEMU version: ${QEMU_VERSION}")
    endif()
else()
    message(STATUS "QEMU not found - install with: sudo apt install qemu-system-misc")
endif()

# Find Spike
find_program(SPIKE spike)
if(SPIKE)
    message(STATUS "Found Spike: ${SPIKE}")
else()
    message(STATUS "Spike not found - install with: ./scripts/setup-simulators.sh")
endif()

# Find gem5
find_program(GEM5_OPT gem5.opt PATHS /opt/gem5/build/RISCV)
if(GEM5_OPT)
    message(STATUS "Found gem5: ${GEM5_OPT}")
else()
    message(STATUS "gem5 not found - install with: ./scripts/setup-simulators.sh")
endif()

# Find Renode
find_program(RENODE renode)
if(RENODE)
    message(STATUS "Found Renode: ${RENODE}")
else()
    message(STATUS "Renode not found - install from: https://renode.io/")
endif()

# =============================================================================
# Helper Functions
# =============================================================================

# Function to add QEMU run target
function(add_qemu_run_target TARGET_NAME ELF_TARGET)
    if(NOT QEMU_SYSTEM_RISCV64)
        return()
    endif()

    set(QEMU_ARGS -machine virt -nographic -bios none)
    
    # Add SMP configuration
    if(DEFINED NUM_HARTS AND NUM_HARTS GREATER 1)
        list(APPEND QEMU_ARGS -smp ${NUM_HARTS})
    endif()
    
    # Add memory size
    list(APPEND QEMU_ARGS -m 128M)
    
    # Add RVV configuration
    if(ENABLE_RVV)
        list(APPEND QEMU_ARGS -cpu rv64,v=true,vlen=${VLEN})
    endif()
    
    # Add kernel
    list(APPEND QEMU_ARGS -kernel $<TARGET_FILE:${ELF_TARGET}>)
    
    add_custom_target(${TARGET_NAME}
        COMMAND ${QEMU_SYSTEM_RISCV64} ${QEMU_ARGS}
        DEPENDS ${ELF_TARGET}
        COMMENT "Running ${ELF_TARGET} on QEMU"
        USES_TERMINAL
    )
endfunction()

# Function to add Spike run target
function(add_spike_run_target TARGET_NAME ELF_TARGET)
    if(NOT SPIKE)
        return()
    endif()

    set(ISA_STRING "rv64gc")
    if(ENABLE_RVV)
        set(ISA_STRING "rv64gcv")
    endif()
    
    set(SPIKE_ARGS --isa=${ISA_STRING})
    
    # Add SMP configuration
    if(DEFINED NUM_HARTS AND NUM_HARTS GREATER 1)
        list(APPEND SPIKE_ARGS -p${NUM_HARTS})
    endif()
    
    # Add RVV varch configuration
    if(ENABLE_RVV)
        list(APPEND SPIKE_ARGS --varch=vlen:${VLEN},elen:64)
    endif()
    
    # Add ELF
    list(APPEND SPIKE_ARGS $<TARGET_FILE:${ELF_TARGET}>)
    
    add_custom_target(${TARGET_NAME}
        COMMAND ${SPIKE} ${SPIKE_ARGS}
        DEPENDS ${ELF_TARGET}
        COMMENT "Running ${ELF_TARGET} on Spike"
        USES_TERMINAL
    )
endfunction()

# Function to add gem5 run target
function(add_gem5_run_target TARGET_NAME ELF_TARGET MODE)
    if(NOT GEM5_OPT)
        return()
    endif()

    if(MODE STREQUAL "se")
        set(CONFIG_SCRIPT ${CMAKE_SOURCE_DIR}/platforms/gem5/configs/se_config.py)
    else()
        set(CONFIG_SCRIPT ${CMAKE_SOURCE_DIR}/platforms/gem5/configs/fs_config.py)
    endif()
    
    if(NOT EXISTS ${CONFIG_SCRIPT})
        message(STATUS "gem5 config script not found: ${CONFIG_SCRIPT}")
        return()
    endif()
    
    add_custom_target(${TARGET_NAME}
        COMMAND ${GEM5_OPT} ${CONFIG_SCRIPT} --cmd=$<TARGET_FILE:${ELF_TARGET}>
        DEPENDS ${ELF_TARGET}
        COMMENT "Running ${ELF_TARGET} on gem5 (${MODE} mode)"
        USES_TERMINAL
    )
endfunction()

# Function to add Renode run target
function(add_renode_run_target TARGET_NAME ELF_TARGET)
    if(NOT RENODE)
        return()
    endif()

    set(RENODE_SCRIPT ${CMAKE_SOURCE_DIR}/platforms/renode/configs/run.resc)
    if(NOT EXISTS ${RENODE_SCRIPT})
        message(STATUS "Renode script not found: ${RENODE_SCRIPT}")
        return()
    endif()
    
    add_custom_target(${TARGET_NAME}
        COMMAND ${RENODE} -e "s @${RENODE_SCRIPT}"
        DEPENDS ${ELF_TARGET}
        COMMENT "Running ${ELF_TARGET} on Renode"
        USES_TERMINAL
    )
endfunction()
