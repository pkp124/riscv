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

# Find gem5 (check multiple common locations)
find_program(GEM5_OPT gem5.opt
    PATHS
        ${CMAKE_SOURCE_DIR}/gem5-build/build/RISCV
        /opt/gem5/build/RISCV
        $ENV{GEM5_HOME}/build/RISCV
        ${CMAKE_SOURCE_DIR}/../gem5/build/RISCV
    NO_DEFAULT_PATH
)
# Also check system PATH
if(NOT GEM5_OPT)
    find_program(GEM5_OPT gem5.opt)
endif()
if(GEM5_OPT)
    message(STATUS "Found gem5: ${GEM5_OPT}")
    # Try to get gem5 version
    execute_process(
        COMMAND ${GEM5_OPT} --version
        OUTPUT_VARIABLE GEM5_VERSION_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        TIMEOUT 5
    )
    if(GEM5_VERSION_OUTPUT)
        message(STATUS "gem5 version: ${GEM5_VERSION_OUTPUT}")
    endif()
else()
    message(STATUS "gem5 not found - install with: ./scripts/setup-simulators.sh")
endif()

# gem5 Python config locations
set(GEM5_FS_CONFIG ${CMAKE_SOURCE_DIR}/platforms/gem5/configs/fs_config.py)
set(GEM5_SE_CONFIG ${CMAKE_SOURCE_DIR}/platforms/gem5/configs/se_config.py)

if(EXISTS ${GEM5_FS_CONFIG})
    message(STATUS "gem5 FS config: ${GEM5_FS_CONFIG}")
endif()
if(EXISTS ${GEM5_SE_CONFIG})
    message(STATUS "gem5 SE config: ${GEM5_SE_CONFIG}")
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
    
    # Note: RVV VLEN configuration via --varch was removed in newer Spike versions.
    # The --isa=rv64gcv flag alone enables RVV with default VLEN.
    
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
# MODE: "se" or "fs"
# Optional: CPU_TYPE (default: AtomicSimpleCPU)
function(add_gem5_run_target TARGET_NAME ELF_TARGET MODE)
    if(NOT GEM5_OPT)
        return()
    endif()

    # Select config script based on mode
    if(MODE STREQUAL "se")
        set(CONFIG_SCRIPT ${GEM5_SE_CONFIG})
    else()
        set(CONFIG_SCRIPT ${GEM5_FS_CONFIG})
    endif()
    
    if(NOT EXISTS ${CONFIG_SCRIPT})
        message(STATUS "gem5 config script not found: ${CONFIG_SCRIPT}")
        return()
    endif()

    # Default CPU type
    set(GEM5_CPU_TYPE "AtomicSimpleCPU" CACHE STRING "gem5 CPU model")

    # Build gem5 command
    set(GEM5_ARGS ${CONFIG_SCRIPT})
    list(APPEND GEM5_ARGS --cpu-type=${GEM5_CPU_TYPE})
    list(APPEND GEM5_ARGS --cmd=$<TARGET_FILE:${ELF_TARGET}>)

    # Add SMP if configured
    if(DEFINED NUM_HARTS AND NUM_HARTS GREATER 1)
        list(APPEND GEM5_ARGS --num-cpus=${NUM_HARTS})
    endif()
    
    add_custom_target(${TARGET_NAME}
        COMMAND ${GEM5_OPT} ${GEM5_ARGS}
        DEPENDS ${ELF_TARGET}
        COMMENT "Running ${ELF_TARGET} on gem5 (${MODE} mode, ${GEM5_CPU_TYPE})"
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
