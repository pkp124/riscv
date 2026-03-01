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

# Phase 2 QEMU tests: Only for single-core builds (NUM_HARTS == 1) without RVV
if(TARGET app AND QEMU_SYSTEM_RISCV64 AND NUM_HARTS EQUAL 1 AND NOT ENABLE_RVV)

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

# Phase 3 Spike tests: Only for single-core builds (NUM_HARTS == 1) without RVV
if(TARGET app AND SPIKE AND NUM_HARTS EQUAL 1 AND NOT ENABLE_RVV)
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
# Phase 5: RVV Tests (QEMU)
# =============================================================================

# Phase 5 QEMU tests: Single-core with RVV enabled
if(TARGET app AND QEMU_SYSTEM_RISCV64 AND ENABLE_RVV AND NUM_HARTS EQUAL 1)

    # Test 1: RVV Detection - check misa V-bit
    add_test(
        NAME phase5_qemu_rvv_detect
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -cpu rv64,v=true,vlen=${VLEN}
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_qemu_rvv_detect PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] RVV detection: PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] RVV detection: FAIL"
        TIMEOUT 30
        LABELS "phase5;qemu;rvv;smoke"
    )

    # Test 2: RVV VLEN detection
    add_test(
        NAME phase5_qemu_rvv_vlen
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -cpu rv64,v=true,vlen=${VLEN}
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_qemu_rvv_vlen PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[RVV\\] VLEN  = ${VLEN} bits"
        FAIL_REGULAR_EXPRESSION "\\[RVV\\] Not available"
        TIMEOUT 30
        LABELS "phase5;qemu;rvv;detect"
    )

    # Test 3: Integer vector add
    add_test(
        NAME phase5_qemu_rvv_vec_add_i32
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -cpu rv64,v=true,vlen=${VLEN}
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_qemu_rvv_vec_add_i32 PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Vec add \\(int32\\): PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] Vec add \\(int32\\): FAIL"
        TIMEOUT 30
        LABELS "phase5;qemu;rvv;functional"
    )

    # Test 4: Vector memcpy
    add_test(
        NAME phase5_qemu_rvv_memcpy
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -cpu rv64,v=true,vlen=${VLEN}
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_qemu_rvv_memcpy PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Vec memcpy: PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] Vec memcpy: FAIL"
        TIMEOUT 30
        LABELS "phase5;qemu;rvv;functional"
    )

    # Test 5: Float32 vector add
    add_test(
        NAME phase5_qemu_rvv_vec_add_f32
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -cpu rv64,v=true,vlen=${VLEN}
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_qemu_rvv_vec_add_f32 PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Vec add \\(float32\\): PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] Vec add \\(float32\\): FAIL"
        TIMEOUT 30
        LABELS "phase5;qemu;rvv;functional"
    )

    # Test 6: Dot product
    add_test(
        NAME phase5_qemu_rvv_dot_product
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -cpu rv64,v=true,vlen=${VLEN}
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_qemu_rvv_dot_product PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Dot product \\(float32\\): PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] Dot product \\(float32\\): FAIL"
        TIMEOUT 30
        LABELS "phase5;qemu;rvv;functional"
    )

    # Test 7: SAXPY
    add_test(
        NAME phase5_qemu_rvv_saxpy
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -cpu rv64,v=true,vlen=${VLEN}
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_qemu_rvv_saxpy PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] SAXPY \\(float32\\): PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] SAXPY \\(float32\\): FAIL"
        TIMEOUT 30
        LABELS "phase5;qemu;rvv;functional"
    )

    # Test 8: Matrix multiply
    add_test(
        NAME phase5_qemu_rvv_matmul
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -cpu rv64,v=true,vlen=${VLEN}
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_qemu_rvv_matmul PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Matrix multiply \\(float32\\): PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] Matrix multiply \\(float32\\): FAIL"
        TIMEOUT 30
        LABELS "phase5;qemu;rvv;functional"
    )

    # Test 9: All Phase 5 tests pass (integration)
    add_test(
        NAME phase5_qemu_rvv_complete
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -cpu rv64,v=true,vlen=${VLEN}
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_qemu_rvv_complete PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[RESULT\\] Phase 5 tests: 7/7 PASS"
        FAIL_REGULAR_EXPRESSION "FAIL"
        TIMEOUT 30
        LABELS "phase5;qemu;rvv;integration"
    )

    # Test 10: Hello RISC-V (still works in RVV mode)
    add_test(
        NAME phase5_qemu_rvv_hello
        COMMAND ${QEMU_SYSTEM_RISCV64}
            -machine virt
            -cpu rv64,v=true,vlen=${VLEN}
            -nographic
            -bios none
            -kernel $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_qemu_rvv_hello PROPERTIES
        PASS_REGULAR_EXPRESSION "Hello RISC-V"
        FAIL_REGULAR_EXPRESSION "ERROR|panic"
        TIMEOUT 30
        LABELS "phase5;qemu;rvv;smoke"
    )

endif()

# =============================================================================
# Phase 5: RVV Tests (Spike)
# =============================================================================

# Phase 5 Spike tests: Single-core with RVV enabled
if(TARGET app AND SPIKE AND ENABLE_RVV AND NUM_HARTS EQUAL 1)

    # Test 1: RVV Detection on Spike
    add_test(
        NAME phase5_spike_rvv_detect
        COMMAND ${SPIKE} --isa=rv64gcv $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_spike_rvv_detect PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] RVV detection: PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] RVV detection: FAIL"
        TIMEOUT 30
        LABELS "phase5;spike;rvv;smoke"
    )

    # Test 2: RVV VLEN detection on Spike
    add_test(
        NAME phase5_spike_rvv_vlen
        COMMAND ${SPIKE} --isa=rv64gcv $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_spike_rvv_vlen PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[RVV\\] VLEN  = [0-9]+ bits"
        FAIL_REGULAR_EXPRESSION "\\[RVV\\] Not available"
        TIMEOUT 30
        LABELS "phase5;spike;rvv;detect"
    )

    # Test 3: Integer vector add on Spike
    add_test(
        NAME phase5_spike_rvv_vec_add_i32
        COMMAND ${SPIKE} --isa=rv64gcv $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_spike_rvv_vec_add_i32 PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Vec add \\(int32\\): PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] Vec add \\(int32\\): FAIL"
        TIMEOUT 30
        LABELS "phase5;spike;rvv;functional"
    )

    # Test 4: Vector memcpy on Spike
    add_test(
        NAME phase5_spike_rvv_memcpy
        COMMAND ${SPIKE} --isa=rv64gcv $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_spike_rvv_memcpy PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Vec memcpy: PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] Vec memcpy: FAIL"
        TIMEOUT 30
        LABELS "phase5;spike;rvv;functional"
    )

    # Test 5: Dot product on Spike
    add_test(
        NAME phase5_spike_rvv_dot_product
        COMMAND ${SPIKE} --isa=rv64gcv $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_spike_rvv_dot_product PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Dot product \\(float32\\): PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] Dot product \\(float32\\): FAIL"
        TIMEOUT 30
        LABELS "phase5;spike;rvv;functional"
    )

    # Test 6: SAXPY on Spike
    add_test(
        NAME phase5_spike_rvv_saxpy
        COMMAND ${SPIKE} --isa=rv64gcv $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_spike_rvv_saxpy PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] SAXPY \\(float32\\): PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] SAXPY \\(float32\\): FAIL"
        TIMEOUT 30
        LABELS "phase5;spike;rvv;functional"
    )

    # Test 7: Matrix multiply on Spike
    add_test(
        NAME phase5_spike_rvv_matmul
        COMMAND ${SPIKE} --isa=rv64gcv $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_spike_rvv_matmul PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[TEST\\] Matrix multiply \\(float32\\): PASS"
        FAIL_REGULAR_EXPRESSION "\\[TEST\\] Matrix multiply \\(float32\\): FAIL"
        TIMEOUT 30
        LABELS "phase5;spike;rvv;functional"
    )

    # Test 8: All Phase 5 tests pass on Spike (integration)
    add_test(
        NAME phase5_spike_rvv_complete
        COMMAND ${SPIKE} --isa=rv64gcv $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_spike_rvv_complete PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[RESULT\\] Phase 5 tests: 7/7 PASS"
        FAIL_REGULAR_EXPRESSION "FAIL"
        TIMEOUT 30
        LABELS "phase5;spike;rvv;integration"
    )

    # Test 9: Platform name on Spike
    add_test(
        NAME phase5_spike_rvv_platform
        COMMAND ${SPIKE} --isa=rv64gcv $<TARGET_FILE:app>
    )
    set_tests_properties(phase5_spike_rvv_platform PROPERTIES
        PASS_REGULAR_EXPRESSION "Platform: Spike"
        FAIL_REGULAR_EXPRESSION "ERROR|panic"
        TIMEOUT 30
        LABELS "phase5;spike;rvv;smoke"
    )

endif()

# =============================================================================
# Phase 6: gem5 Tests
# =============================================================================

# Phase 6 gem5 FS tests: Single-core full system mode
# gem5 FS reuses the same application binary as QEMU (UART I/O, M-mode CSRs).
# The binary is identical to QEMU build but uses gem5-specific linker script.
if(TARGET app AND GEM5_OPT AND PLATFORM STREQUAL "gem5" AND GEM5_MODE STREQUAL "fs" AND NUM_HARTS EQUAL 1 AND NOT ENABLE_RVV)

    # Test 1: Boot on gem5 FS mode (AtomicSimpleCPU)
    add_test(
        NAME phase6_gem5_fs_boot_hello
        COMMAND ${GEM5_OPT}
            ${GEM5_FS_CONFIG}
            --cpu-type=AtomicSimpleCPU
            --cmd=$<TARGET_FILE:app>
            --max-ticks=100000000
    )
    set_tests_properties(phase6_gem5_fs_boot_hello PROPERTIES
        PASS_REGULAR_EXPRESSION "Hello RISC-V"
        FAIL_REGULAR_EXPRESSION "panic|fatal|Error"
        TIMEOUT 120
        LABELS "phase6;gem5;fs;smoke"
    )

    # Test 2: CSR read - Hart ID on gem5
    add_test(
        NAME phase6_gem5_fs_csr_hartid
        COMMAND ${GEM5_OPT}
            ${GEM5_FS_CONFIG}
            --cpu-type=AtomicSimpleCPU
            --cmd=$<TARGET_FILE:app>
            --max-ticks=100000000
    )
    set_tests_properties(phase6_gem5_fs_csr_hartid PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[CSR\\] Hart ID: 0"
        FAIL_REGULAR_EXPRESSION "panic|fatal"
        TIMEOUT 120
        LABELS "phase6;gem5;fs;csr"
    )

    # Test 3: CSR read - mstatus on gem5
    add_test(
        NAME phase6_gem5_fs_csr_mstatus
        COMMAND ${GEM5_OPT}
            ${GEM5_FS_CONFIG}
            --cpu-type=AtomicSimpleCPU
            --cmd=$<TARGET_FILE:app>
            --max-ticks=100000000
    )
    set_tests_properties(phase6_gem5_fs_csr_mstatus PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[CSR\\] mstatus:"
        FAIL_REGULAR_EXPRESSION "panic|fatal"
        TIMEOUT 120
        LABELS "phase6;gem5;fs;csr"
    )

    # Test 4: All Phase 2 tests pass on gem5 FS mode (integration)
    add_test(
        NAME phase6_gem5_fs_complete
        COMMAND ${GEM5_OPT}
            ${GEM5_FS_CONFIG}
            --cpu-type=AtomicSimpleCPU
            --cmd=$<TARGET_FILE:app>
            --max-ticks=100000000
    )
    set_tests_properties(phase6_gem5_fs_complete PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[RESULT\\] Phase 2 tests: [0-9]+/[0-9]+ PASS"
        FAIL_REGULAR_EXPRESSION "0/[0-9]+ PASS"
        TIMEOUT 120
        LABELS "phase6;gem5;fs;integration"
    )

    # Test 5: Platform name reports "gem5"
    add_test(
        NAME phase6_gem5_fs_platform_name
        COMMAND ${GEM5_OPT}
            ${GEM5_FS_CONFIG}
            --cpu-type=AtomicSimpleCPU
            --cmd=$<TARGET_FILE:app>
            --max-ticks=100000000
    )
    set_tests_properties(phase6_gem5_fs_platform_name PROPERTIES
        PASS_REGULAR_EXPRESSION "Platform: gem5"
        FAIL_REGULAR_EXPRESSION "panic|fatal"
        TIMEOUT 120
        LABELS "phase6;gem5;fs;smoke"
    )

    # Test 6: Boot on gem5 FS with TimingSimpleCPU
    add_test(
        NAME phase6_gem5_fs_timing_boot
        COMMAND ${GEM5_OPT}
            ${GEM5_FS_CONFIG}
            --cpu-type=TimingSimpleCPU
            --cmd=$<TARGET_FILE:app>
            --max-ticks=500000000
    )
    set_tests_properties(phase6_gem5_fs_timing_boot PROPERTIES
        PASS_REGULAR_EXPRESSION "Hello RISC-V"
        FAIL_REGULAR_EXPRESSION "panic|fatal"
        TIMEOUT 180
        LABELS "phase6;gem5;fs;timing"
    )

    # Test 7: Boot on gem5 FS with MinorCPU (in-order pipeline)
    add_test(
        NAME phase6_gem5_fs_minor_boot
        COMMAND ${GEM5_OPT}
            ${GEM5_FS_CONFIG}
            --cpu-type=MinorCPU
            --cmd=$<TARGET_FILE:app>
            --max-ticks=500000000
    )
    set_tests_properties(phase6_gem5_fs_minor_boot PROPERTIES
        PASS_REGULAR_EXPRESSION "Hello RISC-V"
        FAIL_REGULAR_EXPRESSION "panic|fatal"
        TIMEOUT 300
        LABELS "phase6;gem5;fs;minor"
    )

    # Test 8: gem5 generates stats file
    # After simulation, gem5 writes m5out/stats.txt with performance data
    add_test(
        NAME phase6_gem5_fs_stats_generated
        COMMAND ${GEM5_OPT}
            ${GEM5_FS_CONFIG}
            --cpu-type=AtomicSimpleCPU
            --cmd=$<TARGET_FILE:app>
            --max-ticks=100000000
    )
    set_tests_properties(phase6_gem5_fs_stats_generated PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[gem5\\] Stats file:"
        TIMEOUT 120
        LABELS "phase6;gem5;fs;stats"
    )

    # Test 9: Compare cycle counts across CPU models (Atomic vs Timing)
    set(GEM5_COMPARE_WORK_DIR "${CMAKE_BINARY_DIR}/gem5_compare_test")
    add_test(
        NAME phase6_gem5_fs_cycle_compare
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/run-gem5-compare-test.sh
            ${GEM5_OPT}
            ${GEM5_FS_CONFIG}
            $<TARGET_FILE:app>
            ${CMAKE_SOURCE_DIR}/scripts/parse-gem5-stats.py
            ${GEM5_COMPARE_WORK_DIR}
    )
    set_tests_properties(phase6_gem5_fs_cycle_compare PROPERTIES
        PASS_REGULAR_EXPRESSION "CPI|cycle comparison test PASSED"
        TIMEOUT 600
        LABELS "phase6;gem5;fs;performance"
    )

endif()

# Phase 6 gem5 FS SMP tests: Multi-core full system mode
if(TARGET app AND GEM5_OPT AND PLATFORM STREQUAL "gem5" AND GEM5_MODE STREQUAL "fs" AND NUM_HARTS GREATER 1)

    # Test: SMP Boot on gem5
    add_test(
        NAME phase6_gem5_fs_smp_boot
        COMMAND ${GEM5_OPT}
            ${GEM5_FS_CONFIG}
            --cpu-type=AtomicSimpleCPU
            --num-cpus=${NUM_HARTS}
            --cmd=$<TARGET_FILE:app>
            --max-ticks=500000000
    )
    set_tests_properties(phase6_gem5_fs_smp_boot PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[SMP\\] All ${NUM_HARTS} harts online"
        FAIL_REGULAR_EXPRESSION "panic|fatal"
        TIMEOUT 300
        LABELS "phase6;gem5;fs;smp;smoke"
    )

    # Test: All Phase 4 SMP tests pass on gem5
    add_test(
        NAME phase6_gem5_fs_smp_complete
        COMMAND ${GEM5_OPT}
            ${GEM5_FS_CONFIG}
            --cpu-type=AtomicSimpleCPU
            --num-cpus=${NUM_HARTS}
            --cmd=$<TARGET_FILE:app>
            --max-ticks=500000000
    )
    set_tests_properties(phase6_gem5_fs_smp_complete PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[RESULT\\] Phase 4 tests: [0-9]+/[0-9]+ PASS"
        FAIL_REGULAR_EXPRESSION "0/[0-9]+ PASS"
        TIMEOUT 300
        LABELS "phase6;gem5;fs;smp;integration"
    )

endif()

# Phase 6 gem5 SE tests: Syscall emulation mode
if(TARGET app AND GEM5_OPT AND PLATFORM STREQUAL "gem5" AND GEM5_MODE STREQUAL "se" AND NUM_HARTS EQUAL 1 AND NOT ENABLE_RVV)

    # Test 1: Boot on gem5 SE mode
    add_test(
        NAME phase6_gem5_se_boot_hello
        COMMAND ${GEM5_OPT}
            ${GEM5_SE_CONFIG}
            --cpu-type=AtomicSimpleCPU
            --cmd=$<TARGET_FILE:app>
            --max-ticks=100000000
    )
    set_tests_properties(phase6_gem5_se_boot_hello PROPERTIES
        PASS_REGULAR_EXPRESSION "Hello RISC-V"
        FAIL_REGULAR_EXPRESSION "panic|fatal|Error"
        TIMEOUT 120
        LABELS "phase6;gem5;se;smoke"
    )

    # Test 2: All Phase 2 tests pass on gem5 SE (integration)
    add_test(
        NAME phase6_gem5_se_complete
        COMMAND ${GEM5_OPT}
            ${GEM5_SE_CONFIG}
            --cpu-type=AtomicSimpleCPU
            --cmd=$<TARGET_FILE:app>
            --max-ticks=100000000
    )
    set_tests_properties(phase6_gem5_se_complete PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[RESULT\\] Phase 2 tests: [0-9]+/[0-9]+ PASS"
        FAIL_REGULAR_EXPRESSION "0/[0-9]+ PASS"
        TIMEOUT 120
        LABELS "phase6;gem5;se;integration"
    )

    # Test 3: Platform name in SE mode
    add_test(
        NAME phase6_gem5_se_platform_name
        COMMAND ${GEM5_OPT}
            ${GEM5_SE_CONFIG}
            --cpu-type=AtomicSimpleCPU
            --cmd=$<TARGET_FILE:app>
            --max-ticks=100000000
    )
    set_tests_properties(phase6_gem5_se_platform_name PROPERTIES
        PASS_REGULAR_EXPRESSION "Platform: gem5"
        FAIL_REGULAR_EXPRESSION "panic|fatal"
        TIMEOUT 120
        LABELS "phase6;gem5;se;smoke"
    )

endif()

# =============================================================================
# Phase 7: Renode Tests
# =============================================================================

# Phase 7 Renode tests: Only for single-core renode builds without RVV
# Uses run-renode-test.sh to capture UART output (Renode writes to file, not stdout)
if(TARGET app AND RENODE AND PLATFORM STREQUAL "renode" AND NUM_HARTS EQUAL 1 AND NOT ENABLE_RVV)

    # Test 1: Boot and print "Hello RISC-V"
    add_test(
        NAME phase7_renode_boot_hello
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/run-renode-test.sh
            ${RENODE}
            $<TARGET_FILE:app>
            ${CMAKE_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(phase7_renode_boot_hello PROPERTIES
        PASS_REGULAR_EXPRESSION "Hello RISC-V"
        FAIL_REGULAR_EXPRESSION "ERROR|FAIL|panic"
        TIMEOUT 30
        LABELS "phase7;renode;smoke"
    )

    # Test 2: Platform name reports "Renode"
    add_test(
        NAME phase7_renode_platform_name
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/run-renode-test.sh
            ${RENODE}
            $<TARGET_FILE:app>
            ${CMAKE_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(phase7_renode_platform_name PROPERTIES
        PASS_REGULAR_EXPRESSION "Platform: Renode"
        FAIL_REGULAR_EXPRESSION "panic|fatal"
        TIMEOUT 30
        LABELS "phase7;renode;smoke"
    )

    # Test 3: All Phase 2 tests pass (integration)
    add_test(
        NAME phase7_renode_complete
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/run-renode-test.sh
            ${RENODE}
            $<TARGET_FILE:app>
            ${CMAKE_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(phase7_renode_complete PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[RESULT\\].*5/5.*PASS"
        FAIL_REGULAR_EXPRESSION "ERROR|FAIL"
        TIMEOUT 60
        LABELS "phase7;renode;functional"
    )

endif()

# Phase 7 Renode SMP tests: 4-hart builds without RVV
# NOTE: Renode SMP platform (riscv_virt_smp.repl) has CLINT IRQ mapping issues -
# CoreLevelInterruptor only exposes [0,1]. SMP tests disabled until platform fixed.
if(FALSE AND TARGET app AND RENODE AND PLATFORM STREQUAL "renode" AND NUM_HARTS EQUAL 4 AND NOT ENABLE_RVV)

    # Test: SMP boot on Renode
    add_test(
        NAME phase7_renode_smp_boot
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/run-renode-test.sh
            ${RENODE}
            $<TARGET_FILE:app>
            ${CMAKE_SOURCE_DIR}
            platforms/renode/configs/run_smp.resc
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(phase7_renode_smp_boot PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[SMP\\] All 4 harts online"
        FAIL_REGULAR_EXPRESSION "ERROR|FAIL|panic"
        TIMEOUT 60
        LABELS "phase7;renode;smp;smoke"
    )

    # Test: All Phase 4 SMP tests pass on Renode
    add_test(
        NAME phase7_renode_smp_complete
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/run-renode-test.sh
            ${RENODE}
            $<TARGET_FILE:app>
            ${CMAKE_SOURCE_DIR}
            platforms/renode/configs/run_smp.resc
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(phase7_renode_smp_complete PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[RESULT\\] Phase 4 tests: [0-9]+/[0-9]+ PASS"
        FAIL_REGULAR_EXPRESSION "ERROR|FAIL"
        TIMEOUT 90
        LABELS "phase7;renode;smp;functional"
    )

endif()

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
