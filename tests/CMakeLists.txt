# =============================================================================
# Test Configuration
# =============================================================================
# This file defines all CTest test cases for the RISC-V bare-metal project.
# Tests are organized by phase and platform.
# =============================================================================

# CTest configuration
set(CTEST_PROJECT_NAME "RISC-V Bare-Metal Tests")
set(CTEST_NIGHTLY_START_TIME "00:00:00 UTC")

# Test timeout (default: 60 seconds)
set(DART_TESTING_TIMEOUT 60 CACHE STRING "Test timeout in seconds")

# =============================================================================
# Test Utilities
# =============================================================================

# Function to add a simulator test
function(add_simulator_test)
    set(options "")
    set(oneValueArgs NAME PLATFORM COMMAND PASS_REGEX FAIL_REGEX TIMEOUT)
    set(multiValueArgs LABELS)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # Add the test
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_COMMAND})
    
    # Set properties
    set(TEST_PROPERTIES "")
    
    if(DEFINED TEST_PASS_REGEX)
        list(APPEND TEST_PROPERTIES PASS_REGULAR_EXPRESSION "${TEST_PASS_REGEX}")
    endif()
    
    if(DEFINED TEST_FAIL_REGEX)
        list(APPEND TEST_PROPERTIES FAIL_REGULAR_EXPRESSION "${TEST_FAIL_REGEX}")
    endif()
    
    if(DEFINED TEST_TIMEOUT)
        list(APPEND TEST_PROPERTIES TIMEOUT ${TEST_TIMEOUT})
    else()
        list(APPEND TEST_PROPERTIES TIMEOUT ${DART_TESTING_TIMEOUT})
    endif()
    
    if(DEFINED TEST_LABELS)
        list(APPEND TEST_PROPERTIES LABELS "${TEST_LABELS}")
    endif()
    
    if(TEST_PROPERTIES)
        set_tests_properties(${TEST_NAME} PROPERTIES ${TEST_PROPERTIES})
    endif()
endfunction()

# =============================================================================
# Phase 1: Build System Tests
# =============================================================================

# Test: CMake configuration succeeds
add_test(
    NAME phase1_cmake_configure
    COMMAND ${CMAKE_COMMAND} --preset default
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(phase1_cmake_configure PROPERTIES
    LABELS "phase1;build;smoke"
    TIMEOUT 30
)

# Test: Build succeeds (when app exists)
if(TARGET app)
    add_test(
        NAME phase1_cmake_build
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target app
    )
    set_tests_properties(phase1_cmake_build PROPERTIES
        LABELS "phase1;build;smoke"
        TIMEOUT 60
    )
endif()

# =============================================================================
# Phase 2: Single-Core Tests (QEMU)
# =============================================================================

# Test 1: Boot and print "Hello RISC-V"
if(TARGET app AND QEMU_SYSTEM_RISCV64)
    add_test(
        NAME phase2_qemu_boot_hello
        COMMAND ${QEMU_SYSTEM_RISCV64} 
            -machine virt 
            -nographic 
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase2_qemu_boot_hello PROPERTIES
        PASS_REGULAR_EXPRESSION "Hello RISC-V"
        FAIL_REGULAR_EXPRESSION "ERROR|FAIL|panic"
        TIMEOUT 30
        LABELS "phase2;qemu;smoke"
    )
endif()

# Test 2: CSR read - Hart ID
if(TARGET app AND QEMU_SYSTEM_RISCV64)
    add_test(
        NAME phase2_qemu_csr_hartid
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase2_qemu_csr_hartid PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[CSR\\] Hart ID: 0"
        FAIL_REGULAR_EXPRESSION "ERROR|FAIL|panic"
        TIMEOUT 30
        LABELS "phase2;qemu;csr"
    )
endif()

# Test 3: CSR read - mstatus
if(TARGET app AND QEMU_SYSTEM_RISCV64)
    add_test(
        NAME phase2_qemu_csr_mstatus
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase2_qemu_csr_mstatus PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[CSR\\] mstatus:"
        FAIL_REGULAR_EXPRESSION "ERROR|FAIL|panic"
        TIMEOUT 30
        LABELS "phase2;qemu;csr"
    )
endif()

# Test 4: UART character output
if(TARGET app AND QEMU_SYSTEM_RISCV64)
    add_test(
        NAME phase2_qemu_uart_output
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase2_qemu_uart_output PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[UART\\] Character output: PASS"
        FAIL_REGULAR_EXPRESSION "ERROR|FAIL"
        TIMEOUT 30
        LABELS "phase2;qemu;uart"
    )
endif()

# Test 5: Basic memory operations
if(TARGET app AND QEMU_SYSTEM_RISCV64)
    add_test(
        NAME phase2_qemu_memory_ops
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase2_qemu_memory_ops PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Memory operations: PASS"
        FAIL_REGULAR_EXPRESSION "ERROR|FAIL"
        TIMEOUT 30
        LABELS "phase2;qemu;memory"
    )
endif()

# Test 6: Function call stack
if(TARGET app AND QEMU_SYSTEM_RISCV64)
    add_test(
        NAME phase2_qemu_function_calls
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase2_qemu_function_calls PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Function calls: PASS"
        FAIL_REGULAR_EXPRESSION "ERROR|FAIL"
        TIMEOUT 30
        LABELS "phase2;qemu;functional"
    )
endif()

# Test 7: All Phase 2 tests complete
if(TARGET app AND QEMU_SYSTEM_RISCV64)
    add_test(
        NAME phase2_qemu_complete
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase2_qemu_complete PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[RESULT\\] Phase 2 tests: [0-9]+/[0-9]+ PASS"
        FAIL_REGULAR_EXPRESSION "ERROR|0/[0-9]+ PASS"
        TIMEOUT 30
        LABELS "phase2;qemu;integration"
    )
endif()

# =============================================================================
# Phase 3: Cross-Platform Tests (Spike)
# =============================================================================

# To be implemented in Phase 3

# =============================================================================
# Phase 4: SMP Tests
# =============================================================================

# To be implemented in Phase 4

# =============================================================================
# Phase 5: RVV Tests
# =============================================================================

# To be implemented in Phase 5

# =============================================================================
# Phase 6: gem5 Tests
# =============================================================================

# To be implemented in Phase 6

# =============================================================================
# Phase 7: Renode Tests
# =============================================================================

# To be implemented in Phase 7

# =============================================================================
# Test Groups
# =============================================================================

# Add subdirectories for integration tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/CMakeLists.txt)
    add_subdirectory(integration)
endif()

# =============================================================================
# Status Message
# =============================================================================

message(STATUS "Test framework configured")
message(STATUS "Run tests with: ctest --test-dir ${CMAKE_BINARY_DIR} --output-on-failure")
