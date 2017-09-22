# This CMakeLists wraps the OpenSSL build scripts in order to allow compilation
# in a separate build tree.
#
# Expected CMake variables are:
#
#  MODULES_SEARCH_PATH        : Search path for cmake-build-system modules
#  OPENSSL_SOURCE_DIR <path>  : The absolute path to the OpenSSL's source tree
#  OPENSSL_EXTRA_FLAGS        : Extra flags to provide to OpenSSL's
#                               configuration script
#  OPENSSL_FLAGS              : List of flags to configure OpenSSL's build
#    RUNTIME_STATIC : Flag to request static link with run-time
#
cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)

include(ExternalPackageHelpers)
include(HelpersArguments)
include(EnhancedList)
include(openssl_build)

# ==============================================================================
# ======================= D e f i n e   f u n c t i o n s ======================
# ==============================================================================

# ------------------------------------------------------------------------------
# Prepare the build context for the OpenSSL package
macro(_openssl_setup_build_context)
  # Parse given flags
  if(OPENSSL_FLAGS)
    parse_arguments("OPENSSL"
      "RUNTIME_STATIC;STATIC"
      ""
      ""
      ""
      ${OPENSSL_FLAGS}
    )
  endif()

  # Prepare build base flags and build directory suffix
  unset(_base_build_flags)
  unset(_build_dir_suffix)

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(APPEND _build_dir_suffix x64)
  else()
    list(APPEND _build_dir_suffix x86)
  endif()

  if(OPENSSL_STATIC)
    list(APPEND _base_build_flags STATIC)
    list(APPEND _build_dir_suffix st)
  else()
    list(APPEND _build_dir_suffix sh)
  endif()

  if(OPENSSL_RUNTIME_STATIC)
    list(APPEND _base_build_flags RUNTIME_STATIC)
    list(APPEND _build_dir_suffix rs)
  endif()

  list_join(_build_dir_suffix "_" _build_dir_suffix)
  string(TOLOWER "${_build_dir_suffix}" _build_dir_suffix)

  # Set-up some directories
  set(OPENSSL_BUILD_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/build/${_build_dir_suffix}"
  )

  set(OPENSSL_INSTALL_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/stage/${_build_dir_suffix}"
  )

  external_setup_build_context(OpenSSL "${_build_dir_suffix}")
endmacro()

# ------------------------------------------------------------------------------
# ==============================================================================
# ==================== C o r e   I m p l e m e n t a t i o n ===================
# ==============================================================================

# Prepare build context
_openssl_setup_build_context()

# Check if package has already been built
if(OPENSSL_NEED_BUILD)
  if(OPENSSL_RUNTIME_STATIC)
    external_log("[Embedding static runtime]")
  endif()

  # Launch build process
  openssl_build(
    ${_base_build_flags}
    SOURCE_DIR  "${OPENSSL_SOURCE_DIR}"
    INSTALL_DIR "${OPENSSL_INSTALL_DIR}"
    BUILD_DIR   "${OPENSSL_BUILD_DIR}"
    EXTRA_FLAGS "${OPENSSL_EXTRA_FLAGS}"
  )

  external_teardown_build_context(OpenSSL "${_build_dir_suffix}")
endif()

