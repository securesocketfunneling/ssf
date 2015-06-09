# This CMAKE script should be included from parent wishing to build OpenSSL
# library.
#
#   openssl_build(
#     RUNTIME_STATIC       # Link against static runtime
#     STATIC               # Build only static libraries
#     SOURCE_DIR <PATH>    # Path to package's source tree
#     BUILD_DIR <PATH>     # Working directory where package should be built
#     INSTALL_DIR <PATH>   # Destination where to store built libraries
#     EXTRA_FLAGS          # OpenSSL's specific configuration flags
#   )
#
# Example:
#   include(openssl_build)
#
#   openssl_build(
#     SOURCE_DIR      "path/to/the/openssl/source/tree"
#     DESTINATION_DIR "path/where/to/store/built/libraries"
#   )
#
if(__H_OPENSSL_BUILD_INCLUDED)
  return()
endif()
set(__H_OPENSSL_BUILD_INCLUDED TRUE)

cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)

include(ExternalPackageHelpers)
include(FindPerl)
include(HelpersArguments)
include(MultiList)
include(FileEdit)
include(SystemTools)


# ==============================================================================
# ======================= D e f i n e   f u n c t i o n s ======================
# ==============================================================================

# ------------------------------------------------------------------------------
# Set-up the environment for Unix platforms
#
# Usage:
#   openssl_build_setup_unix(
#     out_setup_rules  # output variable where set-up rules will be stored
#     platform         # targeted platform
#     [extra_args]     # Additional arguments for configuration
#   )
#
macro(openssl_build_setup_unix out_setup_rules platform)
  # This set-up rule runs the Configure with all configuration options
  multi_list(APPEND ${out_setup_rules}
    EXEC "${PERL_EXECUTABLE}" ./Configure ${platform} ${ARGN}
    LOG  "${OPENSSL_CONFIG_LOG}"
  )

  # This next rule is to build the package
  multi_list(APPEND ${out_setup_rules} EXEC make all)

  # Finally, this last rule is to install package in case a destination
  # has been provided
  if(OPENSSL_BUILD_INSTALL_DIR)
    multi_list(APPEND ${out_setup_rules} EXEC make install_sw)
  endif()
endmacro()

# ------------------------------------------------------------------------------
# Set-up the environment for Darwin
#
# Usage:
#   openssl_build_setup_darwin(
#     out_setup_rules  # output variable where set-up rules will be stored
#     [extra_args]     # Additional arguments for configuration
#   )
#
macro(openssl_build_setup_darwin out_setup_rules)
  openssl_build_setup_unix(${out_setup_rules} darwin64-x86_64-cc ${ARGN})
endmacro()

# ------------------------------------------------------------------------------
# Set-up the environment for Linux
#
# Usage:
#   openssl_build_setup_linux(
#     out_setup_rules  # output variable where set-up rules will be stored
#     [extra_args]     # Additional arguments for configuration
#   )
#
macro(openssl_build_setup_linux out_setup_rules)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    openssl_build_setup_unix(${out_setup_rules} linux-x86_64 ${ARGN})
  else()
    openssl_build_setup_unix(${out_setup_rules} linux-elf ${ARGN})
  endif()
endmacro()

# ------------------------------------------------------------------------------
# Patches some generated files for the Windows environment
function(openssl_build_patch_windows)
  # Find generated Makefile files (i.e. "*.mak") on windows platform
  file(GLOB _files "${OPENSSL_BUILD_SOURCE_DIR}/ms/*.mak")

  # Define common path rules
  unset(_rules)
  list(APPEND _rules
    # Remove /Zi and /Fd from *CFLAG and ASM variables
    RULE "(^ASM|CFLAG)[\t ]*=" "[\t ]*/(Zi|Fd[^\t ]+)" ""
    # Remove option /debug from *LFLAGS variables
    RULE "LFLAGS[\t ]*=" "[\t ]*/debug" ""
  )

  if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(APPEND _rules
      # Add option /safeseh to *LFLAGS and ASM variables
      RULE "(^ASM|LFLAGS)[\t ]*=" "(.+)[\t ]*$" "\\1 -safeseh"
    )
  endif()

  list(APPEND _rules
    # Add option /dynamicbase to *LFLAGS
    RULE "LFLAGS[\t ]*=" "(.+)[\t ]*$" "\\1 /dynamicbase"
  )

  # Define runtime specific rules
  if(NOT OPENSSL_BUILD_RUNTIME_STATIC)
    list(APPEND _rules
      # Replace /MT to /MD from CFLAG
      RULE "^CFLAG[\t ]*=" "([\t ]*/M)T([d\t ]|$)" "\\1D\\2"
    )
  endif()

  # Patch all the generated Makefile files
  file_edit(
    MERGE_SPLIT
    FILE ${_files}
    ${_rules}
  )
endfunction()

# ------------------------------------------------------------------------------
# Set-up the environment for windows
#
# Usage:
#   openssl_build_setup_unix(
#     out_setup_rules  # output variable where set-up rules will be stored
#     [extra_args]     # Additional arguments for configuration
#   )
#
macro(openssl_build_setup_windows out_setup_rules)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    multi_list(APPEND ${out_setup_rules}
      WRAP "${PERL_EXECUTABLE}"
      ARGS "${OPENSSL_SOURCE_DIR}/Configure" VC-WIN64A ${ARGN}
      LOG  "${OPENSSL_CONFIG_LOG}"
    )
    multi_list(APPEND ${out_setup_rules}
      WRAP "${OPENSSL_SOURCE_DIR}/ms/do_win64a.bat"
    )
  else()
    multi_list(APPEND ${out_setup_rules}
      WRAP "${PERL_EXECUTABLE}"
      ARGS "${OPENSSL_SOURCE_DIR}/Configure" VC-WIN32 ${ARGN}
      LOG  "${OPENSSL_CONFIG_LOG}"
    )
    multi_list(APPEND ${out_setup_rules}
      WRAP "${OPENSSL_SOURCE_DIR}/ms/do_nasm.bat"
    )
  endif()

  multi_list(APPEND ${out_setup_rules}
    EVAL "openssl_build_patch_windows()"
  )

  multi_list(APPEND ${out_setup_rules}
    EVAL
      "file(MAKE_DIRECTORY \"${OPENSSL_BUILD_BUILD_DIR}/tmp/lib\")"
      "file(MAKE_DIRECTORY \"${OPENSSL_BUILD_BUILD_DIR}/tmp/dll\")"
      "file(MAKE_DIRECTORY \"${OPENSSL_BUILD_INSTALL_DIR}/lib\")"
      "file(MAKE_DIRECTORY \"${OPENSSL_BUILD_INSTALL_DIR}/dll\")"
      "file(MAKE_DIRECTORY \"${OPENSSL_BUILD_INSTALL_DIR}/include/openssl\")"
  )

  multi_list(APPEND ${out_setup_rules}
    WRAP nmake
    ARGS /nologo /f "${OPENSSL_SOURCE_DIR}/ms/nt.mak"
    ARGS "OUT_D=${OPENSSL_BUILD_INSTALL_DIR}/lib"
    ARGS "TMP_D=${OPENSSL_BUILD_BUILD_DIR}/tmp/lib"
    ARGS "INC_D=${OPENSSL_BUILD_INSTALL_DIR}/include"
    ARGS "INCO_D=${OPENSSL_BUILD_INSTALL_DIR}/include/openssl"
    ARGS headers lib
  )

  multi_list(APPEND ${out_setup_rules}
    WRAP nmake
    ARGS /nologo /f "${OPENSSL_SOURCE_DIR}/ms/ntdll.mak"
    ARGS "OUT_D=${OPENSSL_BUILD_INSTALL_DIR}/dll"
    ARGS "TMP_D=${OPENSSL_BUILD_BUILD_DIR}/tmp/dll"
    ARGS "INC_D=${OPENSSL_BUILD_INSTALL_DIR}/include"
    ARGS "INCO_D=${OPENSSL_BUILD_INSTALL_DIR}/include/openssl"
    ARGS headers lib
  )
endmacro()

# ------------------------------------------------------------------------------
# Initializes the environment that will be needed to build the OPENSSL package
macro(openssl_build_initialization)

  # Parse arguments
  parse_arguments("OPENSSL_BUILD"
    "RUNTIME_STATIC;STATIC"
    "SOURCE_DIR;INSTALL_DIR;BUILD_DIR"
    "EXTRA_FLAGS"
    ""
    ${ARGN}
  )

  if(OpenSSL_DEBUG)
    set(OpenSSL_DEBUG NO_LOG DEBUG)
  else()
    unset(OpenSSL_DEBUG)
  endif()

  # Ensure perl runtime is accessible
  if(NOT PERL_FOUND)
    external_error(
      "No perl interpreter found. "
      "Please ensure perl is available from system before running "
      "this script"
    )
  endif()

  # Set path to log file
  if(NOT OPENSSL_BUILD_BUILD_DIR)
    set(OPENSSL_BUILD_LOG "${CMAKE_BINARY_DIR}/openssl_build.log")
    set(OPENSSL_CONFIG_LOG "${CMAKE_BINARY_DIR}/openssl_configure.log")
  else()
    set(OPENSSL_BUILD_LOG "${OPENSSL_BUILD_BUILD_DIR}/build.log")
    set(OPENSSL_CONFIG_LOG "${OPENSSL_BUILD_BUILD_DIR}/configure.log")
  endif()

  # Check source directory
  if(NOT IS_DIRECTORY "${OPENSSL_BUILD_SOURCE_DIR}"
     OR NOT EXISTS "${OPENSSL_BUILD_SOURCE_DIR}/openssl.spec")
    external_error("Source path to OpenSSL package not defined or invalid")
  endif()

  # Create working directories
  foreach(_tmp "${OPENSSL_BUILD_BUILD_DIR}" "${OPENSSL_BUILD_INSTALL_DIR}")
    if(_tmp AND NOT IS_DIRECTORY "${_tmp}")
      file(MAKE_DIRECTORY "${_tmp}")
    endif()
  endforeach()
  unset(_tmp)

  # Retrieve OS name
  unset(os_name)
  if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Darwin|Linux|Windows)$")
    external_error("OS '${CMAKE_SYSTEM_NAME}' NOT HANDLED")
  endif()

  string(TOLOWER "${CMAKE_SYSTEM_NAME}" os_name)

endmacro()

# ------------------------------------------------------------------------------
# Build the OpenSSL command lines to compile package
macro(openssl_build_compilation_scripts out_setup_rules)
  # Compute compilation rules
  if(os_name STREQUAL "darwin")
    openssl_build_setup_darwin(${out_setup_rules} ${ARGN})
  elseif(os_name STREQUAL "linux")
    openssl_build_setup_linux(${out_setup_rules} ${ARGN})
  elseif(os_name STREQUAL "windows")
    openssl_build_setup_windows(${out_setup_rules} ${ARGN})
  else()
    external_error("COMPILATION FOR OS '${os_name}'NOT HANDLED")
  endif()
endmacro()

# ------------------------------------------------------------------------------
function(openssl_run)
  parse_arguments("RUN"
    ""
    ""
    "EXEC;EVAL;WRAP"
    ""
    ${ARGN}
  )

  # Ensure that NO unparsed arguments has been found
  if(RUN_ARGN)
    message(FATAL_ERROR "Unparsed arguments found:\n${RUN_ARGN}")
  endif()

  if(RUN_EVAL)
    # Proceed to the EVAL (if any)
    system_eval(${RUN_EVAL})
  elseif(RUN_EXEC)
    # Proceed to the EXEC
    system_execute(
      COMMAND ${RUN_EXEC}
      WORKING_DIR "${OPENSSL_BUILD_SOURCE_DIR}"
      ${OpenSSL_DEBUG}
    )
  else()
    # Proceed to the wrapped execution
    external_execute(
      COMMAND ${RUN_WRAP}
      WORKING_DIR "${OPENSSL_BUILD_SOURCE_DIR}"
      ${OpenSSL_DEBUG}
    )
  endif()
endfunction()

# ------------------------------------------------------------------------------
# Build the OpenSSL package from the given source path
function(openssl_build)

  # Prepare OpenSSL context
  openssl_build_initialization("${ARGN}")

  # Set-up list of configuration options
  unset(base_configure_options)
  if(OPENSSL_BUILD_STATIC)
    list(APPEND base_configure_options "no-shared")
  else()
    list(APPEND base_configure_options "shared")
  endif()

  # Complete configure options
  if(OPENSSL_BUILD_INSTALL_DIR)
    list(APPEND base_configure_options
      "--prefix=${OPENSSL_BUILD_INSTALL_DIR}"
    )
  endif()

  if(OPENSSL_EXTRA_FLAGS)
    list(APPEND base_configure_options "${OPENSSL_EXTRA_FLAGS}")
  endif()

  # Generate configure and build scripts
  unset(config_scripts)
  openssl_build_compilation_scripts(config_scripts ${base_configure_options})

  # Configure and build OpenSSL
  foreach(_command ${config_scripts})
    openssl_run(${_command})
  endforeach()
endfunction()

