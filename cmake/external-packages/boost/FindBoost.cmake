# Module for locating the Boost libraries.

cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)

include(FindPackageHandleStandardArgs)
include(EnhancedList)
include(ExternalPackageHelpers)

# ==============================================================================
# ======================= D e f i n e   f u n c t i o n s ======================
# ==============================================================================

# ------------------------------------------------------------------------------
# Prepare the build context for the Boost package
macro(_boost_setup_build_context)
  # Parse arguments for Boost compilation flags
  external_parse_arguments(Boost
    "STATIC;RUNTIME_STATIC"
    "SOURCE_DIR"
    "EXTRA_FLAGS"
    ""
  )

  if(BOOST_RUNTIME_STATIC AND NOT BOOST_STATIC)
    external_error(
      "Boost's Option 'RUNTIME_STATIC' cannot be used with shared mode"
    )
  endif()

  # Prepare build directory suffix
  unset(_build_dir_suffix)

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(APPEND _build_dir_suffix x64)
  else()
    list(APPEND _build_dir_suffix x86)
  endif()

  if(BOOST_STATIC)
    list(APPEND _build_dir_suffix ST)
    list(APPEND BOOST_FLAGS STATIC)
  else()
    list(APPEND _build_dir_suffix SH)
  endif()

  if(BOOST_RUNTIME_STATIC)
    list(APPEND _build_dir_suffix RS)
    list(APPEND BOOST_FLAGS RUNTIME_STATIC)
  endif()

  string(REPLACE ";" "_" _build_dir_suffix "${_build_dir_suffix}")
  string(TOLOWER "${_build_dir_suffix}" _build_dir_suffix)
endmacro()

# ------------------------------------------------------------------------------
# Ensure all the components defined in Boost_FIND_COMPONENTS are built
function(boost_build_components)
  _boost_setup_build_context()

  # Set-up Boost's source directory
  unset(_source_fields)
  if(BOOST_SOURCE_DIR)
    list(APPEND _source_fields SOURCE_DIR "${BOOST_SOURCE_DIR}")
  endif()
  list(APPEND _source_fields SOURCE_SEARCH_CLUES "boostcpp.jam")

  # Build/Update Boost package
  if(BOOST_NO_COMPONENTS)
    set(_components NO_COMPONENTS)
  else()
    set(_components COMPONENTS ${Boost_FIND_COMPONENTS})
  endif()

  externals_build(Boost
    ${_source_fields}
    ${_components}
    FLAGS      "${BOOST_FLAGS}"
    EXTRA_FLAGS "${BOOST_EXTRA_FLAGS}"
  )

  # Set-up Boost's variables
  set(BOOST_NO_COMPONENTS "${BOOST_NO_COMPONENTS}" PARENT_SCOPE)

  set(BOOST_LIBRARYDIR
    "${BOOST_BUILD_DIR}/stage/${_build_dir_suffix}/lib"
    PARENT_SCOPE
  )

  set(BOOST_INCLUDEDIR "${BOOST_SOURCE_DIR}" PARENT_SCOPE)

endfunction()

# ------------------------------------------------------------------------------
macro(_boost_find_version)
  if(BOOST_INCLUDEDIR AND EXISTS "${BOOST_INCLUDEDIR}/boost/version.hpp")
    file(STRINGS
      "${BOOST_INCLUDEDIR}/boost/version.hpp" _boost_version_lines
      REGEX "#define BOOST(_LIB)?_VERSION "
    )
    foreach(_line IN LISTS _boost_version_lines)
      if(_line MATCHES ".*#define BOOST_VERSION ([0-9]+).*")
        set(BOOST_VERSION "${CMAKE_MATCH_1}")
      elseif(_line MATCHES ".*#define BOOST_LIB_VERSION \"([0-9_]+)\".*")
        set(Boost_LIB_VERSION "${CMAKE_MATCH_1}")
      endif()
    endforeach()

    math(EXPR Boost_MAJOR_VERSION "${BOOST_VERSION} / 100000")
    math(EXPR Boost_MINOR_VERSION "${BOOST_VERSION} / 100 % 1000")
    math(EXPR Boost_SUBMINOR_VERSION "${BOOST_VERSION} % 100")
    set(Boost_VERSION
      "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}"
    )
  else()
    external_error("No version header file in Boost's source tree")
  endif()
endmacro()

# ------------------------------------------------------------------------------
function(_boost_find)
  # Extract version of the compiled Boost
  _boost_find_version()
  foreach(_var "" _LIB _MAJOR _MINOR _SUBMINOR)
    set(Boost${_var}_VERSION "${Boost${_var}_VERSION}" PARENT_SCOPE)
    if(Boost_DEBUG)
      external_debug("  - Boost${_var}_VERSION: '${Boost${_var}_VERSION}'")
    endif()
  endforeach()
  set(Boost_INCLUDE_DIR "${BOOST_INCLUDEDIR}" PARENT_SCOPE)
  set(Boost_INCLUDE_DIRS "${BOOST_INCLUDEDIR}" PARENT_SCOPE)

  if(BOOST_NO_COMPONENTS)
    # No components to find ...
    external_debug("Build WITH NO components requested")
    return()
  endif()

  # Set-up reg-ex parser for Boost's library name
  set(_regex "(lib|)boost")                        # Library prefix
  set(_regex "${_regex}_([^-]+)")                  # Component name
  set(_regex "${_regex}(-.+|)")                    # Optional compiler's ID
  set(_regex "${_regex}(-mt)")                     # Multi-thread tag
  set(_regex "${_regex}(-s?g?d?|)")                # ABI tag
  set(_regex "${_regex}(-${Boost_LIB_VERSION}|)")  # Optional Version
  if(CMAKE_HOST_WIN32 AND NOT CYGWIN)
    set(_regex "${_regex}\\.(lib)")                # Win32 Library extension
  elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    set(_regex "${_regex}\\.(dylib|a)")            # Darwin Library extensions
  else()
    set(_regex "${_regex}\\.(so|a)")               # Linux Library extensions
  endif()

  # Look-up for all available (i.e. compiled) libraries
  file(GLOB _libraries RELATIVE "${BOOST_LIBRARYDIR}" "${BOOST_LIBRARYDIR}/*")
  set(Boost_LIBRARY_DIR "${BOOST_LIBRARYDIR}" PARENT_SCOPE)
  set(Boost_LIBRARY_DIRS "${BOOST_LIBRARYDIR}" PARENT_SCOPE)
  foreach(_lib IN LISTS _libraries)
    if("${_lib}" MATCHES "${_regex}")
      set(_component "${CMAKE_MATCH_2}")
      set(_abi_tag "${CMAKE_MATCH_5}")
      if("${Boost_FIND_COMPONENTS}" MATCHES "(.*;|)${_component}(;.*|)")
        if("${_abi_tag}" MATCHES "^-s?g?d$")
          set(BOOST_${_component}_LIBRARY_DEBUG "${BOOST_LIBRARYDIR}/${_lib}")
        else()
          set(BOOST_${_component}_LIBRARY_RELEASE "${BOOST_LIBRARYDIR}/${_lib}")
        endif()
      endif()
    endif()
  endforeach()

  unset(Boost_LIBRARIES)
  foreach(_component IN LISTS Boost_FIND_COMPONENTS)
    if(BOOST_${_component}_LIBRARY_RELEASE OR BOOST_${_component}_LIBRARY_DEBUG)
      set(Boost_${_component}_FOUND TRUE)
      if(NOT BOOST_${_component}_LIBRARY_DEBUG)
        list(APPEND Boost_LIBRARIES "${BOOST_${_component}_LIBRARY_RELEASE}")
        set(Boost_${_component}_LIBRARY
          "${BOOST_${_component}_LIBRARY_RELEASE}"
          PARENT_SCOPE
        )
      elseif(NOT BOOST_${_component}_LIBRARY_RELEASE)
        list(APPEND Boost_LIBRARIES "${BOOST_${_component}_LIBRARY_DEBUG}")
        set(Boost_${_component}_LIBRARY
          "${BOOST_${_component}_LIBRARY_DEBUG}"
          PARENT_SCOPE
        )
      else()
        list(APPEND Boost_LIBRARIES
          optimized "${BOOST_${_component}_LIBRARY_RELEASE}"
          debug     "${BOOST_${_component}_LIBRARY_DEBUG}"
        )
        set(Boost_${_component}_LIBRARY
          optimized "${BOOST_${_component}_LIBRARY_RELEASE}"
          debug     "${BOOST_${_component}_LIBRARY_DEBUG}"
          PARENT_SCOPE
        )
      endif()
      
      foreach(_var _RELEASE _DEBUG)
        if(Boost_${_component}_LIBRARY${_var})
          set(Boost_${_component}_LIBRARY${_var}
            "${BOOST_${_component}_LIBRARY_DEBUG}"
            PARENT_SCOPE
          )
        endif()
      endforeach()
    else()
      set(Boost_${_component}_FOUND "${_component}-NOTFOUND")
    endif()
    set(Boost_${_component}_FOUND "${Boost_${_component}_FOUND}" PARENT_SCOPE)
  endforeach()
  set(Boost_LIBRARIES "${Boost_LIBRARIES}" PARENT_SCOPE)
endfunction()

# ------------------------------------------------------------------------------
# Set-up BOOST variables
macro(_boost_setup_vars)
  if(Boost_DEBUG)
    foreach(_var INCLUDE_DIR INCLUDE_DIRS LIBRARY_DIRS LIBRARIES)
      external_debug("  Boost_${_var}:")
      foreach(_item IN LISTS Boost_${_var})
        external_debug("    '${_item}'")
      endforeach()
    endforeach()
  endif()

  unset(_lib_vars)
  if(NOT BOOST_NO_COMPONENTS)
    set(_lib_vars
      Boost_LIBRARIES
      Boost_LIBRARY_DIR
      Boost_LIBRARY_DIRS
    )
  endif()

  find_package_handle_standard_args(Boost
    FOUND_VAR
      Boost_FOUND
    REQUIRED_VARS
      Boost_VERSION
      ${_lib_vars}
      Boost_INCLUDE_DIR
      Boost_INCLUDE_DIRS
    VERSION_VAR
      Boost_VERSION
    HANDLE_COMPONENTS
  )
endmacro()

# ==============================================================================
# ==================== C o r e   I m p l e m e n t a t i o n ===================
# ==============================================================================

# Build missing components
set(PKG_NAME Boost)
boost_build_components()

if(Boost_DEBUG)
  foreach(_var BOOST_LIBRARYDIR BOOST_INCLUDEDIR)
    external_debug(" ${_var} = '${${_var}}'")
  endforeach()
endif()

# Find package's libraries
_boost_find()

# Set-up package's variables
_boost_setup_vars()
