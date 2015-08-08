#
# Define some helpers for IDE's Groups management
#
if(__H_HELPER_IDE_GROUP_INCLUDED)
  return()
endif()
set(__H_HELPER_IDE_GROUP_INCLUDED TRUE)

set(ENABLE_SUBTEST_TARGETS "OFF" CACHE BOOL
  "Create targets for each sub-tests declared in high level test targets"
)

set(UNIT_TEST_REPORTS_DIR "gtest_reports" CACHE STRING
  "Location where to store artifacts"
)

if(NOT IS_ABSOLUTE "${UNIT_TEST_REPORTS_DIR}")
  get_filename_component(UNIT_TEST_REPORTS_DIR
    "${CMAKE_BINARY_DIR}/${UNIT_TEST_REPORTS_DIR}"
    ABSOLUTE
  )
endif()

# ===
# === Include some external files
# ===
include(CMakeParseArguments)
include(EnhancedList)
include(HelpersArguments)

cmake_policy(PUSH)

foreach(_policy CMP0053 CMP0026)
  if(POLICY ${_policy})
    cmake_policy(SET ${_policy} OLD)
  endif()
endforeach()

include(CreateLaunchers OPTIONAL RESULT_VARIABLE HasCreateLaunchers_)

if(HasCreateLaunchers_)
  # Overwrite some functions called by the create_target_launcher(...)
  if(MSVC)
    # Overwrite _launcher_produce_vcproj_user(...) to no longer escape quotes
    macro(_launcher_produce_vcproj_user)
        file(READ
          "${_launchermoddir}/perconfig.${VCPROJ_TYPE}.user.in"
          _perconfig)
        set(USERFILE_CONFIGSECTIONS)
        foreach(USERFILE_CONFIGNAME ${CMAKE_CONFIGURATION_TYPES})
          if(NOT USERFILE_COMMAND)
            get_target_property(USERFILE_${USERFILE_CONFIGNAME}_COMMAND
              ${_targetname}
              LOCATION_${USERFILE_CONFIGNAME})
          else()
            set(USERFILE_${USERFILE_CONFIGNAME}_COMMAND "${USERFILE_COMMAND}")
          endif()
          file(TO_NATIVE_PATH
            "${USERFILE_${USERFILE_CONFIGNAME}_COMMAND}"
            USERFILE_${USERFILE_CONFIGNAME}_COMMAND)
          string(CONFIGURE "${_perconfig}" _temp @ONLY)
          string(CONFIGURE
            "${USERFILE_CONFIGSECTIONS}${_temp}"
            USERFILE_CONFIGSECTIONS)
        endforeach()


        configure_file("${_launchermoddir}/${VCPROJ_TYPE}.user.in"
          ${VCPROJNAME}.${VCPROJ_TYPE}.${USERFILE_EXTENSION}
          @ONLY)
    endmacro()
  endif()

  # Overwrite _launcher_process_args(...) to:
  #  - replace arguments list's separator ';' with space. Otherwise, command
  #    line fails on execution of launcher
  #  - add support for COMMAND option
  macro(_launcher_process_args)
    CMake_Parse_Arguments("USERFILE"
                          ""
                          "COMMAND"
                          ""
                          ${ARGN})
    __launcher_process_args(${USERFILE_UNPARSED_ARGUMENTS})
     # __launcher_process_args(${ARGN})
    foreach(_var USERFILE_COMMAND_ARGUMENTS LAUNCHERSCRIPT_COMMAND_ARGUMENTS)
      string(REGEX REPLACE "([^\\\\]|);" "\\1 " ${_var} "${${_var}}")
    endforeach()
  endmacro()
else()
  function(create_target_launcher)
  endfunction()
endif()

cmake_policy(POP)

# ===
# === Set up some parameters
# ===

# --- Enable IDE to display projects' and files' groups
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ===
# === Define helper functions
# ===

# ---
# --- apply_properties(<SCOPE> <ENTITY> prop1=value1 [prop2=value2] ...)
# ---
# --- <SCOPE> can be any name from <DIRECTORY|TARGET|SOURCE|TEST|CACHE>
# --- <ENTITY> name of the entity in <SCOPE> the properties are to applied to
# ---
function(apply_properties scope entity)
    string(TOUPPER "${scope}" scope)
    if(NOT scope MATCHES "(GLOBAL|DIRECTORY|TARGET|SOURCE|TEST|CACHE)")
        message(FATAL_ERROR "Invalid properties scope '${scope}'")
    endif()

    if(NOT ARGN)
        return()
    endif()

    foreach(prop_ ${ARGN})
        if(NOT prop_ MATCHES "^([^=]+[^+:~])([+:~]|)=(.+)$")
            message(FATAL_ERROR "Malformed ${scope} PROPERTY '${prop_}'")
        endif()

        set(prop_name_ "${CMAKE_MATCH_1}")
        set(prop_action_ "${CMAKE_MATCH_2}")
        set(prop_value_ "${CMAKE_MATCH_3}")

        string(REGEX REPLACE "\"((\\\\\"|[^\"])*)\"" "\\1" prop_value_ ${prop_value_})
        string(REPLACE "\\\"" "\"" prop_value_ ${prop_value_})

        if(prop_action_)
            get_property(_old_value ${scope} "${entity}" PROPERTY ${prop_name_})
            if(_old_value)
                if("${prop_action_}" STREQUAL ":")
                    unset(prop_value_)
                elseif("${prop_action_}" STREQUAL "+")
                    set(prop_action_ "APPEND")
                elseif("${prop_action_}" STREQUAL "~")
                    set(prop_action_ "APPEND_STRING")
                else()
                    unset(prop_action_)
                endif()
            else()
                unset(prop_action_)
            endif()
        else()
            unset(prop_action_)
        endif()

        if(prop_value_)
            set_property(${scope} "${entity}" ${prop_action_} PROPERTY ${prop_name_} ${prop_value_})
        endif()
    endforeach()
endfunction()

# ---
# --- Internal helper to create unit test linked to created executable target
# ---
function(_setup_unit_test target_name)
  set(_tests_group "${_ATG_GROUP}/All")

  # Associate given target to 'build_tests' global target
  if(NOT TARGET build_tests)
    add_custom_target(build_tests)
    project_group("${_tests_group}" build_tests)
  endif()
  add_dependencies(build_tests "${target_name}")

  # Set-up a run_tests to allow verbose output
  if(NOT TARGET run_build_tests)
    add_custom_target(run_build_tests
      COMMAND "${CMAKE_CTEST_COMMAND}" "-V"
      DEPENDS build_tests
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    project_group("${_tests_group}" run_build_tests)
  endif()

  # Prepare context for CMake's test target
  set(_args ${_ATG_TEST_ARGS})
  if(TARGET gtest_main)
    # Report the need to link against gtest_main library
    target_link_libraries(${target_name} gtest_main)

    # Check all gtest_main include directories
    target_include_directories(${target_name}
      PRIVATE
      "$<TARGET_PROPERTY:gtest_main,INCLUDE_DIRECTORIES>"
    )

    # Complete list of arguments to provide to unit test
    list(INSERT _args 0
      "--gtest_output=xml:${UNIT_TEST_REPORTS_DIR}/${target_name}.xml"
    )
  endif()

  # Create CMake's test target
  add_test(${target_name}_test ${target_name} ${_args})
  apply_properties(TEST ${target_name}_test ${_ATG_TEST_PROPERTIES})

  # Create targets for all sub-tests
  if(ENABLE_SUBTEST_TARGETS AND TARGET gtest_main AND HasCreateLaunchers_)
    set(_reports_base_dir
      "${CMAKE_CURRENT_BINARY_DIR}/gtest_reports/${target_name}"
    )
    foreach(source_file IN LISTS _ATG_FILES)
      get_filename_component(_source_reports_dir "${source_file}" NAME_WE)
      set(_source_reports_dir "${_reports_base_dir}/${_source_reports_dir}")

      file(STRINGS "${source_file}" source_file_tests
        REGEX "TEST_?F?\\(([A-Za-z_0-9 \t,]+)\\)"
      )

      set(_tests_filters
        ".*\\([ \t]*([A-Za-z_0-9]+)[ \t]*,[ \t]*([A-Za-z_0-9]+)[ \t]*\\).*"
      )
      foreach(test_case IN LISTS source_file_tests)
        if("${test_case}" MATCHES "${_tests_filters}")
          set(_family "${CMAKE_MATCH_1}")
          set(_case   "${CMAKE_MATCH_2}")
          set(test_case "${_family}.${_case}")
          if(NOT TARGET "${test_case}")
            set(_family_reports_dir "${_source_reports_dir}/${_family}")

            add_custom_target(${test_case}
              SOURCES
                "${source_file}"
              DEPENDS
                ${target_name}
            )
            project_group("${_tests_group}/details/${_family}"
              TARGET ${test_case}
              LABEL  ${_case}
            )

            cmake_policy(PUSH)
            cmake_policy(SET CMP0026 OLD)
            foreach(_config "" ${CMAKE_CONFIGURATION_TYPES})
              set(_config LOCATION ${_config})
              string(REGEX REPLACE "([^\\\\]|);" "\\1_" _config "${_config}")
              get_target_property(_prop ${target_name} ${_config})
              set_target_properties(${test_case} PROPERTIES ${_config} "${_prop}")
            endforeach()
            cmake_policy(POP)

            create_target_launcher(${test_case}
              ARGS
                --gtest_filter="${test_case}"
                --gtest_output="xml:${_family_reports_dir}/${_case}.xml"
                ${_ATG_TEST_ARGS}
              ${_ATG_TEST_PROPERTIES}
            )
          endif()
        endif()
      endforeach()
    endforeach()
  endif()
endfunction()

# ---
# --- add_target(<target_name>
# ---     TYPE                  <EXECUTABLE> [TEST|WIN32|MACOSX_BUNDLE|EXCLUDE_FROM_ALL|RUNTIME_STATIC]
# ---                           or
# ---                           <LIBRARY> [OBJECT|SHARED|MODULE|STATIC|EXCLUDE_FROM_ALL|RUNTIME_STATIC]
# ---     FILES                 <list_of_target's files>
# ---     [PREFIX_SKIP          <prefix_regex>]                  (default '(\./)?(src|sources)')
# ---     [SOURCE_GROUP_NAME    <name_of_source_files' group>]   (default 'Source Files')
# ---     [HEADER_GROUP_NAME    <name_of_header_files' group>]   (default 'Source Files')
# ---     [RESOURCE_GROUP_NAME  <name_of_resource_files' group>] (default 'Resource Files')
# ---     [SOURCE_FILTER        <source_files' regex_filter>]    (default '\.(c(\+\+|xx|pp|c)?|C|M|mm)')
# ---     [HEADER_FILTER        <header_files' regex_filter>]    (default '\.h(h|m|pp|\+\+)?')
# ---     [GROUP                <target_group_tree_in_ide>]      (default 'Executable' or 'Libraries')
# ---     [LABEL                <target_label_in_ide>]
# ---     [LINKS                <list_of_libraries_to_link_with>]
# ---     [DEPENDS              <list_of_dependencies>]
# ---     [DEFINITIONS          <list_of_definitions>] (cf. target_compile_definitions)
# ---     [INCLUDE_DIRECTORIES  <list_of_include_directories>] (cf. target_include_directories)
# ---     [COMPILE_OPTIONS      <list_of_options>] (cf. target_compile_options)
# ---     [PROPERTIES           <List_of_properties> (syntax PROP1=VALUE1 [[PROP2=VALUE2]...])
# ---     [LAUNCHER             <cf. create_target_launcher() documentation']
# ---     [TEST_ARGS            <List_of_Test's specific_arguments>
# ---     [TEST_PROPERTIES      <List_of_Test's specific properties> (syntax PROP1=VALUE1 [[PROP2=VALUE2]...])
# ---     [INSTALL]             Add target to global install directive
# ---     [INSTALL_PATH         <installation_path>]
# --- )
# ---
# --- N.B.: <target_name>  is the name of the Target to create
# ---       <target_flags> is a set of flags specific to the Target (cf documentation of
# ---                      'add_executable' and 'add_library' fro more precisions)
# ---       Properties' syntax has to follow rule:
# ---           <PROP_NAME> <ASSIGNMENT_OPERATOR> <PROP_VALUE>
# ---             <PROP_NAME> is the name of the property
# ---             <PROP_VALUE> is the value to assign to the property
# ---             <ASSIGNMENT_OPERATOR> is how value has to be assign to the property
# ---                 '=' set the property with the given value
# ---                 ':=' set the property with the given value only and only if property is not already set
# ---                 '+=' add the given value to the property as a list element
# ---                 '~=' concat the given value to the string property
# ---
# --- Creates a target (executable or library) whose files will be grouped into
# --- IDE's virtual folders (sources, headers and resources)
# ---
# --- Example:
# --- add_target(myTargetLib
# ---     TYPE
# ---         LIBRARY STATIC
# ---     FILES
# ---         "src/files/gui/main.cpp"
# ---         "src/files/gui/dialog.cpp"
# ---         "src/files/tools/convert.cpp"
# ---         "src/files/gui/dialog.h"
# ---         "src/files/tools/convert.h"
# ---     PREFIX_SKIP
# ---         "src/files"
# ---     DEFINITIONS
# ---         MY_DEF1=VAL1
# ---         MY_DEF2=VAL2
# ---         MY_NO_VAL_DEF
# --- )
# --- 
# --- will create an target for an executable named "myTarget". For the IDE, the
# --- target will be composed of virtual groups tree that hold the files:
# ---     myTargetLib
# ---     |- Source Files
# ---     |  |- gui
# ---     |  |  |- main.cpp
# ---     |  |  \- dialog.cpp
# ---     |  |
# ---     |  \- tools
# ---     |     \-  convert.cpp
# ---     |
# ---     \- HeaderFiles
# ---        |- gui
# ---        |  \- dialog.h
# ---        |
# ---        \- tools
# ---           \-  convert.h
# ---        
# --- N.B.: If the PREFIX_SKIP parameter is omitted, then the above result would
# ---       be:
# ---     myTargetLib
# ---     |- Source Files
# ---     |  \- files
# ---     |     |- gui
# ---     |     |  |- main.cpp
# ---     |     |  \- dialog.cpp
# ---     |     |
# ---     |     \- tools
# ---     |        \-  convert.cpp
# ---     |
# ---     \- HeaderFiles
# ---        \- files
# ---            |- gui
# ---            |  \- dialog.h
# ---            |
# ---            \- tools
# ---               \-  convert.h
function(add_target _target_name)
    # --- Parse arguments
    set(executable_options "WIN32;MACOSX_BUNDLE")   # Target Executable specific
    set(library_options    "STATIC;SHARED;MODULE")  # Target Library specific
    set(all_options
        ${executable_options}
        ${library_options}
        "TEST"              # Consider target as a unit test
        "OBJECT"            # Consider target as a set of compiled objects
        "EXCLUDE_FROM_ALL"  # Exclude target from global build (i.e. 'all' rule)
        "RUNTIME_STATIC"    # Link with static runtime
        "INSTALL"           # Add target to install directive
    )

    set(all_one_arg
        "TYPE"
        "GROUP;LABEL"
        "SOURCE_GROUP_NAME;HEADER_GROUP_NAME;RESOURCE_GROUP_NAME"
        "PREFIX_SKIP;SOURCE_FILTER;HEADER_FILTER"
        "INSTALL_PATH"
    )

    set(all_set_args
        "FILES"
        "LINKS;DEPENDS"
        "DEFINITIONS;PROPERTIES;INCLUDE_DIRECTORIES;COMPILE_OPTIONS"
        "TEST_ARGS;TEST_PROPERTIES"
    )

    if(HasCreateLaunchers_)
        list(APPEND all_set_args "LAUNCHER")
    endif()

    parse_arguments("_ATG"
                    "${all_options}"
                    "${all_one_arg}"
                    "${all_set_args}"
                    ""
                    ${ARGN})

    if(_ATG_UNPARSED_ARGUMENTS)
      list(APPEND _ATG_FILES "${_ATG_UNPARSED_ARGUMENTS}")
    endif()

    # --- Check parsed arguments
    if(NOT _target_name)
        message(FATAL_ERROR "No target specified")
    endif()

    if(NOT _ATG_TYPE)
        # No type specified. Try to guess it...
        if(_ATG_TEST)
            set(_ATG_TYPE "EXECUTABLE")
        elseif(_ATG_OBJECT)
            set(_ATG_TYPE "LIBRARY")
        else()
            # Check type's specific options
            foreach(_mode executable library)
                foreach(_option ${${_mode}_options})
                  if(_ATG_${_option})
                    set(_ATG_TYPE "${_mode}")
                    break()
                  endif()
                endforeach()
                if(_ATG_TYPE)
                  break()
                endif()
            endforeach()

            if(NOT _ATG_TYPE)
                message(FATAL_ERROR "No type has been specified to target '${_target_name}'")
            endif()
        endif()
    endif()
    string(TOUPPER "${_ATG_TYPE}" _ATG_TYPE)

    if((NOT _ATG_TYPE STREQUAL "EXECUTABLE") AND (NOT _ATG_TYPE STREQUAL "LIBRARY"))
        message(FATAL_ERROR "Invalid type '${_ATG_TYPE}' specified to target '${_target_name}'")
    endif()

    if(NOT _ATG_FILES)
        message(FATAL_ERROR "No files specified for target '${_target_name}'")
    endif()

    if(_ATG_TEST_ARGS AND NOT _ATG_TEST)
        message(FATAL_ERROR "Target '${_target_name}' is not a Test. Test arguments have been specified.")
    endif()

    if(_ATG_TEST_PROPERTIES)
      if(NOT _ATG_TEST)
        message(FATAL_ERROR "Target '${_target_name}' is not a Test. Test properties have been specified.")
      endif()
    endif()

    if(NOT _ATG_PREFIX_SKIP)
        set(_ATG_PREFIX_SKIP "(\\./)?(src|sources)")
    endif()
    
    if(NOT _ATG_SOURCE_GROUP_NAME)
        set(_ATG_SOURCE_GROUP_NAME "Source Files")
    endif()
    
    if(NOT _ATG_HEADER_GROUP_NAME)
        set(_ATG_HEADER_GROUP_NAME "Source Files")
    endif()

    if(NOT _ATG_RESOURCE_GROUP_NAME)
        set(_ATG_RESOURCE_GROUP_NAME "Resource Files")
    endif()
    
    if(NOT _ATG_SOURCE_FILTER)
        set(_ATG_SOURCE_FILTER "\\.(c(\\+\\+|xx|pp|c)?|C|M|mm)")
    endif()
    
    if(NOT _ATG_HEADER_FILTER)
        set(_ATG_HEADER_FILTER "\\.h(h|m|pp|\\+\\+)?")
    endif()

    if(_ATG_INSTALL_PATH AND NOT _ATG_INSTALL)
      set(_ATG_INSTALL ON)
    endif()

    set(_ATG_TARGET_OPTIONS)

    if(_ATG_TEST AND NOT _ATG_TYPE STREQUAL "EXECUTABLE")
        message(FATAL_ERROR
            "Invalid option 'TEST' for '${_ATG_TYPE}' target '${_target_name}'")
    endif()

    foreach(_option ${executable_options})
        if(_ATG_${_option})
            if(NOT _ATG_TYPE STREQUAL "EXECUTABLE")
                message(FATAL_ERROR
                  "Invalid option '${_option}' for '${_ATG_TYPE}' target '${_target_name}'")
            endif()
            list(APPEND _ATG_TARGET_OPTIONS ${_option})
        endif()
    endforeach()

    if(_ATG_OBJECT)
        if(NOT _ATG_TYPE STREQUAL "LIBRARY")
            message(FATAL_ERROR
                "Invalid option 'OBJECT' for '${_ATG_TYPE}' target '${_target_name}'")
        endif()
        list(APPEND _ATG_TARGET_OPTIONS OBJECT)
    endif()

    foreach(_option ${library_options})
        if(_ATG_${_option})
            if(NOT _ATG_TYPE STREQUAL "LIBRARY" OR _ATG_OBJECT)
                message(FATAL_ERROR
                  "Invalid option '${_option}' for '${_ATG_TYPE}' target '${_target_name}'")
            endif()
            list(APPEND _ATG_TARGET_OPTIONS ${_option})
        endif()
    endforeach()

    if(_ATG_EXCLUDE_FROM_ALL OR (_ATG_TEST AND NOT _ATG_INSTALL))
        list(APPEND _ATG_TARGET_OPTIONS EXCLUDE_FROM_ALL)
    endif()

    if(MSVC)
      if(_ATG_RUNTIME_STATIC)
          list(APPEND _ATG_COMPILE_OPTIONS PRIVATE "/MT$<$<CONFIG:Debug>:d>")
      else()
          list(APPEND _ATG_COMPILE_OPTIONS PRIVATE "/MD$<$<CONFIG:Debug>:d>")
      endif()
    endif()

    # --- Link each files to a filter/group
    foreach(_file_path ${_ATG_FILES})
        get_filename_component(_sub_group "${_file_path}" PATH)
        if("${_sub_group}" MATCHES "^${CMAKE_CURRENT_SOURCE_DIR}/*(.*)$")
            set(_sub_group "${CMAKE_MATCH_1}")
        elseif("${_sub_group}" MATCHES "^${CMAKE_SOURCE_DIR}/*(.*)$")
            set(_sub_group "${CMAKE_MATCH_1}")
        endif()

        if (_ATG_PREFIX_SKIP)
            string(REGEX REPLACE "^${_ATG_PREFIX_SKIP}/*" "" _sub_group "${_sub_group}")
        endif()

        if(_file_path MATCHES "^.*${_ATG_SOURCE_FILTER}$")
            set(_sub_group "${_ATG_SOURCE_GROUP_NAME}/${_sub_group}")
        elseif(_file_path MATCHES "^.*${_ATG_HEADER_FILTER}$")
            set(_sub_group "${_ATG_HEADER_GROUP_NAME}/${_sub_group}")
        else()
            set(_sub_group "${_ATG_RESOURCE_GROUP_NAME}/${_sub_group}")
        endif()

        if(MSVC)
            string(REPLACE "/" "\\" _sub_group "${_sub_group}")
        endif()

        source_group("${_sub_group}" FILES "${_file_path}")
    endforeach()
    
    # --- Create Target
    if(_ATG_TYPE STREQUAL "EXECUTABLE")
        add_executable(${_target_name} ${_ATG_TARGET_OPTIONS}
            ${_ATG_FILES}
        )
        if(NOT _ATG_TEST AND NOT _ATG_GROUP)
            set(_ATG_GROUP "Executables")
        elseif(_ATG_TEST)
            if(NOT _ATG_GROUP)
                set(_ATG_GROUP "Unit Tests")
            endif()
            _setup_unit_test(${_target_name})
        endif()
    else()
        add_library(${_target_name} ${_ATG_TARGET_OPTIONS}
            ${_ATG_FILES}
        )
        if(NOT _ATG_GROUP)
            set(_ATG_GROUP "Libraries")
        endif()
    endif()

    # --- Declare target's dependencies and set-up a custom property "DEPENDS"
    if(_ATG_DEPENDS)
        add_dependencies(${_target_name} ${_ATG_DEPENDS})
        set_property(TARGET "${_target_name}" PROPERTY DEPENDS ${_ATG_DEPENDS})
    endif()

    # --- Declare target's links and set-up a custom property "LINKS"
    if(_ATG_LINKS)
        target_link_libraries(${_target_name} ${_ATG_LINKS})
        set_property(TARGET "${_target_name}" PROPERTY LINKS ${_ATG_LINKS})
    endif()
    
    # -- Set-up some target's properties
    set_property(TARGET "${_target_name}" PROPERTY FOLDER "${_ATG_GROUP}")
    
    if(_ATG_LABEL)
        set_property(TARGET "${_target_name}" PROPERTY PROJECT_LABEL "${_ATG_LABEL}")
    endif()

    if(_ATG_DEFINITIONS)
        if(NOT "${_ATG_DEFINITIONS}" MATCHES "^(INTERFACE|PUBLIC|PRIVATE);.*$")
          list(INSERT _ATG_DEFINITIONS 0 "PRIVATE")
        endif()
        target_compile_definitions("${_target_name}" ${_ATG_DEFINITIONS})
    endif()

    if(_ATG_INCLUDE_DIRECTORIES)
        set(_pattern "^(((SYSTEM|BEFORE);)+|)((INTERFACE|PUBLIC|PRIVATE);|)(.*)$")
        if("${_ATG_INCLUDE_DIRECTORIES}" MATCHES "${_pattern}")
            set(_prefix "${CMAKE_MATCH_1}")
            set(_suffix "${CMAKE_MATCH_6}")
            if(NOT "${CMAKE_MATCH_5}" MATCHES "^(INTERFACE|PUBLIC|PRIVATE)$")
                set(_ATG_INCLUDE_DIRECTORIES "${_prefix}PRIVATE;${_suffix}")
            endif()
            target_include_directories("${_target_name}" ${_ATG_INCLUDE_DIRECTORIES})
        else()
          message(FATAL_ERROR "Invalid declaration for include directory")
        endif()
    endif()

    if(_ATG_COMPILE_OPTIONS)
        set(_pattern "^((BEFORE;)+|)((INTERFACE|PUBLIC|PRIVATE);|)(.*)$")
        if("${_ATG_COMPILE_OPTIONS}" MATCHES "${_pattern}")
            set(_prefix "${CMAKE_MATCH_1}")
            set(_suffix "${CMAKE_MATCH_5}")
            if(NOT "${CMAKE_MATCH_4}" MATCHES "^(INTERFACE|PUBLIC|PRIVATE)$")
                set(_ATG_COMPILE_OPTIONS "${_prefix}PRIVATE;${_suffix}")
            endif()
            target_compile_options("${_target_name}" ${_ATG_COMPILE_OPTIONS})
        else()
          message(FATAL_ERROR "Invalid declaration for compile options")
        endif()
    endif()

    if(_ATG_PROPERTIES)
      apply_properties(TARGET ${_target_name} ${_ATG_PROPERTIES})
    endif()

    if(HasCreateLaunchers_ AND _ATG_LAUNCHER)
        create_target_launcher(${_target_name} ${_ATG_LAUNCHER})
    endif()

    # -- Set-up  target's installation rules
    if(_ATG_INSTALL OR _ATG_INSTALL_PATH)
        if(NOT _ATG_INSTALL_PATH)
            if(_ATG_TEST)
                set(_ATG_INSTALL_PATH tests)
            else()
                set(_ATG_INSTALL_PATH
                    bin                      # Default destination
                    ARCHIVE DESTINATION lib  # Specific destination for static libraries
                )
            endif()
        endif()

        install(TARGETS ${_target_name}
            DESTINATION ${_ATG_INSTALL_PATH}
        )
    endif()
endfunction(add_target)

# ---
# --- project_group(<group_tree> [<list_of_targets> | TARGET <target_name> LABEL <target_name_in_group>])
# ---
# --- Set target(s) as being hold by a specific project group
# ---
# --- A Target associated to a tree will appear under the associated project group in the IDE.
# ---
# --- Example:
# --- project_group(
# ---     "tools/commandlines"
# ---     TARGET "myTarget" LABEL "theTool"
# ---     
# --- )
# --- 
# --- will create a virtual project group tree to hold the target:
# ---     tools                      \ 
# ---     |                          | -> Create group tree
# ---     \-  commandlines           /
# ---         |
# ---         \-  theTool            | -> Displayed name for the target
# ---
function(project_group _group)
    CMake_Parse_Arguments("_PG" "" "TARGET;LABEL" "" ${ARGN})
    if(NOT _group)
        message(FATAL_ERROR "No group name/path specified")
    endif()

    if(_PG_TARGET AND _PG_LABEL AND (NOT _PG_UNPARSED_ARGUMENTS))
        if (NOT TARGET ${_PG_TARGET})
            message(FATAL_ERROR "Invalid target's name '${_PG_TARGET}' added to project group '${_group}'")
        endif()
        set_property(TARGET "${_PG_TARGET}" PROPERTY FOLDER "${_group}")
        set_property(TARGET "${_PG_TARGET}" PROPERTY PROJECT_LABEL "${_PG_LABEL}")
    elseif((NOT _PG_TARGET) AND (NOT _PG_LABEL) AND _PG_UNPARSED_ARGUMENTS)
        foreach(_tgt ${_PG_UNPARSED_ARGUMENTS})
            if (NOT TARGET ${_tgt})
                message(FATAL_ERROR "Invalid target's name '${_tgt}' added to project group '${_group}'")
            endif()
            set_property(TARGET "${_tgt}" PROPERTY FOLDER "${_group}")
        endforeach()
    else()
        message(FATAL_ERROR "Invalid target parameters '${ARGN}' while added to project group '${_group}'")
    endif()

endfunction(project_group)

# ---
# --- file_group(<group_name>
# ---     FILES         <list_of_files_for_target>
# ---     [PREFIX_SKIP  <prefix_regex>]             (default to '(\./)?src')
# --- )
# ---
# --- Associates a list of files to a virtual group tree
# ---
# --- Example:
# --- file_group("Sources"
# ---     PREFIX_SKIP     "\\.\\./src"
# ---
# ---     FILES
# ---         ../src/gui/main.cpp
# ---         ../src/gui/dialog.cpp
# ---         ../src/tools/convert.cpp
# --- )
# --- 
# --- will create a virtual group tree to hold the files:
# ---     Source
# ---     |-  gui
# ---     |   |-  main.cpp
# ---     |   \-  dialog.cpp
# ---     |
# ---     \-  tools
# ---         \-  convert.cpp
# ---        
# --- N.B.: If the PREFIX_SKIP parameter is omitted, then the above result would had been:
# ---     Source
# ---     \-  ..
# ---         \-  src
# ---             |-  gui
# ---             |   |-  main.cpp
# ---             |   \-  dialog.cpp
# ---             |
# ---             \-  tools
# ---                 \-  convert.cpp
# ---
function(file_group _group)
    CMake_Parse_Arguments("_FG" "" "PREFIX_SKIP" "FILES" ${ARGN})
    list(APPEND _FG_FILES "${_FG_UNPARSED_ARGUMENTS}")
    if(NOT _FG_PREFIX_SKIP)
        set(_FG_PREFIX_SKIP "(\\./)?(sources|src)")
    endif()
    
    foreach(_file_path ${_FG_FILES})
        get_filename_component(_sub_group "${_file_path}" PATH)
        if (_FG_PREFIX_SKIP)
            string(REGEX REPLACE "^${_FG_PREFIX_SKIP}/*" "" _sub_group "${_sub_group}")
        endif()

        if (_group)
            set(_sub_group "${_group}/${_sub_group}")
        endif()

        if(MSVC)
            string(REPLACE "/" "\\" _sub_group "${_sub_group}")
        endif()
        source_group("${_sub_group}" FILES "${_file_path}")
    endforeach()
endfunction(file_group)

# ---
# --- enable_coding_style(
# ---     [FILES   <list_of_files>]
# ---     [TARGETS <list_of_targets>]
# --- )
# ---
# --- Proceed to coding style check on each files or targets' file each time
# --- they are modified
# ---
# --- Examples:
# --- enable_coding_style(
# ---     FILES
# ---         ../src/gui/main.cpp
# ---         ../src/gui/dialog.cpp
# ---         ../src/tools/convert.cpp
# --- )
# ---
# --- will check coding style on specifed files.
# ---
# --- enable_coding_style(
# ---     TARGETS
# ---         target_1
# ---         target_2
# --- )
# ---
# --- will check coding style on all files associated to the specified targets.
# ---
# --- enable_coding_style(
# ---     TARGETS
# ---         target_1
# ---         target_2
# ---     FILES
# ---         ../src/gui/main.cpp
# ---         ../src/gui/dialog.cpp
# ---         ../src/tools/convert.cpp
# --- )
# ---
# --- will check coding style on:
# ---   + all files associated to the specified targets.
# ---   + all files specified
# ---
function(enable_coding_style)
    CMake_Parse_Arguments("_CS" "" "" "TARGETS;FILES" ${ARGN})

    if(NOT (_CS_TARGETS OR _CS_FILES))
      message(FATAL_ERROR "Neither targets nor files specified!!")
    endif()

    # --- Check availability of Google's 'cpplint' script
    if(NOT DEFINED HAS_CPP_LINT)
      find_program(HAS_CPP_LINT cpplint.py)
      set(HAS_CPP_LINT "${HAS_CPP_LINT}" CACHE FILEPATH "Path to 'cpplint.py' Google's script")
    endif()

    if(NOT HAS_CPP_LINT)
      message("### No CPP Lint program available")
      return()
    endif()

    # --- Complete Files list with targets' ones
    if(_CS_TARGETS)
        foreach(target_ ${_CS_TARGETS})
          get_property(target_ TARGET "${target_}" PROPERTY SOURCES)
          list(APPEND _CS_FILES "${target_}")
        endforeach()
    endif()

    # --- Parse each given files
    foreach(file_ ${_CS_FILES})
      get_filename_component(name_ "${file_}" NAME)
      get_filename_component(file_ "${file_}" ABSOLUTE)
      set(lint_ "${CMAKE_CURRENT_BINARY_DIR}/${name_}.lint")

      # --- Define a Rule to start cpplint
      add_custom_command(
        OUTPUT "${lint_}"
        DEPENDS "${file_}"
        COMMENT "Checking coding style: ${name_}"
        COMMAND "${HAS_CPP_LINT}" --output=vs7 --verbose=-1 "\"${file_}\""
        COMMAND "${CMAKE_COMMAND}" -E touch "\"${lint_}\""
      )

      # --- Set file's dependencies
      get_source_file_property(prop_ "${file_}" OBJECT_DEPENDS)
      if(prop_)
        set(prop_ "${prop_};")
      else()
        set(prop_)
      endif()
      set_source_files_properties("${file_}"
        PROPERTIES
          OBJECT_DEPENDS "${prop_}${lint_}"
      )
    endforeach()
endfunction(enable_coding_style)

# ---
# --- enable_target_linked_artefact_copy(
# ---   <list_of_targets>
# --- )
# ---
# --- Set-up targets to copy all artefacts of linked modules or shared libraries
# --- in their respective binary directory hosting their artefact.
# ---
# --- This feature can be useful when running application in debug mode
# ---
function(enable_target_linked_artefact_copy target)
  foreach(_target ${target} ${ARGN})
    # build complete (recursive) list of linked artefacts
    set(_dep_artefacts ${_target})  # Add name of target to avoid infinite loop
    _build_linked_artefact_list(${_target} _dep_artefacts)
    list(REMOVE_AT _dep_artefacts 0)
    if(_dep_artefacts)
      # Set-up copy command for each linked artefact
      unset(_copy_commands)
      foreach(_artefact IN LISTS _dep_artefacts)
        list(APPEND _copy_commands
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                        $<TARGET_FILE:${_artefact}>
                                        $<TARGET_FILE_DIR:${_target}>
        )
      endforeach()

      # Declare copy command to the specified target
      add_custom_command(TARGET ${_target} POST_BUILD ${_copy_commands})
    endif()
  endforeach()
endfunction()

# ---
# --- _build_linked_artefact_list(
# ---   <target>
# ---   <dep_artefacts>
# --- )
# ---
# --- Helper to function 'enable_target_linked_artefact_copy(...)' to build list
# --- of linked artefacts
# ---
function(_build_linked_artefact_list target dep_artefacts)
  get_target_property(_link_libraries ${target} LINK_LIBRARIES)
  if(_link_libraries)
    foreach(_link IN LISTS _link_libraries)
      if(TARGET ${_link})
        get_target_property(_target_type ${_link} TYPE)
        foreach(_type MODULE_LIBRARY SHARED_LIBRARY)
          if("${_type}" STREQUAL "${_target_type}")
            list_join(${dep_artefacts} "|" _filter)
            if(NOT "${_link}" MATCHES "^(.*[^\\\\]\;|)(${_filter})(\;.*|)$")
              _build_linked_artefact_list(${_link} ${dep_artefacts})
              list(APPEND ${dep_artefacts} ${_link})
            endif()
            break()
          endif()
        endforeach()
      endif()
    endforeach()

    set(${dep_artefacts} "${${dep_artefacts}}" PARENT_SCOPE)
  endif()
endfunction()
