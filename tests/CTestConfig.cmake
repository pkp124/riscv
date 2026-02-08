# =============================================================================
# CTest Dashboard Configuration
# =============================================================================

set(CTEST_PROJECT_NAME "RISC-V-Bare-Metal")
set(CTEST_NIGHTLY_START_TIME "00:00:00 UTC")

set(CTEST_DROP_METHOD "http")
set(CTEST_DROP_SITE "")
set(CTEST_DROP_LOCATION "")
set(CTEST_DROP_SITE_CDASH FALSE)

# Test timeout
set(CTEST_TEST_TIMEOUT 60)
