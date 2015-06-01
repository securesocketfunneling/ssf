#
# Define some helpers for IDE's Groups management
#
if(__H_HELPER_IDE_GROUP_INCLUDED)
  return()
endif()
set(__H_HELPER_IDE_GROUP_INCLUDED TRUE)

# ===
# === Include some external files
# ===
include(CMakeParseArguments)

# ===
# === Set up some parameters
# ===

# --- Enable IDE to display projects' and files' groups
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ===
# === Define helper functions
# ===

# ---
# --- add_target(<target_name>
# ---     TYPE                  <type_of_target> [<target_flags>]   (EXECUTABLE or LIBRARY followed by optional flags [WIN32|MACOSX_BUNDLE|SHARED|MODULE|STATIC|EXCLUDE_FROM_ALL])
# ---     FILES                 <list_of_files_for_target>
# ---     [PREFIX_SKIP          <prefix_regex>]                     (default to '(\./)?src')
# ---     [SOURCE_GROUP_NAME    <name_of_group_for_source_files>]   (default to 'Source Files')
# ---     [HEADER_GROUP_NAME    <name_of_group_for_header_files>]   (default to 'Header Files')
# ---     [RESOURCE_GROUP_NAME  <name_of_group_for_resource_files>] (default to 'Resource Files')
# ---     [SOURCE_FILTER        <regex_filter_for_source_files>]    (default to '\.(c(\+\+|xx|pp|c)?|C|M|mm)')
# ---     [HEADER_FILTER        <regex_filter_for_header_files>]    (default to '\.h(h|m|pp|\+\+)?')
# ---     [GROUP                <target_group_tree_in_ide>]         (default to 'Executable' or 'Libraries')
# ---     [LABEL                <target_label_in_ide>]
# ---     [LINKS                <list_of_libraries_to_link_with>]
# ---     [DEPENDS              <list_of_dependencies>]
# ---     [DEFINITIONS          <list_of_definitions>]
# --- )
# ---
# --- N.B.: <target_name> is the name of the Target to create
# ---       <target_flags> is a set of flags specific to the Target (cf documentation of 'add_executable' and 'add_library' fro more precisions)
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
    set(exec_options "WIN32;MACOSX_BUNDLE")
    set(lib_options "STATIC;SHARED;MODULE")
    
    CMake_Parse_Arguments("_ATG"
                          "${exec_options};${lib_options};EXCLUDE_FROM_ALL"
                          "TYPE;LABEL;GROUP;PREFIX_SKIP;SOURCE_GROUP_NAME;HEADER_GROUP_NAME;RESOURCE_GROUP_NAME;SOURCE_FILTER;HEADER_FILTER"
                          "FILES;LINKS;DEPENDS;DEFINITIONS"
                          ${ARGN})

    list(APPEND _ATG_FILES "${_ATG_UNPARSED_ARGUMENTS}")
    
    # --- Check parsed arguments
    if(NOT _target_name)
        message(FATAL_ERROR "No target specified")
    endif()

    if(NOT _ATG_TYPE)
        message(FATAL_ERROR "No type has been specified to target '${_target_name}'")
    endif()
    string(TOUPPER "${_ATG_TYPE}" _ATG_TYPE)
    
    if((NOT _ATG_TYPE STREQUAL "EXECUTABLE") AND (NOT _ATG_TYPE STREQUAL "LIBRARY"))
        message(FATAL_ERROR "Invalid type '${_ATG_TYPE}' specified to target '${_target_name}'")
    endif()

    if(NOT _ATG_FILES)
        message(FATAL_ERROR "No files specified for target '${_target_name}'")
    endif()

    if(NOT _ATG_PREFIX_SKIP)
        set(_ATG_PREFIX_SKIP "(\\./)?src")
    endif()
    
    if(NOT _ATG_SOURCE_GROUP_NAME)
        set(_ATG_SOURCE_GROUP_NAME "Source Files")
    endif()
    
    if(NOT _ATG_HEADER_GROUP_NAME)
        set(_ATG_HEADER_GROUP_NAME "Header Files")
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
    
    set(_ATG_TARGET_OPTIONS)
    foreach(_option ${exec_options})
        if(_ATG_${_option})
            if(NOT _ATG_TYPE STREQUAL "EXECUTABLE")
                message(FATAL_ERROR "Invalid option '${_option}' for '${_ATG_TYPE}' target '${_target_name}'")
            endif()
            list(APPEND _ATG_TARGET_OPTIONS ${_option})
        endif()
    endforeach()
    
    foreach(_option ${lib_options})
        if(_ATG_${_option})
            if(NOT _ATG_TYPE STREQUAL "LIBRARY")
                message(FATAL_ERROR "Invalid option '${_option}' for '${_ATG_TYPE}' target '${_target_name}'")
            endif()
            list(APPEND _ATG_TARGET_OPTIONS ${_option})
        endif()
    endforeach()

    if(_ATG_EXCLUDE_FROM_ALL)
        list(APPEND _ATG_TARGET_OPTIONS EXCLUDE_FROM_ALL)
    endif()

    # --- Link each files to a filter/group
    foreach(_file_path ${_ATG_FILES})
        get_filename_component(_sub_group "${_file_path}" PATH)
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
        if(NOT _ATG_GROUP)
            set(_ATG_GROUP "Executables")
        endif()
    else()
        add_library(${_target_name} ${_ATG_TARGET_OPTIONS}
            ${_ATG_FILES}
        )
        if(NOT _ATG_GROUP)
            set(_ATG_GROUP "Libraries")
        endif()
    endif()

    # --- Declare target's dependencies
    if(_ATG_DEPENDS)
        add_dependencies(${_target_name} ${_ATG_DEPENDS})
    endif()

    # --- Declare target's links
    if(_ATG_LINKS)
        target_link_libraries(${_target_name} ${_ATG_LINKS})
    endif()
    
    # -- Setup some target's properties
    set_property(TARGET "${_target_name}" PROPERTY FOLDER "${_ATG_GROUP}")
    
    if(_ATG_LABEL)
        set_property(TARGET "${_target_name}" PROPERTY PROJECT_LABEL "${_ATG_LABEL}")
    endif()

    if(_ATG_DEFINITIONS)
        get_property(_target_definitions TARGET "${_target_name}" PROPERTY COMPILE_DEFINITIONS)
        set_property(TARGET "${_target_name}" PROPERTY COMPILE_DEFINITIONS  ${_target_definitions} ${_ATG_DEFINITIONS})
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
        set(_FG_PREFIX_SKIP "(\\./)?src")
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