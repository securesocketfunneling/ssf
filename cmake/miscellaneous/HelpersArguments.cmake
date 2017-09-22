#
# Define some helpers to handle function/macro arguments
#
if(__H_HELPER_ARGUMENTS_INCLUDED)
  return()
endif()
set(__H_HELPER_ARGUMENTS_INCLUDED TRUE)

# ==============================================================================
# ======================= D e f i n e   f u n c t i o n s ======================
# ==============================================================================
# ------------------------------------------------------------------------------
# parse_arguments(
#   <PREFIX>  # Prefix that will be added to created variable name
#   <OPTIONS> # List of boolean options
#   <SINGLES> # List of single value options
#   <LISTS>   # List of multiple values options
#   <MAPS>    # List of complex values (single and nested multiple) options
#   <ARGS...> # Arguments to match against provided options
# )
#
# Parse the given arguments and dispatches them among the specified fields
#
# All parsed arguments will be stored in their respective created variable
# of the form <PREFIX>_<NAME> where <NAME> will be the name of the associated
# option.
# N.B.: All the unparsed arguments will be stored in created variable named
#       <PREFIX>_ARGN.
# --
function(parse_arguments prefix options singles lists maps)
  # Prepare returned values
  set(fields_list ${options} ${singles} ${lists} ${maps})
  foreach(field ARGN ${fields_list})
    unset(${prefix}_${field})
  endforeach()

  # Parse each arguments
  unset(current_field)
  foreach(arg ${ARGN})
    # Test if current argument is a field's keyword
    list(FIND fields_list "${arg}" field_index)
    if(field_index GREATER -1)
      # Before changing field, store value of previous one
      if(current_field AND tmp_list)
        if(field_type STREQUAL MAP)
          string(REPLACE ";" "\\;" tmp_list "${tmp_list}")
        endif()
        list(APPEND ${prefix}_${current_field} "${tmp_list}")
        unset(tmp_list)
      endif()

      # Test if new field is an option
      list(FIND options "${arg}" field_index)
      if(NOT field_index EQUAL -1)
        # This field need do not depend on other args. No need for buffering
        # Just set its value to ON
        set(${prefix}_${arg} ON)
        unset(current_field)
      else()
        # Setup the name of field to populate
        set(current_field ${arg})

        # Test if new field is a single value
        list(FIND singles "${arg}" field_index)
        if(NOT field_index EQUAL -1)
          unset(field_type)
        else()
          # Test if new field is a multiple value
          list(FIND lists "${arg}" field_index)
          if(field_index GREATER -1)
            set(field_type LIST)
          else()
            # Last choice, field is a map (i.e. multiple list)
            set(field_type MAP)
          endif()
        endif()
      endif()
    else()
      # Store current argument in its destination
      if(current_field)
        if(field_type)
          list(APPEND tmp_list ${arg})
        else()
          set(${prefix}_${current_field} ${arg})
          unset(current_field)
        endif()
      else()
        # No field currently selected. Default destination is ARGN
        list(APPEND ${prefix}_ARGN ${arg})
      endif()
    endif()
  endforeach()

  # Flush parse cache ... if any
  if(current_field AND tmp_list)
    if(field_type STREQUAL MAP)
      string(REPLACE ";" "\\;" tmp_list "${tmp_list}")
    endif()
    list(APPEND ${prefix}_${current_field} "${tmp_list}")
  endif()

  # Export created variables to parent's scope
  foreach(field ARGN ${fields_list})
    set(${prefix}_${field} "${${prefix}_${field}}" PARENT_SCOPE)
  endforeach()

endfunction()
