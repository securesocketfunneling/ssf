# This CMakeLists wraps the Boost build scripts in order to allow compilation
# in a separate build tree.
#
# Expected CMake variables are:
#
#  MODULES_SEARCH_PATH          : Search path for cmake-build-system modules
#  BOOST_SOURCE_DIR <path>      : The absolute path to the Boost's source tree
#  BOOST_NO_COMPONENTS          : Boolean flag if no components has to be built
#  BOOST_COMPONENTS <components>: List of components to build
#  BOOST_EXTRA_FLAGS <flags>    : List of extra raw flags to provide to package
#                                 low level build engine
#  BOOST_FLAGS                  : List of flags to configure Boost's build
#    RUNTIME_STATIC     : Flag to request static link with runtime
#    STATIC             : Flag to request compilation of a staic library
#
cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)

include(ExternalPackageHelpers)
include(HelpersArguments)
include(EnhancedList)
include(boost_build)

# ==============================================================================
# ======================= D e f i n e   f u n c t i o n s ======================
# ==============================================================================

# ------------------------------------------------------------------------------
# Prepare the build context for the BOOST package
macro(_boost_setup_build_context)
  # Parse given flags
  if(BOOST_FLAGS)
    parse_arguments("BOOST"
      "STATIC;RUNTIME_STATIC"
      ""
      ""
      ""
      ${BOOST_FLAGS}
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

  if(BOOST_STATIC)
    list(APPEND _base_build_flags STATIC)
    list(APPEND _build_dir_suffix ST)
  else()
    list(APPEND _build_dir_suffix SH)
  endif()

  if(BOOST_RUNTIME_STATIC)
    list(APPEND _base_build_flags RUNTIME_STATIC)
    list(APPEND _build_dir_suffix RS)
  endif()

  list_join(_build_dir_suffix "_" _build_dir_suffix)
  string(TOLOWER "${_build_dir_suffix}" _build_dir_suffix)

  # Set-up some directories
  set(BOOST_BUILD_BASE_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/build/${_build_dir_suffix}"
  )

  set(BOOST_INSTALL_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/stage/${_build_dir_suffix}"
  )

  external_setup_build_context(Boost "${_build_dir_suffix}")
endmacro()


# ==============================================================================
# ==================== C o r e   I m p l e m e n t a t i o n ===================
# ==============================================================================

_boost_setup_build_context()

if(Boost_DEBUG)
  external_debug("Build context status:")
  foreach(_var NEED_BUILD NO_COMPONENTS EXTRA_FLAGS COMPONENTS
               STATIC RUNTIME_STATIC INSTALL_DIR BUILD_BASE_DIR)
    external_debug("  BOOST_${_var} : ${BOOST_${_var}}")
  endforeach()
endif()

if(BOOST_NEED_BUILD)
  # Determine what modes the Boost has to be compiled in
  if(CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(_build_modes "dbg" "rel")
  elseif(CMAKE_BUILD_TYPE MATCHES "^[Dd][Ee][Bb][Uu][Gg]$")
    set(_build_modes "dbg")
  else()
    set(_build_modes "rel")
  endif()

  file(MAKE_DIRECTORY "${BOOST_INSTALL_DIR}")
  foreach(_mode IN LISTS _build_modes)
    # Set build flags
    unset(_build_flags)
    unset(_lib_mode)

    if(BOOST_NO_COMPONENTS)
      list(APPEND _build_flags "NO_LIB")
    endif()

    if(_base_build_flags)
      list(APPEND _build_flags "${_base_build_flags}")
      list(APPEND _lib_mode  "${_base_build_flags}")
    else()
      list(APPEND _lib_mode  "SHARED")
    endif()

    if(_mode STREQUAL "dbg")
      list(APPEND _build_flags "DEBUG")
      list(APPEND _lib_mode  "DEBUG")
    else()
      list(APPEND _lib_mode  "RELEASE")
    endif()

    if(Boost_DEBUG)
      external_debug("_build_flags: '${_build_flags}'")
    endif()

    # Start the build session
    list_join(_lib_mode "/" _lib_mode)
    external_log("Building Libraries in mode ${_lib_mode}")
    boost_build(
      NO_LOG
      "${_build_flags}"
      SOURCE_DIR  "${BOOST_SOURCE_DIR}"
      INSTALL_DIR "${BOOST_INSTALL_DIR}"
      BUILD_DIR   "${BOOST_BUILD_BASE_DIR}/${_mode}"
      COMPONENTS   ${BOOST_COMPONENTS}
      EXTRA_FLAGS "${BOOST_EXTRA_FLAGS}"
    )
  endforeach()

  external_teardown_build_context(Boost "${_build_dir_suffix}")

endif()
