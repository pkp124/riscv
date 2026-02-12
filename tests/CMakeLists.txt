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

# Phase 2 QEMU tests: Only for single-core builds (NUM_HARTS == 1)
if(TARGET app AND QEMU_SYSTEM_RISCV64 AND NUM_HARTS EQUAL 1)

    # Test 1: Boot and print "Hello RISC-V"
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

    # Test 2: CSR read - Hart ID
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

    # Test 3: CSR read - mstatus
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

    # Test 4: UART character output
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

    # Test 5: Basic memory operations
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

    # Test 6: Function call stack
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

    # Test 7: All Phase 2 tests complete
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

# Phase 3 Spike tests: Only for single-core builds (NUM_HARTS == 1)
if(TARGET app AND SPIKE AND NUM_HARTS EQUAL 1)
    add_test(
        NAME phase3_spike_boot_hello
        COMMAND ${SPIKE} --isa=rv64gc $<TARGET_FILE:app>
    )
    set_tests_properties(phase3_spike_boot_hello PROPERTIES
        PASS_REGULAR_EXPRESSION "Hello RISC-V"
        FAIL_REGULAR_EXPRESSION "ERROR|panic"
        TIMEOUT 30
        LABELS "phase3;spike;smoke"
    )
    # Test 2: CSR read - Hart ID on Spike
    add_test(
        NAME phase3_spike_csr_hartid
        COMMAND ${SPIKE} --isa=rv64gc $<TARGET_FILE:app>
    )
    set_tests_properties(phase3_spike_csr_hartid PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[CSR\\] Hart ID: 0"
        FAIL_REGULAR_EXPRESSION "ERROR|panic"
        TIMEOUT 30
        LABELS "phase3;spike;csr"
    )

    # Test 3: CSR read - mstatus on Spike
    add_test(
        NAME phase3_spike_csr_mstatus
        COMMAND ${SPIKE} --isa=rv64gc $<TARGET_FILE:app>
    )
    set_tests_properties(phase3_spike_csr_mstatus PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[CSR\\] mstatus:"
        FAIL_REGULAR_EXPRESSION "ERROR|panic"
        TIMEOUT 30
        LABELS "phase3;spike;csr"
    )

    # Test 4: HTIF console output on Spike
    add_test(
        NAME phase3_spike_htif_output
        COMMAND ${SPIKE} --isa=rv64gc $<TARGET_FILE:app>
    )
    set_tests_properties(phase3_spike_htif_output PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[UART\\] Character output: PASS"
        FAIL_REGULAR_EXPRESSION "ERROR"
        TIMEOUT 30
        LABELS "phase3;spike;htif"
    )

    # Test 5: Memory operations on Spike
    add_test(
        NAME phase3_spike_memory_ops
        COMMAND ${SPIKE} --isa=rv64gc $<TARGET_FILE:app>
    )
    set_tests_properties(phase3_spike_memory_ops PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Memory operations: PASS"
        FAIL_REGULAR_EXPRESSION "ERROR"
        TIMEOUT 30
        LABELS "phase3;spike;memory"
    )

    # Test 6: Function calls on Spike
    add_test(
        NAME phase3_spike_function_calls
        COMMAND ${SPIKE} --isa=rv64gc $<TARGET_FILE:app>
    )
    set_tests_properties(phase3_spike_function_calls PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Function calls: PASS"
        FAIL_REGULAR_EXPRESSION "ERROR"
        TIMEOUT 30
        LABELS "phase3;spike;functional"
    )

    # Test 7: All tests pass on Spike (integration)
    add_test(
        NAME phase3_spike_complete
        COMMAND ${SPIKE} --isa=rv64gc $<TARGET_FILE:app>
    )
    set_tests_properties(phase3_spike_complete PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[RESULT\\] Phase 2 tests: [0-9]+/[0-9]+ PASS"
        FAIL_REGULAR_EXPRESSION "0/[0-9]+ PASS"
        TIMEOUT 30
        LABELS "phase3;spike;integration"
    )

    # Test 8: Platform name reports "Spike"
    add_test(
        NAME phase3_spike_platform_name
        COMMAND ${SPIKE} --isa=rv64gc $<TARGET_FILE:app>
    )
    set_tests_properties(phase3_spike_platform_name PROPERTIES
        PASS_REGULAR_EXPRESSION "Platform: Spike"
        FAIL_REGULAR_EXPRESSION "ERROR|panic"
        TIMEOUT 30
        LABELS "phase3;spike;smoke"
    )

endif()

# =============================================================================
# Phase 4: SMP Tests (QEMU)
# =============================================================================

# Phase 4 tests require SMP build (NUM_HARTS > 1)
if(TARGET app AND QEMU_SYSTEM_RISCV64 AND NUM_HARTS GREATER 1)

    # Test 1: SMP Boot - all harts come online
    add_test(
        NAME phase4_qemu_smp_boot
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -smp ${NUM_HARTS}
            -m 256M
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase4_qemu_smp_boot PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[SMP\\] All ${NUM_HARTS} harts online"
        FAIL_REGULAR_EXPRESSION "ERROR|panic"
        TIMEOUT 60
        LABELS "phase4;qemu;smp;smoke"
    )

    # Test 2: Spinlock correctness
    add_test(
        NAME phase4_qemu_smp_spinlock
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -smp ${NUM_HARTS}
            -m 256M
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase4_qemu_smp_spinlock PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Spinlock: PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] Spinlock: FAIL"
        TIMEOUT 60
        LABELS "phase4;qemu;smp;functional"
    )

    # Test 3: Atomic operations
    add_test(
        NAME phase4_qemu_smp_atomic
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -smp ${NUM_HARTS}
            -m 256M
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase4_qemu_smp_atomic PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Atomic operations: PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] Atomic operations: FAIL"
        TIMEOUT 60
        LABELS "phase4;qemu;smp;functional"
    )

    # Test 4: Barrier synchronization
    add_test(
        NAME phase4_qemu_smp_barrier
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -smp ${NUM_HARTS}
            -m 256M
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase4_qemu_smp_barrier PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Barrier synchronization: PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] Barrier synchronization: FAIL"
        TIMEOUT 60
        LABELS "phase4;qemu;smp;functional"
    )

    # Test 5: Hello RISC-V (still works in SMP mode)
    add_test(
        NAME phase4_qemu_smp_hello
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -smp ${NUM_HARTS}
            -m 256M
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase4_qemu_smp_hello PROPERTIES
        PASS_REGULAR_EXPRESSION "Hello RISC-V"
        FAIL_REGULAR_EXPRESSION "ERROR|panic"
        TIMEOUT 60
        LABELS "phase4;qemu;smp;smoke"
    )

    # Test 6: Per-hart stack (each hart announced online)
    add_test(
        NAME phase4_qemu_smp_per_hart
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -smp ${NUM_HARTS}
            -m 256M
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase4_qemu_smp_per_hart PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[SMP\\] Hart 1 online"
        FAIL_REGULAR_EXPRESSION "ERROR|panic"
        TIMEOUT 60
        LABELS "phase4;qemu;smp;functional"
    )

    # Test 7: All Phase 4 tests pass (integration)
    add_test(
        NAME phase4_qemu_smp_complete
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -smp ${NUM_HARTS}
            -m 256M
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase4_qemu_smp_complete PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[RESULT\\] Phase 4 tests: [0-9]+/[0-9]+ PASS"
        FAIL_REGULAR_EXPRESSION "0/[0-9]+ PASS|FAIL"
        TIMEOUT 60
        LABELS "phase4;qemu;smp;integration"
    )

endif()

# =============================================================================
# Phase 4: SMP Tests (Spike)
# =============================================================================

if(TARGET app AND SPIKE AND NUM_HARTS GREATER 1)

    # Test 1: SMP Boot on Spike
    add_test(
        NAME phase4_spike_smp_boot
        COMMAND ${SPIKE} --isa=rv64gc -p${NUM_HARTS} $<TARGET_FILE:app>
    )
    set_tests_properties(phase4_spike_smp_boot PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[SMP\\] All ${NUM_HARTS} harts online"
        FAIL_REGULAR_EXPRESSION "ERROR|panic"
        TIMEOUT 60
        LABELS "phase4;spike;smp;smoke"
    )

    # Test 2: Spinlock on Spike
    add_test(
        NAME phase4_spike_smp_spinlock
        COMMAND ${SPIKE} --isa=rv64gc -p${NUM_HARTS} $<TARGET_FILE:app>
    )
    set_tests_properties(phase4_spike_smp_spinlock PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Spinlock: PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] Spinlock: FAIL"
        TIMEOUT 60
        LABELS "phase4;spike;smp;functional"
    )

    # Test 3: Atomic operations on Spike
    add_test(
        NAME phase4_spike_smp_atomic
        COMMAND ${SPIKE} --isa=rv64gc -p${NUM_HARTS} $<TARGET_FILE:app>
    )
    set_tests_properties(phase4_spike_smp_atomic PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Atomic operations: PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] Atomic operations: FAIL"
        TIMEOUT 60
        LABELS "phase4;spike;smp;functional"
    )

    # Test 4: Barrier on Spike
    add_test(
        NAME phase4_spike_smp_barrier
        COMMAND ${SPIKE} --isa=rv64gc -p${NUM_HARTS} $<TARGET_FILE:app>
    )
    set_tests_properties(phase4_spike_smp_barrier PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Barrier synchronization: PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] Barrier synchronization: FAIL"
        TIMEOUT 60
        LABELS "phase4;spike;smp;functional"
    )

    # Test 5: All Phase 4 tests pass on Spike (integration)
    add_test(
        NAME phase4_spike_smp_complete
        COMMAND ${SPIKE} --isa=rv64gc -p${NUM_HARTS} $<TARGET_FILE:app>
    )
    set_tests_properties(phase4_spike_smp_complete PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[RESULT\\] Phase 4 tests: [0-9]+/[0-9]+ PASS"
        FAIL_REGULAR_EXPRESSION "0/[0-9]+ PASS|FAIL"
        TIMEOUT 60
        LABELS "phase4;spike;smp;integration"
    )

endif()

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
