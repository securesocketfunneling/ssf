# Find and extract the Google Test package

cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)

include(FindPackageHandleStandardArgs)
include(EnhancedList)
include(ExternalPackageHelpers)

# ==============================================================================
# ======================= D e f i n e   f u n c t i o n s ======================
# ==============================================================================

# ------------------------------------------------------------------------------
# Unpack GTest package
function(gtest_unpack_archive)

  # Extract GTest archive
  external_search_source_path(
    NAME "gtest"
    VAR "GTEST_UNPACK_DIR"
    CLUES "README"
  )
  
  # Set GTEST_ROOT_DIR
  set(
    GTEST_ROOT_DIR
    "${GTEST_UNPACK_DIR}"
    PARENT_SCOPE
  )
  
endfunction()