# =============================================================================
# Code Formatting and Linting
# =============================================================================
# This module provides targets for code formatting and static analysis.
# =============================================================================

# Find clang-format
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    message(STATUS "Found clang-format: ${CLANG_FORMAT}")
    
    # Get version
    execute_process(
        COMMAND ${CLANG_FORMAT} --version
        OUTPUT_VARIABLE CLANG_FORMAT_VERSION_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "clang-format version: ${CLANG_FORMAT_VERSION_OUTPUT}")
endif()

# Find cppcheck
find_program(CPPCHECK cppcheck)
if(CPPCHECK)
    message(STATUS "Found cppcheck: ${CPPCHECK}")
    
    # Get version
    execute_process(
        COMMAND ${CPPCHECK} --version
        OUTPUT_VARIABLE CPPCHECK_VERSION_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "cppcheck version: ${CPPCHECK_VERSION_OUTPUT}")
endif()

# Find clang-tidy
find_program(CLANG_TIDY clang-tidy)
if(CLANG_TIDY)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY}")
endif()
