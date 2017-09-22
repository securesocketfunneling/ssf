# Implementation of helpers for external package build scripts
#
if(__H_EXTERNAL_PACKAGE_HELPERS_INCLUDED)
  return()
endif()
set(__H_EXTERNAL_PACKAGE_HELPERS_INCLUDED TRUE)


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#                               B O O T S T R A P
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
set(_EXTERNALS_LOG_PATTERN "xXx")
set(_EXTERNALS_DIR "${CMAKE_CURRENT_LIST_DIR}")
if(EXTERNALS_BOOTSTRAP)
  # Register package's name
  string(TOUPPER "${EXTERNALS_PKG_NAME}" EXTERNALS_PKG_NAME_UPPER)

  # Set-up modules' search path
  if(NOT "$ENV{MODULES_SEARCH_PATH}" STREQUAL "")
    string(REPLACE "::" ";" CMAKE_MODULE_PATH "$ENV{MODULES_SEARCH_PATH}")
  endif()
  get_filename_component(_tmp "${CMAKE_PARENT_LIST_FILE}" PATH)
  list(APPEND CMAKE_MODULE_PATH "${_tmp}")

  # Check environment variables for package's settings
  foreach(_var SOURCE_DIR FLAGS EXTRA_FLAGS COMPONENTS NO_COMPONENTS)
    set(_var "${EXTERNALS_PKG_NAME_UPPER}_${_var}")
    if(NOT "$ENV{${_var}}" STREQUAL "")
      string(REPLACE "::" ";" ${_var} "$ENV{${_var}}")
    endif()
  endforeach()

  if (${EXTERNALS_PKG_NAME_UPPER}_COMPONENTS
  AND ${EXTERNALS_PKG_NAME_UPPER}_NO_COMPONENTS)
    message(FATAL_ERROR "[${EXTERNALS_PKG_NAME}] "
      "COMPONENTS and NO_COMPONENTS cannot stand together")
  endif()

  if(NOT ${EXTERNALS_PKG_NAME_UPPER}_SOURCE_DIR)
    message(FATAL_ERROR "[${EXTERNALS_PKG_NAME}] "
      "No source directory path defined"
    )
  endif()

  set(_EXTERNALS_LOG "${_EXTERNALS_LOG_PATTERN}")
else()
  set(_EXTERNALS_LOG "STATUS")
endif()

if(WIN32 AND NOT CYGWIN)
  find_program(GIT_PROGRAM git)
  if(GIT_PROGRAM)
    get_filename_component(_tmp "${GIT_PROGRAM}" PATH)
    if(_tmp MATCHES "(.*)/cmd$")
      list(APPEND CMAKE_PROGRAM_PATH "${CMAKE_MATCH_1}/bin")
    endif()
  endif()
endif()

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
include(HelpersArguments)
include(SystemTools)
include(FileEdit)
include(EnhancedList)

# ==============================================================================
# ======================= D e f i n e   f u n c t i o n s ======================
# ==============================================================================

# ------------------------------------------------------------------------------
# external_execute(
#  <legacy options>  # Options to path through to system_execute(...) function
# )
#
# This function is a wrapper to SystemTools's system_execute(...) function.
# Its purpose is to wrap the given command, and its specified arguments,
# with a platform's specific build environment where compile tools are all
# available.
#
function(external_execute)
  if(NOT (MSVC AND "${CMAKE_GENERATOR}" MATCHES "^Visual Studio [0-9]+"))
    system_execute(${ARGN})
    return()
  endif()

  # Parse arguments
  parse_arguments("WRAP"
    "NO_LOG;NO_LOG_DUMP;DEBUG"
    "WORKING_DIR;LOG;LOG_VAR;COMMAND"
    "ARGS;ENV"
    ""
    ${ARGN}
  )

  unset(_script_path)

  # Prepare wrapped command
  unset(_command_flags)
  foreach(_arg NO_LOG NO_LOG_DUMP DEBUG WORKING_DIR LOG LOG_VAR ENV ARGN)
    if(WRAP_${_arg})
      list(APPEND _command_flags "${_arg}" "${WRAP_${_arg}}")
    endif()
  endforeach()

  # Determine Visual Studio's script path
  if(CMAKE_CXX_COMPILER)
    get_filename_component(_vc_path "${CMAKE_CXX_COMPILER}" PATH)
  else()
    get_filename_component(_vc_path "${CMAKE_C_COMPILER}" PATH)
  endif()

  # Get Visual Studio profile
  get_filename_component(_vc_profile "${_vc_path}" NAME)
  if("${_vc_profile}" STREQUAL "bin")
    set(_vc_profile "32")
  endif()

  # Build BAT script to execute input command
  set(_debug OFF)
  if(WRAP_DEBUG)
    set(_debug ON)
  endif()

  file_create_unique(_script_path
    PREFIX "wrap_"
    SUFFIX ".bat"
    CONTENT
      "@REM Auto generated BAT file
       @echo ${_debug}
       call \"${_vc_path}/vcvars${_vc_profile}.bat\" && goto :next
       exit /b %ERRORLEVEL%
       goto :eof
       :next
       @echo ${_debug}
       \"${WRAP_COMMAND}\""
  )
  foreach(_arg IN LISTS WRAP_ARGS)
    file(APPEND "${_script_path}" " \"${_arg}\"")
  endforeach()
  file(APPEND "${_script_path}" "\n")

  if(NOT (_script_path AND EXISTS "${_script_path}"))
    external_error("Cannot create wrapper script")
  endif()

  if(WRAP_DEBUG)
    message(STATUS "*ENVWRAP SCRIPT: ${_script_path}")
  endif()

  system_execute(
    ${_command_flags}
    COMMAND "$ENV{COMSPEC}"
    ARGS "/c" "${_script_path}"
  )

  if(NOT WRAP_DEBUG)
    file(REMOVE "${_script_path}")
  endif()
endfunction()

# ------------------------------------------------------------------------------
# external_parse_arguments(
#   <name>     # Name of the package
#   <flags>    # List of flag directives to parse out
#   <singles>  # List of single value directives to parse out
#   <lists>    # List of multiple values directives to parse out
#   <maps>     # List of maps directives to parse out
# )
#
# This function parses the packages' arguments with the given list of
# directives' name.
# The packages' arguments are retrieved from the directive FLAGS present (if
# any) in the global variable <package_name>_FIND_COMPONENTS.
#
# Once executed, the following variables will be available in the scope of the
# caller:
# - <package_name>_FIND_COMPONENTS:
#   It is updated to contain ONLY components list
#
# - <PACKAGE_NAME>_<directive_name>:
#   Contains the value(s) of the found directive
#
# - <PACKAGE_NAME>_NO_COMPONENTS:
#   If no components have been specified and NO_COMPONENT directive is found
#   from <package_name>_FIND_COMPONENTS variable, then this variable will be
#   specified to inform caller that no components must be compiled.
#
macro(external_parse_arguments _name _flags _singles _lists _maps)
  # Parse package's FIND_COMPONENTS global variable to filter out
  # package's compilation flags
  string(TOUPPER "${_name}" _name_upper)
  parse_arguments("${_name_upper}"
    "NO_COMPONENTS"
    ""
    "FLAGS;WITH_COMPONENTS"
    ""
    ${${_name}_FIND_COMPONENTS}
  )

  if(${_name}_DEBUG)
    external_debug("external_parse_arguments")
    foreach(_var NO_COMPONENTS FLAGS WITH_COMPONENTS)
      set(_var "${_name_upper}_${_var}")
      external_debug("${_var} = ${${_var}}")
    endforeach()
  endif()

  if(${_name_upper}_WITH_COMPONENTS)
    list(APPEND ${_name_upper}_ARGN "${${_name_upper}_WITH_COMPONENTS}")
    unset(${_name_upper}_WITH_COMPONENTS)
  endif()

  # Reset package's FIND_COMPONENTS variable
  if(${_name_upper}_ARGN)
    set(${_name}_FIND_COMPONENTS ${${_name_upper}_ARGN})
    unset(${_name_upper}_ARGN)
  else()
    unset(${_name}_FIND_COMPONENTS)
  endif()
  set(${_name}_FIND_COMPONENTS ${${_name}_FIND_COMPONENTS} PARENT_SCOPE)

  if(${_name}_DEBUG)
    external_debug("${_name}_FIND_COMPONENTS = ${${_name}_FIND_COMPONENTS}")
  endif()

  # Parse package's (real) arguments
  if(${_name_upper}_FLAGS)
    parse_arguments("${_name_upper}"
      "${_flags}"
      "${_singles}"
      "${_lists}"
      "${_maps}"
      ${${_name_upper}_FLAGS}
    )
    unset(${_name_upper}_FLAGS)

    if(${${_name_upper}_ARGN})
      external_error("Unexpected flags found: '${${_name_upper}_ARGN}'")
    endif()

    if(${_name}_DEBUG)
      foreach(_var ${_flags} ${_singles} ${_lists} ${_maps})
        set(_var "${_name_upper}_${_var}")
        external_debug("${_var} = ${${_var}}")
      endforeach()
    endif()
  endif()

  unset(_name_upper)
endmacro()

# ------------------------------------------------------------------------------
# external_error(
#   <error_message>  # The list of error messages to display
# )
#
function(external_error)
  if(NOT PKG_NAME AND EXTERNALS_PKG_NAME)
    set(PKG_NAME "${EXTERNALS_PKG_NAME}")
  endif()

  message(FATAL_ERROR "[${PKG_NAME}] # " ${ARGN})
endfunction()

# ------------------------------------------------------------------------------
# external_debug(
#   <debug_message>  # The list of error messages to display
# )
#
function(external_debug)
  if(NOT PKG_NAME AND EXTERNALS_PKG_NAME)
    set(PKG_NAME "${EXTERNALS_PKG_NAME}")
  endif()

  if(NOT ${PKG_NAME}_DEBUG)
    return()
  endif()

  if(PKG_NAME)
    set(PKG_NAME "[${PKG_NAME}] ")
  endif()

  message("${PKG_NAME}* " ${ARGN})
endfunction()

# ------------------------------------------------------------------------------
# external_log(
#   <message_to_log>  # The list of log messages linked to package
# )
#
function(external_log)
  if(NOT PKG_NAME AND EXTERNALS_PKG_NAME)
    set(PKG_NAME "${EXTERNALS_PKG_NAME}")
  endif()

  if(NOT PKG_NAME OR ${PKG_NAME}_DEBUG OR NOT ${PKG_NAME}_FIND_QUIETLY)
    if(PKG_NAME)
      set(PKG_NAME "[${PKG_NAME}] ")
    endif()

    message("${_EXTERNALS_LOG}" "${PKG_NAME}" ${ARGN})
  endif()
endfunction()

# ------------------------------------------------------------------------------
# externals_relative_path(
#   <var_out>   # Destination variable where to store subdirectory relative path
#   <dir_path>  # Path to directory that has to be converted into a relative
#               # subdirectory path
# )
#
# This function transform the given directory path into a relative path anchored
# to value of CMAKE_SOURCE_DIR.
#
function(externals_relative_path var_out dir_path)
  # Reset destination variable
  set(${var_out} "" PARENT_SCOPE)
  # Remove CMAKE_SOURCE_DIR from given path
  string(REPLACE "${CMAKE_SOURCE_DIR}" "" _tmp "${dir_path}")
  if(_tmp AND NOT "${_tmp}" STREQUAL "${dir_path}")
    # Remove any trailing '/' characters before returning result
    string(REGEX REPLACE "^/*([^/].*[^/])/*$" "\\1" _tmp "${_tmp}")
    set(${var_out} "${_tmp}" PARENT_SCOPE)
  endif()
endfunction()

# ------------------------------------------------------------------------------
# external_find_file(
#   <var_out>                  # Output variable where to store full path of
#                              # found file
#   NAMES <file_patterns>      # List of reg-ex patterns of file to found
#   PATHS <search_paths>       # List of paths where search must be performed
#   [PATH_SUFFIXES <suffixes>  # List of suffixes to append to each search
#                              # paths
# )
#
# This function searches for a file matching given patterns in the specified
# folders.
#
function(external_find_file var_out)
  # Reset output variable
  set(${var_out} "NOTFOUND" PARENT_SCOPE)

  # Parse arguments
  parse_arguments("PKG_FILE"
    ""
    ""
    "NAMES;PATHS;PATH_SUFFIXES"
    ""
    ${ARGN}
  )

  if(NOT PKG_FILE_NAMES)
    external_error("No file names to search for")
  endif()

  if(NOT PKG_FILE_PATHS)
    external_error("No file paths to search from")
  endif()

  # Build list of matching path
  unset(_paths_list)
  foreach(_path IN LISTS PKG_FILE_PATHS)
    if(_path)
      list(APPEND _paths_list "${_path}")
      foreach(_suffix IN LISTS PKG_FILE_PATH_SUFFIXES)
        list(APPEND _paths_list "${_path}/${_suffix}")
      endforeach()
    endif()
  endforeach()

  # Search for the specified files
  list_join(PKG_FILE_NAMES "|" _files_pattern)
  foreach(_path IN LISTS _paths_list)
    file(GLOB _files_list RELATIVE "${_path}" "${_path}/*")
    if("${_files_list}" MATCHES "^(.*;|)(${_files_pattern})(;.*|)$")
      set(${var_out} "${_path}/${CMAKE_MATCH_2}" PARENT_SCOPE)
      return()
    endif()
  endforeach()
endfunction()

# ------------------------------------------------------------------------------
# external_teardown_build_context(
#   [NAME]  <package_name>     # Name of the targeted package
#   [CACHE_SUFFIX <suffix>]    # Suffix to use to identify the cache of already
# )
#
# This function stores the list of components and extra flags to the package's
# specific cache. This cache is later used by 'external_setup_build_context()'
# to retrieve list of already available components and extra flags.
#
# N.B.: The components list and the extra flags list is retrieved from caller's
#       scope.
#
function(external_teardown_build_context)
  # Parse arguments
  parse_arguments("PKG"
    ""
    "NAME;CACHE_SUFFIX"
    ""
    ""
    ${ARGN}
  )

  # Check arguments
  if(NOT PKG_NAME)
    if(NOT PKG_ARGN)
      external_error("No name provided for external package")
    endif()
    list(GET PKG_ARGN 0 PKG_NAME)
    list(REMOVE_AT PKG_ARGN 0)
  endif()
  string(TOLOWER "${PKG_NAME}" PKG_NAME_LOWER)
  string(TOUPPER "${PKG_NAME}" PKG_NAME_UPPER)

  if(NOT PKG_CACHE_SUFFIX AND PKG_ARGN)
    list(GET PKG_ARGN 0 PKG_CACHE_SUFFIX)
    list(REMOVE_AT PKG_ARGN 0)
  endif()

  set(PKG_CACHE_VAR "${PKG_NAME_UPPER}_CACHE_COMPONENTS_${PKG_CACHE_SUFFIX}")
  set(PKG_CACHE_FLAGS_VAR "${PKG_NAME_UPPER}_CACHE_FLAGS_${PKG_CACHE_SUFFIX}")

  # Store extra flags cache
  if(${PKG_NAME_UPPER}_EXTRA_FLAGS)
    set(${PKG_CACHE_FLAGS_VAR} "${${PKG_NAME_UPPER}_EXTRA_FLAGS}"
      CACHE INTERNAL "List of ${PKG_NAME}'s extra flags to build components"
    )
  endif()

  # Store components cache
  set(_cache_info "List of ${PKG_NAME}'s already built components")
  if(${PKG_NAME_UPPER}_COMPONENTS)
    set(${PKG_CACHE_VAR} "${${PKG_NAME_UPPER}_COMPONENTS}"
      CACHE INTERNAL "${_cache_info}"
    )
  elseif(NOT ${PKG_NAME_UPPER}_NO_COMPONENTS)
    set(${PKG_CACHE_VAR} "-ALL-" CACHE INTERNAL "${_cache_info}")
  endif()

  if(${PKG_NAME}_DEBUG)
    external_debug("external_set_cache_components:")
    external_debug("  PKG_CACHE_VAR         : ${PKG_CACHE_VAR}")
    external_debug("  ${PKG_CACHE_VAR}      : ${${PKG_CACHE_VAR}}")
    external_debug("  PKG_CACHE_FLAGS_VAR   : ${PKG_CACHE_FLAGS_VAR}")
    external_debug("  ${PKG_CACHE_FLAGS_VAR}: ${${PKG_CACHE_FLAGS_VAR}}")
  endif()
endfunction()

# ------------------------------------------------------------------------------
# external_setup_build_context(
#   [NAME]  <package_name>     # Name of the targeted package
#   [CACHE_SUFFIX <suffix>]    # Suffix to use to identify the cache of already
#                              # built components
# )
#
# This function sets up the package's environment for the build process.
# It will create the following variables in the caller's scope:
# - <PACKAGE_NAME>_EXTRA_FLAGS:
#   The complete list of extra (i.e. raw) flags to pass-through to package's
#   low level build scripts
# - <PACKAGE_NAME>_COMPONENTS:
#   The complete list of components that are requested for this build process
#   of package
# - <PACKAGE_NAME>_NO_COMPONENTS:
#   Boolean flag indicating that package must be build WITHOUT any components.
# - <PACKAGE_NAME>_NEED_BUILD:
#   Boolean flag indicating if a build is really needed (i.e. new components
#   have been requested or some new extra flags have been defined).
#
function(external_setup_build_context)
  # Parse arguments
  parse_arguments("PKG"
    ""
    "NAME;CACHE_SUFFIX"
    ""
    ""
    ${ARGN}
  )

  # Check arguments
  if(NOT PKG_NAME)
    if(NOT PKG_ARGN)
      external_error("No name provided for external package")
    endif()
    list(GET PKG_ARGN 0 PKG_NAME)
    list(REMOVE_AT PKG_ARGN 0)
  endif()
  string(TOLOWER "${PKG_NAME}" PKG_NAME_LOWER)
  string(TOUPPER "${PKG_NAME}" PKG_NAME_UPPER)

  if(NOT PKG_CACHE_SUFFIX AND PKG_ARGN)
    list(GET PKG_ARGN 0 PKG_CACHE_SUFFIX)
    list(REMOVE_AT PKG_ARGN 0)
  endif()

  set(PKG_CACHE_VAR "${PKG_NAME_UPPER}_CACHE_COMPONENTS_${PKG_CACHE_SUFFIX}")
  set(PKG_CACHE_FLAGS_VAR "${PKG_NAME_UPPER}_CACHE_FLAGS_${PKG_CACHE_SUFFIX}")

  if(${PKG_NAME}_DEBUG)
    external_debug("external_setup_build_context:")
    external_debug("  PKG_CACHE_VAR         : ${PKG_CACHE_VAR}")
    external_debug("  ${PKG_CACHE_VAR}      : ${${PKG_CACHE_VAR}}")
    external_debug("  PKG_CACHE_FLAGS_VAR   : ${PKG_CACHE_FLAGS_VAR}")
    external_debug("  ${PKG_CACHE_FLAGS_VAR}: ${${PKG_CACHE_FLAGS_VAR}}")
    foreach(_var COMPONENTS NO_COMPONENTS EXTRA_FLAGS)
      set(_var "${PKG_NAME_UPPER}_${_var}")
      external_debug("  ${_var} : ${${_var}}")
    endforeach()
  endif()

  # Assume no additional build is needed
  set(_need_build OFF)

  # Check no new extra flags have been provided
  if(${PKG_CACHE_FLAGS_VAR} OR ${PKG_NAME_UPPER}_EXTRA_FLAGS)
    set(_flags_to_use "${${PKG_CACHE_FLAGS_VAR}}")
    if(${PKG_NAME_UPPER}_EXTRA_FLAGS)
      string(REPLACE "${${PKG_NAME_UPPER}_EXTRA_FLAGS}" "" _tmp
        "${${PKG_CACHE_FLAGS_VAR}}"
      )
      if("${_tmp}" STREQUAL "${${PKG_CACHE_FLAGS_VAR}}")
        list(APPEND _flags_to_use "${${PKG_NAME_UPPER}_EXTRA_FLAGS}")
        list_join(_flags_to_use " " _tmp)
        external_log("Extra Flags: ${_tmp}")
        set(_need_build ON)
      endif()
    endif()

    set(${PKG_NAME_UPPER}_EXTRA_FLAGS "${_flags_to_use}" PARENT_SCOPE)
  endif()

  # Check if all package's components have already been built
  if(${PKG_NAME_UPPER}_NO_COMPONENTS)
    set(_need_build ON)
    set(${PKG_NAME_UPPER}_COMPONENTS "" PARENT_SCOPE)
  elseif(NOT "${PKG_CACHE_VAR}" STREQUAL "-ALL-")
    # Extract newly specified components from given list
    unset(_components_to_build)
    if(${PKG_NAME_UPPER}_COMPONENTS)
      # Create the list of components to build
      list(REMOVE_DUPLICATES ${PKG_NAME_UPPER}_COMPONENTS)
      list(APPEND _components_to_build "${${PKG_NAME_UPPER}_COMPONENTS}")
      if(${PKG_CACHE_VAR})
        list(REMOVE_ITEM _components_to_build ${${PKG_CACHE_VAR}})
      endif()
    endif()

    # Check if new components have to be built
    set(_build_message "Available components: ")
    if(_components_to_build OR (_need_build AND ${PKG_CACHE_VAR}))
      # Create reg-ex filter for the components to build
      list_join(_components_to_build "|" _components_filter)

      # Create the full list of components to be present in to build libraries
      list(APPEND _components_to_build ${${PKG_CACHE_VAR}})
      list(SORT _components_to_build)

      # Create composed message with new and already available components
      if(_components_filter)
        string(REGEX REPLACE
          "(${_components_filter})"
          "*\\1"
          _displayed_components_list
          "${_components_to_build}"
        )
      else()
        set(_displayed_components_list "${_components_to_build}")
      endif()
      list_join(_displayed_components_list ", " _displayed_components_list)

      # Add new message in list
      list(APPEND _build_message "${_displayed_components_list}")
    elseif(NOT ${PKG_NAME_UPPER}_COMPONENTS)
      list(APPEND _build_message "-ALL-")
    endif()

    if(_components_to_build OR NOT ${PKG_NAME_UPPER}_COMPONENTS)
      external_log(${_build_message})
      set(_need_build ON)
    endif()

    set(${PKG_NAME_UPPER}_COMPONENTS "${_components_to_build}" PARENT_SCOPE)
  endif()


  set(${PKG_NAME_UPPER}_NEED_BUILD "${_need_build}" PARENT_SCOPE)
endfunction()


# ------------------------------------------------------------------------------
# external_unpack_archive(
#   SOURCE      <file_path>       # Path to archive to expand
#   DESTINATION <directory_path>  # Path to destination directory where to
#                                 # expand archive
# )
#
# This function expands the given archive in the specified destination
#
function(external_unpack_archive)
  # Parse arguments
  parse_arguments("PACK"
    ""
    "SOURCE;DESTINATION"
    ""
    ""
    ${ARGN}
  )

  if(NOT (PACK_SOURCE AND EXISTS "${PACK_SOURCE}"))
    external_error("Invalid source")
  endif()

  if(NOT (PACK_DESTINATION AND IS_DIRECTORY "${PACK_DESTINATION}"))
    external_error("Invalid destination")
  endif()

  # Find tools needed for unpacking archives
  unset(_search_paths)
  unset(_unpacker_name)
  unset(_unpacker CACHE)
  if(CMAKE_HOST_WIN32)
    set(_unpacker_name "winrar")
  else()
    if("${PACK_SOURCE}" MATCHES "^.*\\.zip$")
        set(_unpacker_name "unzip")
    else()
        set(_unpacker_name "tar")
    endif()
  endif()

  find_program(_unpacker "${_unpacker_name}" "${_search_paths}")
  if(NOT _unpacker)
     external_error("No '${_unpacker_name}' tool found")
  endif()

  # Compute arguments to extract package's source tree from archive
  unset(_unpacker_args)
  if(CMAKE_HOST_WIN32)
    list(APPEND _unpacker_args
      "x"      # Extract files from archive
      "-inul"  # Do not prompt dialogue box in case of errors
      "-o+"    # Always overwrite destination files
      "-ibck"  # Run in background (i.e. no progress window displayed)
      "${PACK_SOURCE}"
    )
  else()
    list(APPEND _unpacker_args
      "x"     # Extract files from archive
      "-f"    # Unpack following file
      "${PACK_SOURCE}"
    )

    if("${PACK_SOURCE}" MATCHES "^.*\\.(tgz|tar\\.gz)$")
      list(INSERT _unpacker_args 1 "-z")
    elseif("${PACK_SOURCE}" MATCHES "^.*\\.(tbz|tar\\.bz2)$")
      list(INSERT _unpacker_args 1 "-j")
    elseif("${PACK_SOURCE}" MATCHES "^.*\\.(tar\\.Z)$")
      list(INSERT _unpacker_args 1 "-Z")
    elseif("${PACK_SOURCE}" MATCHES "^.*\\.zip$")
      unset(_unpacker_args)
      list(APPEND _unpacker_args "${PACK_SOURCE}")
    elseif(NOT "${PACK_SOURCE}" MATCHES "^.*\\.tar$")
      external_error("Unsuported archive type '${PACK_SOURCE}'")
    endif()
  endif()

  if(${PKG_NAME}_DEBUG)
    external_debug("Archive path '${PACK_SOURCE}':")
    external_debug("  extract base  = '${PACK_DESTINATION}'")
    external_debug("  unpacker      = '${_unpacker_name}' [${_unpacker}]")
    external_debug("  unpacker args = '${_unpacker_args}'")
  endif()

  # Expand archive
  system_execute(
    WORKING_DIR "${PACK_DESTINATION}"
    COMMAND "${_unpacker}" ARGS "${_unpacker_args}"
  )
endfunction()

# ------------------------------------------------------------------------------
function(externals_select_expand_dir _var _base_dir)
  # Set root path
  unset(_expand_dir)
  if(EXTERNALS_EXPAND_DIR)
    list(APPEND _expand_dir "${EXTERNALS_EXPAND_DIR}")
  else()
    list(APPEND _expand_dir "${CMAKE_BINARY_DIR}" "src.externals")
  endif()

  # Add sub (project relative) path
  if(EXTERNALS_USE_RELATIVE_DIR AND _base_dir)
    externals_relative_path(_tmp "${_base_dir}")
    if(_tmp)
      list(APPEND _expand_dir "${_tmp}")
    endif()
  endif()

  # Create directory
  list_join(_expand_dir "/" _expand_dir)
  file(MAKE_DIRECTORY "${_expand_dir}")

  # Return path
  set(${_var} "${_expand_dir}" PARENT_SCOPE)
endfunction()

# ------------------------------------------------------------------------------
# external_search_source_path(
#   [NAME]  <package_name>  # Name of the package the sources have to be found
#   [VAR    <var_name>]     # The name of  variable that will receive the path
#   [CLUES] <files_list>    # List of package's file names that should be found
#                           # while searching for package's source path
# )
#
# This function is used to search and return the path to the directory
# containing the source of an external package.
function(external_search_source_path)
  # Parse arguments
  parse_arguments("PKG"
    ""
    "NAME;VAR"
    "CLUES"
    ""
    ${ARGN}
  )

  # Check arguments
  if(NOT PKG_NAME)
    if(NOT PKG_ARGN)
      external_error("No name provided for external package to build")
    endif()
    list(GET PKG_ARGN 0 PKG_NAME)
    list(REMOVE_AT PKG_ARGN 0)
  endif()
  string(TOLOWER "${PKG_NAME}" PKG_NAME_LOWER)
  string(TOUPPER "${PKG_NAME}" PKG_NAME_UPPER)

  if(NOT PKG_VAR)
    set(PKG_VAR "${PKG_NAME_UPPER}_SOURCE_DIR")
  elseif(${PKG_VAR} AND NOT IS_DIRECTORY "${${PKG_VAR}}")
    external_error("Invalid source directory '${${PKG_VAR}}'")
  endif()

  if(NOT PKG_CLUES)
    if(NOT PKG_ARGN)
      external_error("No clues given to search ${PKG_NAME}'s source path")
    endif()
    set(PKG_CLUES "${PKG_ARGN}")
    unset(PKG_ARGN)
  endif()

  set(_archive_pattern
    "((${PKG_NAME}|${PKG_NAME_LOWER})[-_].*)\\.(t[bg]z|tar(\\.(gz|bz2))?|zip)"
  )
  unset(_witness_file)
  external_find_file(_witness_file
    NAMES
      "${PKG_CLUES}" "${_archive_pattern}"
    PATHS
      "${${PKG_VAR}}"
      "${${PKG_NAME_UPPER}_ROOT_DIR}"
      "${CMAKE_SOURCE_DIR}/third_party"
      "${PROJECT_SOURCE_DIR}/third_party"
      "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
    PATH_SUFFIXES
      ${PKG_NAME}
      ${PKG_NAME_LOWER}
  )

  set(${PKG_VAR} "NOTFOUND")
  if(_witness_file)
    get_filename_component(${PKG_VAR} "${_witness_file}" PATH)

    # Check if found file is an archive
    if("${_witness_file}" MATCHES "^.*/(${_archive_pattern})$")
      set(_archive_base_name "${CMAKE_MATCH_2}")
      external_log("Using package archive: ${_witness_file}")

      # Compute path where to expand archive
      externals_select_expand_dir(_archive_expand_dir "${${PKG_VAR}}")

      # Extract package's source tree from archive
      if(NOT IS_DIRECTORY "${_archive_expand_dir}/${_archive_base_name}")
        external_unpack_archive(
          SOURCE      "${_witness_file}"
          DESTINATION "${_archive_expand_dir}"
        )

        if(NOT IS_DIRECTORY "${_archive_expand_dir}/${_archive_base_name}")
          external_error("Cannot locate expanded archive")
        endif()

        #Check if some patches have to be applied
        file(GLOB _patch_files "${${PKG_VAR}}/patches/*")
        if(_patch_files)
          find_program(PATCH_PROGRAM "patch")
          if(NOT PATCH_PROGRAM)
            message(FATAL_ERROR
              "Command 'patch' is missing.\nCannot apply patches."
            )
          endif()
          list(SORT _patch_files)
          foreach(_patch IN LISTS _patch_files)
            external_log("Applying patch: ${_patch}")
            system_execute(
              WORKING_DIR "${_archive_expand_dir}/${_archive_base_name}"
              COMMAND     "${PATCH_PROGRAM}"
              ARGS        -p 1 -i "${_patch}"
            )
          endforeach()
        endif()
      endif()

      # Ensure extracted files matches the searched package
      external_find_file(_witness_file
        NAMES
          "${PKG_CLUES}"
        PATHS
          "${_archive_expand_dir}/${_archive_base_name}"
      )

      get_filename_component(${PKG_VAR} "${_witness_file}" PATH)
    endif()
  endif()

  if(${PKG_NAME}_DEBUG)
    external_debug("external_search_source_path:")
    external_debug("  ${PKG_VAR}=${${PKG_VAR}}")
  endif()

  if(NOT (${PKG_VAR} AND IS_DIRECTORY "${${PKG_VAR}}"))
    external_error("Cannot determine ${PKG_NAME}'s source directory (${${PKG_VAR}})")
  endif()

  # Return the source path
  external_log("Found source path: ${${PKG_VAR}}")
  set(${PKG_VAR} "${${PKG_VAR}}" PARENT_SCOPE)
endfunction()

# ------------------------------------------------------------------------------
function(externals_select_build_dir _var)
  # Compute sub-path for package
  unset(_build_dir)
  if(EXTERNALS_USE_RELATIVE_DIR AND PKG_SOURCE_DIR)
    get_filename_component(_tmp "${PKG_SOURCE_DIR}" PATH)
    externals_relative_path(_build_dir "${_tmp}")
    list(APPEND _build_dir "${PKG_NAME_LOWER}")
  else()
    string(SHA1 _build_dir "${CMAKE_CURRENT_LIST_DIR}")
  endif()
  list_join(_build_dir "/" _build_dir)

  # Prepend main path for package
  if(EXTERNALS_BINARY_DIR)
    set(_build_dir "${EXTERNALS_BINARY_DIR}/${_build_dir}")
  else()
    set(_build_dir "${CMAKE_BINARY_DIR}/bin.externals/${_build_dir}")
  endif()

  file(MAKE_DIRECTORY "${_build_dir}")

  set(${_var} "${_build_dir}" PARENT_SCOPE)
endfunction()

# ------------------------------------------------------------------------------
# externals_build(
#   [NO_LOG]                            # Flag to not redirect logs to file
#   [NO_COMPONENTS]                     # Flag to not compile any components
#   [NAME] <package_name>               # Name of the package to build
#   [[COMPONENTS] <components>]         # List of components to build
#   [SOURCE_DIR <path>]                 # The path of the package's sources
#   [FLAGS <package_flags>]             # The package's build flags
#   [EXTRA_FLAGS <flags>]               # Extra (i.e. raw) flags to pass-through
#                                       # to package's low level build scripts
#   [SOURCE_SEARCH_CLUES <files_list>]  # List of package's file names that
#                                       # should be found while searching for
#                                       # package's source path
# )
#
# This function builds the specified external package.
# In case SOURCE_DIR flag IS NOT specified, an attempt will be performed to
# automatically find package's source path from:
#   - ${<package_upper_name>_ROOT_DIR}
#   - ${CMAKE_SOURCE_DIR}/third_party
#   - ${PROJECT_SOURCE_DIR}/third_party
#   - ${CMAKE_CURRENT_SOURCE_DIR}/third_party
#
# In order to help identifying the package's source path, the file names,
# specified in flag named SOURCE_SEARCH_CLUES, will searched in the paths
# mentioned above.
#
# N.B.: If SOURCE_DIR is specified then SOURCE_SEARCH_CLUES flag is ignored.
#
# In addition, the global variable EXTERNALS_BINARY_DIR can be defined to force
# the root path where ALL the externals have to be built.
#    N.B.: If defined, this root directory will be used to store ALL
#          package's artefact.
#          Otherwise, the main project's binary directory will be used as binary
#          root path.

function(externals_build)
  # Parse arguments for package's compilation flags
  parse_arguments("PKG"
    "NO_LOG;NO_COMPONENTS"
    "NAME;SOURCE_DIR"
    "FLAGS;EXTRA_FLAGS;SOURCE_SEARCH_CLUES;COMPONENTS"
    ""
    ${ARGN}
  )

  # Check arguments
  if(NOT PKG_NAME)
    if(NOT PKG_ARGN)
      external_error("No name provided for external package to build")
    endif()
    list(GET PKG_ARGN 0 PKG_NAME)
    list(REMOVE_AT PKG_ARGN 0)
  endif()
  string(TOLOWER "${PKG_NAME}" PKG_NAME_LOWER)
  string(TOUPPER "${PKG_NAME}" PKG_NAME_UPPER)

  if(NOT PKG_COMPONENTS AND PKG_ARGN)
    set(PKG_COMPONENTS "${PKG_ARGN}")
    unset(PKG_ARGN)
  endif()

  if(PKG_COMPONENTS AND PKG_NO_COMPONENTS)
    external_error("COMPONENTS and NO_COMPONENTS cannot stand together")
  endif()

  if(PKG_COMPONENTS)
    list(SORT PKG_COMPONENTS)
    list_join(PKG_COMPONENTS ", " _tmp)
  elseif(NOT PKG_NO_COMPONENTS)
    set(_tmp "-ALL-")
  else()
    set(_tmp "-NONE-")
  endif()
  external_log("Looking for components: ${_tmp}")

  # Check/Search for source directory
  external_search_source_path(
    NAME  "${PKG_NAME}"
    VAR   PKG_SOURCE_DIR
    CLUES ${PKG_SOURCE_SEARCH_CLUES}
  )

  if(NOT (PKG_SOURCE_DIR AND EXISTS "${PKG_SOURCE_DIR}"))
    external_error("No source path provided for package ${PKG_NAME}")
  endif()

  if(${PKG_NAME}_DEBUG)
    external_debug("PKG_NAME          : ${PKG_NAME}")
    external_debug("PKG_SOURCE_DIR    : ${PKG_SOURCE_DIR}")
    external_debug("PKG_COMPONENTS    : ${PKG_COMPONENTS}")
    external_debug("PKG_NO_COMPONENTS : ${PKG_NO_COMPONENTS}")
    external_debug("PKG_FLAGS         : ${PKG_FLAGS}")
    external_debug("PKG_EXTRA_FLAGS   : ${PKG_EXTRA_FLAGS}")
  endif()

  # Set-up Package's build directory
  externals_select_build_dir(PKG_BUILD_DIR)
  if(${PKG_NAME}_DEBUG)
    external_debug("PKG_BUILD_DIR: ${PKG_BUILD_DIR}")
  endif()

  # Execute External Package's CMake build script
  unset(_args)
  if(CMAKE_GENERATOR)
    list(APPEND _args "-G" "${CMAKE_GENERATOR}")
  endif()

  if(CMAKE_GENERATOR_TOOLSET)
    list(APPEND _args "-T" "${CMAKE_GENERATOR_TOOLSET}")
  endif()

  set(_vars_list
    CMAKE_BUILD_TYPE
    ${PKG_NAME}_DEBUG
    ${PKG_NAME}_FIND_QUIETLY
  )
  foreach(_var IN LISTS _vars_list)
    if(${_var})
      list(APPEND _args "-D" "${_var}=${${_var}}")
    else()
      list(APPEND _args "-U" "${_var}")
    endif()
  endforeach()

  list(APPEND _args "-D" "EXTERNALS_PKG_NAME=${PKG_NAME}")
  list(APPEND _args "-D" "EXTERNALS_BOOTSTRAP=ON")
  list(APPEND _args "-D" "CMAKE_MODULE_PATH=${_EXTERNALS_DIR}")

  unset(_env_vars)
  foreach(_var SOURCE_DIR FLAGS EXTRA_FLAGS COMPONENTS NO_COMPONENTS)
    if(PKG_${_var})
      list_join(PKG_${_var} "::" _tmp)
      list(APPEND _env_vars "${PKG_NAME_UPPER}_${_var}=${_tmp}")
    endif()
  endforeach()

  list_join(CMAKE_MODULE_PATH "::" _tmp)
  list(APPEND _env_vars "MODULES_SEARCH_PATH=${_tmp}")

  unset(_exec_flags)
  unset(_log_file)
  if(NOT (${PKG_NAME}_DEBUG OR PKG_NO_LOG))
    file_create_unique(_log_file
      PREFIX "${PKG_NAME_UPPER}"
      SUFFIX ".log"
    )
    list(APPEND _exec_flags LOG "${_log_file}")
  elseif(${PKG_NAME}_DEBUG)
    list(APPEND _exec_flags NO_LOG DEBUG)
  elseif(PKG_NO_LOG)
    list(APPEND _exec_flags NO_LOG)
  endif()

  system_execute(
    ${_exec_flags}
    WORKING_DIR "${PKG_BUILD_DIR}"
    COMMAND     "${CMAKE_COMMAND}"
    ARGS        "${_args}" "${CMAKE_CURRENT_LIST_DIR}/build"
    ENV         "${_env_vars}"
  )

  if(_log_file AND EXISTS "${_log_file}")
    file(STRINGS "${_log_file}" _logs REGEX "^${_EXTERNALS_LOG_PATTERN}")
    foreach(_log IN LISTS _logs)
      string(REGEX REPLACE "^${_EXTERNALS_LOG_PATTERN}" "" _log "${_log}")
      message(STATUS "${_log}")
    endforeach()
    file(REMOVE "${_log_file}")
  endif()

  set(${PKG_NAME_UPPER}_SOURCE_DIR "${PKG_SOURCE_DIR}" PARENT_SCOPE)
  set(${PKG_NAME_UPPER}_BUILD_DIR "${PKG_BUILD_DIR}" PARENT_SCOPE)
endfunction()
