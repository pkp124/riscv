# =============================================================================
# RISC-V Bare-Metal Application
# =============================================================================

# Define the application target
add_executable(app
    src/startup.S
    src/main.c
    src/uart.c
    src/htif.c
    src/platform.c
    src/smp.c
)

# Set include directories
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# =============================================================================
# Linker Script Selection
# =============================================================================

# Select linker script based on platform
if(PLATFORM STREQUAL "qemu")
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker/qemu-virt.ld")
elseif(PLATFORM STREQUAL "spike")
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker/spike.ld")
elseif(PLATFORM STREQUAL "gem5")
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker/gem5.ld")
elseif(PLATFORM STREQUAL "renode")
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker/renode.ld")
else()
    message(FATAL_ERROR "Unknown platform: ${PLATFORM}")
endif()

# Check if linker script exists
if(NOT EXISTS ${LINKER_SCRIPT})
    message(WARNING "Linker script not found: ${LINKER_SCRIPT}")
    message(WARNING "Using qemu-virt.ld as fallback")
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker/qemu-virt.ld")
endif()

message(STATUS "Using linker script: ${LINKER_SCRIPT}")

# =============================================================================
# Linker Flags
# =============================================================================

# Pass NUM_HARTS to linker script (for stack allocation)
target_link_options(app PRIVATE
    -T ${LINKER_SCRIPT}
    -Wl,--defsym,NUM_HARTS=${NUM_HARTS}
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/app.map
)

# Make sure linker script is a dependency
set_target_properties(app PROPERTIES
    LINK_DEPENDS ${LINKER_SCRIPT}
)

# =============================================================================
# Post-Build Actions
# =============================================================================

# Generate disassembly
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:app> > ${CMAKE_CURRENT_BINARY_DIR}/app.dump
    COMMENT "Generating disassembly: app.dump"
)

# Generate binary file (optional, for some platforms)
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:app> ${CMAKE_CURRENT_BINARY_DIR}/app.bin
    COMMENT "Generating binary: app.bin"
)

# Print size information
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:app>
    COMMENT "Size information:"
)

# =============================================================================
# Run Targets (using helper functions from Simulators.cmake)
# =============================================================================

# QEMU run target
if(PLATFORM STREQUAL "qemu")
    add_qemu_run_target(run app)
endif()

# Spike run target
if(PLATFORM STREQUAL "spike")
    add_spike_run_target(run app)
endif()

# gem5 run targets
if(PLATFORM STREQUAL "gem5")
    if(GEM5_MODE STREQUAL "se")
        add_gem5_run_target(run app se)
    else()
        add_gem5_run_target(run app fs)
    endif()
endif()

# Renode run target
if(PLATFORM STREQUAL "renode")
    add_renode_run_target(run app)
endif()

# =============================================================================
# Installation
# =============================================================================

install(TARGETS app
    RUNTIME DESTINATION bin
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/app.dump
    ${CMAKE_CURRENT_BINARY_DIR}/app.map
    ${CMAKE_CURRENT_BINARY_DIR}/app.bin
    DESTINATION bin
    OPTIONAL
)
