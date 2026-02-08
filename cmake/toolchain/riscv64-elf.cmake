# =============================================================================
# RISC-V 64-bit Bare-Metal Toolchain File
# =============================================================================
# This toolchain file configures CMake for cross-compilation to RISC-V 64-bit
# bare-metal targets using the riscv64-unknown-elf- toolchain.
# =============================================================================

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

# Toolchain prefix
if(NOT DEFINED RISCV_TOOLCHAIN_PREFIX)
    set(RISCV_TOOLCHAIN_PREFIX "riscv64-unknown-elf-")
endif()

# Toolchain programs
find_program(CMAKE_C_COMPILER ${RISCV_TOOLCHAIN_PREFIX}gcc REQUIRED)
find_program(CMAKE_ASM_COMPILER ${RISCV_TOOLCHAIN_PREFIX}gcc REQUIRED)
find_program(CMAKE_AR ${RISCV_TOOLCHAIN_PREFIX}ar REQUIRED)
find_program(CMAKE_RANLIB ${RISCV_TOOLCHAIN_PREFIX}ranlib REQUIRED)
find_program(CMAKE_OBJCOPY ${RISCV_TOOLCHAIN_PREFIX}objcopy REQUIRED)
find_program(CMAKE_OBJDUMP ${RISCV_TOOLCHAIN_PREFIX}objdump REQUIRED)
find_program(CMAKE_SIZE ${RISCV_TOOLCHAIN_PREFIX}size REQUIRED)
find_program(CMAKE_READELF ${RISCV_TOOLCHAIN_PREFIX}readelf REQUIRED)

# Verify toolchain is found
if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR 
        "RISC-V toolchain not found!\n"
        "Expected: ${RISCV_TOOLCHAIN_PREFIX}gcc\n"
        "Please install the RISC-V toolchain:\n"
        "  - Run: ./scripts/setup-toolchain.sh\n"
        "  - Or install via package manager: sudo apt install gcc-riscv64-unknown-elf\n"
        "  - Or download from: https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack"
    )
endif()

# Don't search for programs in the host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Skip compiler tests (cross-compilation)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_ASM_COMPILER_WORKS 1)

# Bare-metal flags
set(CMAKE_C_FLAGS_INIT "-nostdlib -ffreestanding")
set(CMAKE_ASM_FLAGS_INIT "-nostdlib")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib -static")

message(STATUS "RISC-V Toolchain configured: ${CMAKE_C_COMPILER}")
