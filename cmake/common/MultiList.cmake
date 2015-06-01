# Implementation of a multi list behavior
#
# The main difference versus a legacy list is that all provided elements will be
# aggregated and then insert as a single element which is, at the end, a list
#
if(__H_MULTI_LIST_INCLUDED)
  return()
endif()
set(__H_MULTI_LIST_INCLUDED TRUE)

# ------------------------------------------------------------------------------
# multi_list(
#   <command>      # All the legacy list's command are accepted in addition to
#                  # APPEND_AT which is used to add new items to an item list
#                  # present in the multi-list
#                  # The syntax of the APPEND_AT is:
#                  #     APPEND_AT <list_name> <item_index> <new_elements> ...
#
#   <list_name>    # The name of the multi-list the command has to be applied to
#
#   <command_args> # The arguments associated to the specified command
# )
# ---
function(multi_list command name)
  string(TOUPPER "${command}" command)
  if(command MATCHES "^(INSERT|APPEND_AT)$")
    # Extract the destination index from arguments
    list(GET ARGN 0 _index)
    list(REMOVE_AT ARGN 0)
    string(REPLACE ";" "\\;" ARGN "${ARGN}")

    if(${name})
      unset(_tmp_list)
      foreach(_item ${${name}})
        string(REPLACE ";" "\\;" _item "${_item}")

        # Check if index reached 0 to add the value
        if(_index GREATER -1)
          if(_index EQUAL 0)
            if(command STREQUAL "INSERT")
              list(APPEND _tmp_list "${ARGN}")
            else()
              set(_item "${_item}\\;${ARGN}")
            endif()
          endif()
          math(EXPR _index "${_index} - 1")
        endif()

        # Update destination list with the aggregation of current item's elements
        list(APPEND _tmp_list "${_item}")
      endforeach()
      set(${name} "${_tmp_list}" PARENT_SCOPE)
      return()
    else()
      # Add to underneath list the aggregation of all input elements
      list(APPEND ${name} "${ARGN}")
    endif()
  elseif(command STREQUAL "APPEND")
    # Add to underneath list the aggregation of all input elements
    string(REPLACE ";" "\\;" ARGN "${ARGN}")
    list(${command} ${name} "${ARGN}")
  else()
    # Forward command to system's list
    list(${command} ${name} ${ARGN})
  endif()
  set(${name} "${${name}}" PARENT_SCOPE)
endfunction()
