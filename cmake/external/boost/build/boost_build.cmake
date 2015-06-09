# This CMAKE script should be included from parent wishing to build BOOST
# libraries.
#
# Valid options are:
#     DEBUG                  # Flag to compile library in debug mode
#     STATIC                 # Flag to compile library in static mode
#     RUNTIME_STATIC         # Flag to link with platform's static runtime
#     SOURCE_DIR <path>      # Path to package's source tree
#     INSTALL_DIR <path>     # Destination where to store built libraries
#     BUILD_DIR <path>       # Destination where to build libraries
#     COMPONENTS <names>     # List of names of the components to build
#     EXTRA_FLAGS <flags>    # List of raw flags to provide to boost's scripts
#
# Example:
#   include(boost_build)
#
#   boost_build(
#     SOURCE_DIR      "path/to/the/package/source/tree"
#     COMPONENTS
#       chrono
#       filesystem
#       iostreams
#     EXTRA_FLAGS
#       -sNO_BZIP2=1
#   )
#
if(__H_BOOST_BUILD_INCLUDED)
  return()
endif()
set(__H_BOOST_BUILD_INCLUDED TRUE)

cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)

include(ExternalPackageHelpers)
include(HelpersArguments)
include(SystemTools)
include(ProcessorCount)

# ==============================================================================
# ======================= D e f i n e   f u n c t i o n s ======================
# ==============================================================================

# ------------------------------------------------------------------------------
# Initializes the environment that will be needed to build the BOOST package
macro(boost_build_initialization)
  # Parse arguments
  parse_arguments("BOOST_BUILD"
    "NO_LIB;NO_LOG;DEBUG;STATIC;RUNTIME_STATIC"
    "SOURCE_DIR;INSTALL_DIR;BUILD_DIR"
    "COMPONENTS;EXTRA_FLAGS"
    ""
    ${ARGN}
  )

  if(NOT BOOST_BUILD_BUILD_DIR)
    set(BOOST_BUILD_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/boost")
  endif()

  if(NOT BOOST_BUILD_INSTALL_DIR)
    set(BOOST_BUILD_INSTALL_DIR "${BOOST_BUILD_BUILD_DIR}/stage")
  endif()

  if(Boost_DEBUG)
    set(Boost_DEBUG NO_LOG DEBUG)
  else()
    unset(Boost_DEBUG)
  endif()

  set(BOOST_BOOTSTRAP   "${BOOST_BUILD_SOURCE_DIR}/bootstrap")
  set(BOOST_BUILDER     "${BOOST_BUILD_SOURCE_DIR}/bjam")
  if(NOT BOOST_BUILD_NO_LOG)
    set(BOOST_BUILDER_LOG LOG "${BOOST_BUILD_BUILD_DIR}/boost_builder.log")
  else()
    set(BOOST_BUILDER_LOG NO_LOG)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(BOOST_BUILDER "${BOOST_BUILDER}.exe")
    set(BOOST_BOOTSTRAP "${BOOST_BOOTSTRAP}.bat")
  else()
    set(BOOST_BOOTSTRAP "${BOOST_BOOTSTRAP}.sh")
  endif()

  # Set-up toolset name and associated ABI flags flags
  unset(cxx_flags)
  unset(link_flags)
  string(TOLOWER "${CMAKE_CXX_COMPILER_ID}" toolset_name)
  if ("${toolset_name}" STREQUAL "clang")
    set(cxx_flags "cxxflags=-std=c++11 -stdlib=libc++")
    set(link_flags "linkflags=-stdlib=libc++")
  elseif ("${toolset_name}" STREQUAL "gnu")
    set(toolset_name "gcc")
    string(REGEX REPLACE
      "([0-9]+)(\\.([0-9]+))?.*"
      "gcc\\1\\3"
      toolset_version
      "${CMAKE_CXX_COMPILER_VERSION}"
    )
    set(cxx_flags "cxxflags=-std=c++11")
  elseif("${toolset_name}" STREQUAL "msvc")
  else()
    external_error("COMPILER '${CMAKE_CXX_COMPILER_ID}' NOT HANDLED")
  endif()

  unset(_arch_cfg)
  list(APPEND _arch_cfg "architecture=x86")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(APPEND _arch_cfg "address-model=64")
  else()
    list(APPEND _arch_cfg "address-model=32")
  endif()

  ProcessorCount(_processors_count)
  if(_processors_count)
    set(_processors_count "-j${_processors_count}")
  else()
    unset(_processors_count)
  endif()

  # Compile BOOST's build tool
  if(NOT EXISTS "${BOOST_BUILDER}")
    external_log("Compiling Boost's builder")
    unset(_tmp)
    if(NOT "$ENV{CC}" STREQUAL "")
      set(_tmp "ENV" "CC=")
    endif()
    system_execute(
      ${Boost_DEBUG}
      "${BOOST_BUILDER_LOG}"
      WORKING_DIR "${BOOST_BUILD_SOURCE_DIR}"
      COMMAND     "${BOOST_BOOTSTRAP}"
      ${_tmp}
    )

    # Test we successfully built builder
    if(NOT EXISTS "${BOOST_BUILDER}")
      if(EXISTS "${BOOST_BUILD_SOURCE_DIR}/bootstrap.log")
        file(STRINGS "${BOOST_BUILD_SOURCE_DIR}/bootstrap.log" _tmp)
        external_log("===[ ${BOOST_BUILD_SOURCE_DIR}/bootstrap.log ] ===\n")
        foreach(_line IN LISTS _tmp)
          external_log("${_line}")
        endforeach()
        external_log("===")
      endif()
      external_error("Cannot compile Boost's builder.")
    endif()
  endif()

  # Compute list of possible components
  execute_process(
    OUTPUT_VARIABLE   _boost_libraries
    WORKING_DIRECTORY "${BOOST_BUILD_SOURCE_DIR}"
    COMMAND     "${BOOST_BUILDER}" "--show-libraries"
  )
  string(REGEX REPLACE "(^[^:]+:[\t ]*\n|[\t ]+)" ""
    _boost_libraries "${_boost_libraries}")
  string(REGEX REPLACE "-([^\n]*)\n+" "\\1;"
    _boost_libraries "${_boost_libraries}")
  string(REGEX REPLACE ";+$" ""
    _boost_libraries "${_boost_libraries}")

  # Create BOOST's headers tree in case of Boost Modular repo
  if(NOT EXISTS "${BOOST_BUILD_SOURCE_DIR}/boost")
    external_log("Create Boost's headers tree")
    system_execute(
      ${Boost_DEBUG}
      "${BOOST_BUILDER_LOG}"
      WORKING_DIR "${BOOST_BUILD_SOURCE_DIR}"
      COMMAND     "${BOOST_BUILDER}"
      ARGS        headers
    )
  endif()
endmacro()

# ------------------------------------------------------------------------------
# Build the BOOST package from the given source path
function(boost_build)

  # Prepare BOOST context
  boost_build_initialization("${ARGN}")

  # Prepare build options
  if(BOOST_BUILD_NO_LIB)
    # No libraries to compile...
    return()
  endif()

  set(_build_options
    "${cxx_flags}"
    "${link_flags}"
    "${_arch_cfg}"
    "toolset=${toolset_name}"
    "${_processors_count}"
    "-q"
    "--hash"
    --debug-configuration
    --layout=versioned
    stage
    "--stagedir=${BOOST_BUILD_INSTALL_DIR}"
    "--build-dir=${BOOST_BUILD_BUILD_DIR}"
  )

  if(NOT BOOST_BUILD_DEBUG)
    list(APPEND _build_options variant=release optimization=space)
  else()
    list(APPEND _build_options variant=debug)
  endif()

  unset(_skipped)
  list_join(_boost_libraries "|" _filter)
  foreach(_component ${BOOST_BUILD_COMPONENTS})
    if("${_component}" MATCHES "^(${_filter})$")
      list(APPEND _build_options "--with-${_component}")
    else()
      list(APPEND _skipped "${_component}")
    endif()
  endforeach()
  if(_skipped)
    list_join(_skipped ", " _skipped)
    external_log("Skipping implicit libraries: ${_skipped}")
  endif()

  if(BOOST_BUILD_EXTRA_FLAGS)
    list(APPEND _build_options "${BOOST_BUILD_EXTRA_FLAGS}")
  endif()

  if(NOT BOOST_BUILD_STATIC)
    list(APPEND _build_options link=shared runtime-link=shared)
  elseif(NOT BOOST_BUILD_RUNTIME_STATIC)
    list(APPEND _build_options link=static runtime-link=shared)
  else()
    list(APPEND _build_options link=static runtime-link=static)
  endif()

  # Compile BOOST libraries
  system_execute(
    ${Boost_DEBUG}
    "${BOOST_BUILDER_LOG}"
    WORKING_DIR "${BOOST_BUILD_SOURCE_DIR}"
    COMMAND     "${BOOST_BUILDER}"
    ARGS        "${_build_options}"
  )
endfunction()

