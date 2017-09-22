# Implementation of tools to edit text files
#
if(__H_FILE_EDIT_INCLUDED)
  return()
endif()
set(__H_FILE_EDIT_INCLUDED TRUE)

include(HelpersArguments)

# ==============================================================================
# ======================= D e f i n e   f u n c t i o n s ======================
# ==============================================================================

# ------------------------------------------------------------------------------
#   file_create_unique(
#     <var_name>              # Name of variable where to store file's name
#     [PREFIX <file_prefix>]  # Prefix to use for file's name
#     [SUFFIX <file_suffix>]  # Suffix to add to file's name
#     [CONTENT <Content>]     # Optional content to write in created file
#     [PATH <path>]           # Path to where the file is to be created
#   )
#
# This function creates a file using a unique name. If the content is provided,
# it is used to fill the file.
#
function(file_create_unique var_name)
  # Parse arguments
  parse_arguments("FILE"
    ""
    "PREFIX;SUFFIX;CONTENT;PATH"
    ""
    ""
    ${ARGN}
  )

  if(NOT FILE_PATH)
    set(FILE_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  endif()

  # Prepare meta information to create unique file name
  string(TIMESTAMP _tmp_file "%I%M%S" )
  string(RANDOM
    LENGTH 16
    ALPHABET "0123456789ABCDEF"
    RANDOM_SEED "${_tmp_file}"
    _alea
  )

  # Create file name
  set(_tmp_file "${_tmp_file}-${_alea}${FILE_SUFFIX}")
  set(_tmp_file "${FILE_PATH}/${FILE_PREFIX}${_tmp_file}")

  # Copy given input in the file
  if(FILE_CONTENT)
    file(WRITE "${_tmp_file}" "${FILE_CONTENT}")
  else()
    file(WRITE "${_tmp_file}" "")
  endif()

  # Return created file's name to parent
  set(${var_name} "${_tmp_file}" PARENT_SCOPE)
endfunction()


# ------------------------------------------------------------------------------
# file_load(
#   FILE       <file_path>  # Path to the file to load
#   OUTPUT     <var_name>   # Name of the parent's variable where to store
#                           # file's content
#   MERGE_SPLIT             # Ask to reassemble split lines
# )
#
# This function loads a file's content and store it in a destination list
# variable owned by the caller (i.e. parent)
#
# N.B.: The MERGE_SPLIT option is to force file's split lines to be
#       reassembled into a single line.
#       A split line is a line ending with the character '\' followed with
#       a EOL character (i.e. '\n'  special character)
#
# example:
#   file_load(
#     FILE        my_file
#     OUTPUT      lines
#     MERGE_SPLIT
#   )
#   message("'${lines}'")
#
# With the following content for 'my_file':
#
#   Hello World
#   I \
#   love \
#   football
#
# The displayed message will be a list with 2 elements:
#   'Hello World;I love football'
# --
function(file_load)
  parse_arguments("FILE_LOAD"
    "MERGE_SPLIT"
    "FILE;OUTPUT"
    ""
    ""
    ${ARGN}
  )

  if(NOT FILE_LOAD_FILE OR NOT FILE_LOAD_OUTPUT)
    return()
  endif()

  # Read file's lines but ensure its ';' characters will not conflict
  file(STRINGS "${FILE_LOAD_FILE}" _lines_list NEWLINE_CONSUME)
  if(FILE_LOAD_MERGE_SPLIT)
    string(REGEX REPLACE "\\\\\n" "" _lines_list "${_lines_list}")
  else()
    string(REGEX REPLACE "(\\\\\n)" "\\\\\\1" _lines_list "${_lines_list}")
  endif()
  string(REGEX REPLACE "\n$" "" _lines_list "${_lines_list}")
  string(REGEX REPLACE "\n" ";" _lines_list "${_lines_list}")

  set(${FILE_LOAD_OUTPUT} "${_lines_list}" PARENT_SCOPE)
endfunction()

# ------------------------------------------------------------------------------
# file_edit(
#   FILE <list_of_files>  # of files to edit
#   RULE                  # Edit rule to apply to given files
#     <line_filter>       #   Filter for lines the rule has to be applied to
#     <regex_find>        #   Regular expression to line's part to be edited
#     <regex_replace>     #   Regular expression for modification to apply
#   MERGE_SPLIT           # Ask to reassemble split lines
# )
#
# This function is used to edit the input files using the given rules
# A rule use the following syntax
#   <line_filter> <regex_to_replace_from> <regex_to_apply_to>
#
# N.B.: To provide more than one RULE, just add a new RULE line in your call
#
# example:
#   file_edit(
#     FILE my_file
#     RULE ""         "o"  "O"
#     RULE "fOOtball" "ll" "L2"
#   )
#
# With the following content for 'my_file':
#
#   Hello World
#   I love football
#
# The resulting file's content will be:
#
#   HellO WOrld
#   I lOve fOOtbaL2
# ---
function(file_edit)
  parse_arguments("FILE_EDIT"
    "MERGE_SPLIT"
    ""
    "FILE"
    "RULE"
    ${ARGN}
  )

  if(NOT FILE_EDIT_RULE OR NOT FILE_EDIT_FILE)
    return()
  endif()

  if(FILE_EDIT_MERGE_SPLIT)
    set(FILE_EDIT_MERGE_SPLIT "MERGE_SPLIT")
  endif()

  # Aggregate all rules filter to speed up line validation
  unset(_line_filter)
  foreach(_rule IN LISTS FILE_EDIT_RULE)
    list(GET _rule 0 _rule)
    if(_rule)
      list(APPEND _line_filter "${_rule}")
    endif()
  endforeach()
  string(REPLACE ";" "|" _line_filter "(${_line_filter})")

  foreach(_file IN LISTS FILE_EDIT_FILE)
    # Check file exists
    if(NOT EXISTS "${_file}")
      message(FATAL_ERROR "Cannot access to file '${_file}'")
    endif()

    # Load the current file's content
    file_load(FILE "${_file}" OUTPUT _lines_list ${FILE_EDIT_MERGE_SPLIT})

    # Patch and save file's line
    set(_tmp_file "${_file}.new")
    file(WRITE "${_tmp_file}")
    foreach(_line IN LISTS _lines_list)
      if(_line AND _line MATCHES "${_line_filter}")
        foreach(_rule IN LISTS FILE_EDIT_RULE)
          list(GET _rule 0 _filter)
          if(NOT _filter OR _line MATCHES "${_filter}")
            list(GET _rule 1 _find)
            list(LENGTH _rule _replace)
            if(_replace GREATER 2)
              list(GET _rule 2 _replace)
            else()
              unset(_replace)
            endif()
            string(REGEX REPLACE "${_find}" "${_replace}" _line "${_line}")
          endif()
        endforeach()
      endif()
      if(NOT FILE_EDIT_MERGE_SPLIT)
        string(REGEX REPLACE "\\\\\\\\$" "\\\\" _line "${_line}")
      endif()
      file(APPEND "${_tmp_file}" "${_line}\n")
    endforeach()

    # Replace file with its patched version
    file(RENAME "${_file}" "${_file}.sav")
    file(RENAME "${_tmp_file}" "${_file}")
  endforeach()
endfunction()
