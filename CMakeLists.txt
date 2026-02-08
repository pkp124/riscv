# =============================================================================
# RISC-V Bare-Metal System Simulation Platform
# =============================================================================
# CMake build system for cross-platform RISC-V bare-metal applications
# Supports: QEMU, Spike, gem5 (SE + FS), Renode
# =============================================================================

cmake_minimum_required(VERSION 3.20)

# =============================================================================
# Project Configuration
# =============================================================================

project(riscv-baremetal
    VERSION 0.1.0
    LANGUAGES C ASM
    DESCRIPTION "RISC-V Bare-Metal System Simulation Platform"
)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# =============================================================================
# Build Options
# =============================================================================

# Platform selection
set(PLATFORM "qemu" CACHE STRING "Target platform: qemu, spike, gem5, renode")
set_property(CACHE PLATFORM PROPERTY STRINGS qemu spike gem5 renode)

# Configuration type
set(CONFIG "single" CACHE STRING "Configuration: single, smp, amp")
set_property(CACHE CONFIG PROPERTY STRINGS single smp amp)

# Number of harts (for SMP/AMP)
set(NUM_HARTS "1" CACHE STRING "Number of harts (1, 2, 4, 8)")

# RISC-V Vector Extension
option(ENABLE_RVV "Enable RISC-V Vector Extension (RVV 1.0)" OFF)

# Vector length (when RVV enabled)
set(VLEN "128" CACHE STRING "Vector length in bits (128, 256, 512, 1024)")

# gem5 mode (SE or FS)
set(GEM5_MODE "fs" CACHE STRING "gem5 mode: se (syscall emulation) or fs (full system)")
set_property(CACHE GEM5_MODE PROPERTY STRINGS se fs)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type: Debug, Release, RelWithDebInfo, MinSizeRel" FORCE)
endif()

# =============================================================================
# Toolchain Configuration
# =============================================================================

# RISC-V toolchain prefix
if(NOT DEFINED RISCV_TOOLCHAIN_PREFIX)
    set(RISCV_TOOLCHAIN_PREFIX "riscv64-unknown-elf-" CACHE STRING "RISC-V toolchain prefix")
endif()

# Verify toolchain is available
find_program(RISCV_CC ${RISCV_TOOLCHAIN_PREFIX}gcc)
if(NOT RISCV_CC)
    message(FATAL_ERROR 
        "RISC-V toolchain not found!\n"
        "Expected: ${RISCV_TOOLCHAIN_PREFIX}gcc\n"
        "Run: ./scripts/setup-toolchain.sh"
    )
endif()

message(STATUS "RISC-V Toolchain: ${RISCV_CC}")

# Set compilers
set(CMAKE_C_COMPILER ${RISCV_TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER ${RISCV_TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY ${RISCV_TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_OBJDUMP ${RISCV_TOOLCHAIN_PREFIX}objdump)
set(CMAKE_SIZE ${RISCV_TOOLCHAIN_PREFIX}size)

# =============================================================================
# ISA Configuration
# =============================================================================

# Determine ISA string based on RVV option
if(ENABLE_RVV)
    set(RISCV_ARCH "rv64gcv" CACHE STRING "RISC-V ISA architecture string" FORCE)
    set(RISCV_MARCH_FULL "rv64gcv")
else()
    set(RISCV_ARCH "rv64gc" CACHE STRING "RISC-V ISA architecture string" FORCE)
    set(RISCV_MARCH_FULL "rv64gc")
endif()

# ABI
set(RISCV_ABI "lp64d" CACHE STRING "RISC-V ABI")

message(STATUS "ISA: ${RISCV_MARCH_FULL}")
message(STATUS "ABI: ${RISCV_ABI}")

# =============================================================================
# Compiler Flags
# =============================================================================

# Common flags for all configurations
set(COMMON_FLAGS
    -march=${RISCV_MARCH_FULL}
    -mabi=${RISCV_ABI}
    -mcmodel=medany
    -nostdlib
    -nostartfiles
    -ffreestanding
    -fno-common
    -ffunction-sections
    -fdata-sections
)

# Warning flags
set(WARNING_FLAGS
    -Wall
    -Wextra
    -Werror
    -Wshadow
    -Wundef
    -Wpointer-arith
    -Wcast-align
    -Wwrite-strings
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wredundant-decls
    -Wnested-externs
)

# Debug flags
set(DEBUG_FLAGS
    -g3
    -Og
    -DDEBUG
)

# Release flags
set(RELEASE_FLAGS
    -O2
    -DNDEBUG
)

# Apply flags based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(${COMMON_FLAGS} ${WARNING_FLAGS} ${DEBUG_FLAGS})
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(${COMMON_FLAGS} ${WARNING_FLAGS} ${RELEASE_FLAGS})
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    add_compile_options(${COMMON_FLAGS} ${WARNING_FLAGS} -Os -DNDEBUG)
else()
    add_compile_options(${COMMON_FLAGS} ${WARNING_FLAGS})
endif()

# =============================================================================
# Linker Flags
# =============================================================================

set(LINKER_FLAGS
    -nostdlib
    -static
    -Wl,--gc-sections
    -Wl,--print-memory-usage
)

add_link_options(${LINKER_FLAGS})

# =============================================================================
# Platform-Specific Definitions
# =============================================================================

# Set platform-specific compile definitions
if(PLATFORM STREQUAL "qemu")
    add_compile_definitions(PLATFORM_QEMU_VIRT)
    set(PLATFORM_UPPER "QEMU")
elseif(PLATFORM STREQUAL "spike")
    add_compile_definitions(PLATFORM_SPIKE)
    set(PLATFORM_UPPER "SPIKE")
elseif(PLATFORM STREQUAL "gem5")
    add_compile_definitions(PLATFORM_GEM5)
    if(GEM5_MODE STREQUAL "se")
        add_compile_definitions(GEM5_MODE_SE)
        set(PLATFORM_UPPER "GEM5_SE")
    else()
        add_compile_definitions(GEM5_MODE_FS)
        set(PLATFORM_UPPER "GEM5_FS")
    endif()
elseif(PLATFORM STREQUAL "renode")
    add_compile_definitions(PLATFORM_RENODE)
    set(PLATFORM_UPPER "RENODE")
else()
    message(FATAL_ERROR "Unknown platform: ${PLATFORM}")
endif()

# Configuration-specific definitions
if(CONFIG STREQUAL "smp" OR CONFIG STREQUAL "amp")
    add_compile_definitions(ENABLE_SMP)
    add_compile_definitions(NUM_HARTS=${NUM_HARTS})
else()
    add_compile_definitions(NUM_HARTS=1)
endif()

if(CONFIG STREQUAL "amp")
    add_compile_definitions(ENABLE_AMP)
endif()

# RVV definitions
if(ENABLE_RVV)
    add_compile_definitions(ENABLE_RVV)
    add_compile_definitions(RVV_VLEN=${VLEN})
endif()

# =============================================================================
# Build Directory Structure
# =============================================================================

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# =============================================================================
# CMake Modules
# =============================================================================

# Add custom CMake modules directory
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# Include helper modules
include(cmake/modules/Simulators.cmake OPTIONAL)
include(cmake/modules/Formatting.cmake OPTIONAL)

# =============================================================================
# Subdirectories
# =============================================================================

# Add application subdirectory (will be created in Phase 2)
if(EXISTS ${CMAKE_SOURCE_DIR}/app/CMakeLists.txt)
    add_subdirectory(app)
else()
    message(STATUS "app/ subdirectory not found - Phase 2 not yet implemented")
endif()

# =============================================================================
# Testing
# =============================================================================

# Enable testing
enable_testing()

# Add tests subdirectory (will be created in Phase 1)
if(EXISTS ${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
else()
    message(STATUS "tests/ subdirectory not found - creating placeholder")
    file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/tests)
endif()

# =============================================================================
# Custom Targets
# =============================================================================

# Format target (if clang-format is available)
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i 
            ${CMAKE_SOURCE_DIR}/app/src/*.c
            ${CMAKE_SOURCE_DIR}/app/include/*.h
        COMMENT "Formatting source code with clang-format"
        VERBATIM
    )
endif()

# Lint target (if cppcheck is available)
find_program(CPPCHECK cppcheck)
if(CPPCHECK)
    add_custom_target(lint
        COMMAND ${CPPCHECK}
            --enable=warning,style,performance,portability
            --std=c11
            --suppress=missingIncludeSystem
            --suppress=unusedFunction
            --error-exitcode=1
            -I ${CMAKE_SOURCE_DIR}/app/include
            ${CMAKE_SOURCE_DIR}/app/src
        COMMENT "Running static analysis with cppcheck"
        VERBATIM
    )
endif()

# =============================================================================
# Status Summary
# =============================================================================

message(STATUS "=============================================================================")
message(STATUS "RISC-V Bare-Metal Build Configuration")
message(STATUS "=============================================================================")
message(STATUS "Platform:       ${PLATFORM_UPPER}")
message(STATUS "Configuration:  ${CONFIG}")
message(STATUS "Build Type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "ISA:            ${RISCV_MARCH_FULL}")
message(STATUS "ABI:            ${RISCV_ABI}")
message(STATUS "Number of Harts: ${NUM_HARTS}")
message(STATUS "RVV Enabled:    ${ENABLE_RVV}")
if(ENABLE_RVV)
    message(STATUS "VLEN:           ${VLEN}")
endif()
if(PLATFORM STREQUAL "gem5")
    message(STATUS "gem5 Mode:      ${GEM5_MODE}")
endif()
message(STATUS "Build Directory: ${CMAKE_BINARY_DIR}")
message(STATUS "=============================================================================")
