# Find and Build the OpenSSL package

cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)

include(FindPackageHandleStandardArgs)
include(EnhancedList)
include(ExternalPackageHelpers)

# ==============================================================================
# ======================= D e f i n e   f u n c t i o n s ======================
# ==============================================================================

# ------------------------------------------------------------------------------
# Prepare the build context for the OpenSSL package
macro(_openssl_setup_build_context)
  # Parse the compilation flags
  external_parse_arguments(OpenSSL
    "RUNTIME_STATIC;STATIC"
    "SOURCE_DIR"
    "EXTRA_FLAGS"
    ""
  )

  if(OpenSSL_FIND_COMPONENTS)
    external_error(
      "Components list not supported for this package:\n"
      "${OpenSSL_FIND_COMPONENTS}"
    )
  endif()

  if(OPENSSL_RUNTIME_STATIC AND NOT CMAKE_HOST_WIN32)
    external_error("Invalid platform option 'RUNTIME_STATIC'")
  endif()

  # Prepare build directory suffix
  unset(_build_dir_suffix)

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(APPEND _build_dir_suffix x64)
  else()
    list(APPEND _build_dir_suffix x86)
  endif()

  if(OPENSSL_STATIC)
    list(APPEND _build_dir_suffix st)
    list(APPEND OPENSSL_FLAGS STATIC)
    set(OPENSSL_STATIC "${OPENSSL_STATIC}" PARENT_SCOPE)
  else()
    list(APPEND _build_dir_suffix sh)
  endif()

  if(OPENSSL_RUNTIME_STATIC)
    list(APPEND _build_dir_suffix rs)
    list(APPEND OPENSSL_FLAGS RUNTIME_STATIC)
    set(OPENSSL_RUNTIME_STATIC "${OPENSSL_RUNTIME_STATIC}" PARENT_SCOPE)
  endif()

  string(REPLACE ";" "_" _build_dir_suffix "${_build_dir_suffix}")
  string(TOLOWER "${_build_dir_suffix}" _build_dir_suffix)
endmacro()

# ------------------------------------------------------------------------------
# Build OpenSSL package
function(openssl_build_package)
  # Prepare build context
  _openssl_setup_build_context()

  # Set-up OpenSSL's source directory
  unset(_source_fields)
  if(OPENSSL_SOURCE_DIR)
    list(APPEND _source_fields SOURCE_DIR "${OPENSSL_SOURCE_DIR}")
  endif()
  list(APPEND _source_fields SOURCE_SEARCH_CLUES "openssl.spec")

  # Build/Update OpenSSL package
  externals_build(OpenSSL ${_source_fields}
    FLAGS       "${OPENSSL_FLAGS}"
    EXTRA_FLAGS "${OPENSSL_EXTRA_FLAGS}"
  )

  # Set-up OpenSSL's install directory
  set(OpenSSL_ROOT_DIR
    "${OPENSSL_BUILD_DIR}/stage/${_build_dir_suffix}"
    PARENT_SCOPE
  )

endfunction()


# ------------------------------------------------------------------------------
macro(openssl_setup)
  find_package_handle_standard_args(OpenSSL
    REQUIRED_VARS
      OpenSSL_VERSION
      OpenSSL_ROOT_DIR
      OpenSSL_INCLUDE_DIRS
      OpenSSL_LIBRARIES
    VERSION_VAR
      OpenSSL_VERSION
    HANDLE_COMPONENTS
  )
  set(OpenSSL_FOUND "${OPENSSL_FOUND}" PARENT_SCOPE)
  if(OPENSSL_FOUND)
    foreach(_var ROOT_DIR INCLUDE_DIRS LIBRARIES VERSION FOUND)
      set(OpenSSL_${_var} "${OpenSSL_${_var}}" PARENT_SCOPE)
      mark_as_advanced(${_var})
    endforeach()
  endif()
endmacro()

# ------------------------------------------------------------------------------
function(openssl_from_hex HEX DEC)
  string(TOUPPER "${HEX}" HEX)
  set(_res 0)
  string(LENGTH "${HEX}" _strlen)

  while (_strlen GREATER 0)
    math(EXPR _res "${_res} * 16")
    string(SUBSTRING "${HEX}" 0 1 NIBBLE)
    string(SUBSTRING "${HEX}" 1 -1 HEX)
    if (NIBBLE STREQUAL "A")
      math(EXPR _res "${_res} + 10")
    elseif (NIBBLE STREQUAL "B")
      math(EXPR _res "${_res} + 11")
    elseif (NIBBLE STREQUAL "C")
      math(EXPR _res "${_res} + 12")
    elseif (NIBBLE STREQUAL "D")
      math(EXPR _res "${_res} + 13")
    elseif (NIBBLE STREQUAL "E")
      math(EXPR _res "${_res} + 14")
    elseif (NIBBLE STREQUAL "F")
      math(EXPR _res "${_res} + 15")
    else()
      math(EXPR _res "${_res} + ${NIBBLE}")
    endif()

    string(LENGTH "${HEX}" _strlen)
  endwhile()

  set(${DEC} ${_res} PARENT_SCOPE)
endfunction()

# ------------------------------------------------------------------------------
function(openssl_find_version version_file_header version_var)
  # For version string from the given header file
  file(STRINGS
    "${version_file_header}"
    _version_str
    REGEX "^#[\t ]*define[\t ]+OPENSSL_VERSION_NUMBER[\t ]+0x([0-9a-fA-F])+.*"
  )
  string(TOUPPER "${_version_str}" _version_str)

  # The version number is encoded as 0xMNNFFPPS: major minor patch tweak status
  # The status gives if this is a developer or prerelease and is ignored here.
  # Major, minor, and patch directly translate into the version numbers shown in
  # the string. The tweak field translates to the single character suffix that
  # indicates the bug fix state, which 00 -> nothing, 01 -> a, 02 -> b and so
  # on.
  set(replace_pattern
    "^.*OPENSSL_VERSION_NUMBER[\t ]+0X" # PREFIX
      "([0-9A-F])"                      # MAJOR
      "([0-9A-F][0-9A-F])"              # MINOR
      "([0-9A-F][0-9A-F])"              # PATCH
      "([0-9A-F][0-9A-F])"              # TWEAK
      "[0-9A-F]"                        # STATE
      ".*$"                             # SUFFIX
  )
  list_join(replace_pattern "" replace_pattern)
  string(REGEX REPLACE
    "${replace_pattern}"
    "\\1;\\2;\\3;\\4"
    _version_str
    "${_version_str}"
  )


  unset(_version)
  foreach(_field IN LISTS _version_str)
    openssl_from_hex("${_field}" _field)
    if(_version)
      set(_version "${_version}.")
    endif()
    set(_version "${_version}${_field}")
  endforeach()

#  if(_tweak GREATER 0)
#    # 96 is the ASCII code of 'a' minus 1
#    math(EXPR _tweak "${_tweak} + 96")
#    # Once anyone knows how OpenSSL would call the patch versions beyond 'z'
#    # this should be updated to handle that, too. This has not happened yet
#    # so it is simply ignored here for now.
#    string(ASCII "${_VERSION_PATCH}" _VERSION_PATCH)
#    set(${openssl_revision_var} "${${openssl_revision_var}}${_VERSION_PATCH}")
#  endif()
  

  # Export variables to parent
  set(${version_var} "${_version}" PARENT_SCOPE)
endfunction()

# ------------------------------------------------------------------------------
function(openssl_find_package)

  # Look for OpenSSL in DEBUG and RELEASE
  unset(OpenSSL_INCLUDE_DIRS CACHE)
  unset(OpenSSL_LIBRARY CACHE)
  unset(OpenSSL_VERSION CACHE)

  # Check first for OpenSSL's include directory
  find_path(OpenSSL_INCLUDE_DIRS
    NAMES         "openssl/opensslv.h"
    PATHS         "${OpenSSL_ROOT_DIR}"
    PATH_SUFFIXES "include"
    NO_DEFAULT_PATH
  )

  if(OpenSSL_INCLUDE_DIRS)
    # Extract OpenSSL's version
    openssl_find_version(
      "${OpenSSL_INCLUDE_DIRS}/openssl/opensslv.h"
      OpenSSL_VERSION
    )

    if(NOT DEFINED os_name)
      string(TOLOWER "${CMAKE_SYSTEM_NAME}" os_name)
    endif()

    if(os_name STREQUAL "windows")
      if(OPENSSL_STATIC)
        set(_lib_pattern "${OpenSSL_ROOT_DIR}/lib/*.lib")
      else()
        set(_lib_pattern "${OpenSSL_ROOT_DIR}/dll/*.lib")
      endif()
    elseif(os_name STREQUAL "darwin")
      if(OPENSSL_STATIC)
        set(_lib_pattern "${OpenSSL_ROOT_DIR}/lib/lib*.a")
      else()
        set(_lib_pattern "${OpenSSL_ROOT_DIR}/lib/lib*.dylib")
      endif()
    else()
      if(OPENSSL_STATIC)
        set(_lib_pattern "${OpenSSL_ROOT_DIR}/lib/libssl.a")
        list(APPEND _lib_pattern "${OpenSSL_ROOT_DIR}/lib/lib*.a")
      else()
        set(_lib_pattern "${OpenSSL_ROOT_DIR}/lib/lib*.so")
      endif()
    endif()

    # Find OpenSSL's Libraries
    file(GLOB_RECURSE OpenSSL_LIBRARIES ${_lib_pattern})
  endif()

  # Set-up all OpenSSL's global variables
  openssl_setup()
endfunction()

## ==============================================================================
## ==================== C o r e   I m p l e m e n t a t i o n ===================
## ==============================================================================

# Build package
openssl_build_package()

# Find requested package from installation directory
openssl_find_package()

