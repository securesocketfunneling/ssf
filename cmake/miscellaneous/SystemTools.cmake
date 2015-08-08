# Implementation of tools to perform some system actions
#
if(__H_SYSTEM_TOOLS_INCLUDED)
  return()
endif()
set(__H_SYSTEM_TOOLS_INCLUDED TRUE)

include(HelpersArguments)
include(FileEdit)

# ==============================================================================
# ======================= D e f i n e   f u n c t i o n s ======================
# ==============================================================================

# ------------------------------------------------------------------------------
# system_run(
#   [EXEC <command> [args]]          # Command line to execute
#   [EVAL <command> [<command>...]]  # List of instructions to evaluate
# )
#
# This function runs (i.e. executes or evaluates) the given commands
function(system_run)
  parse_arguments("RUN"
    "DEBUG;NO_LOG"
    "WORKING_DIR"
    "EXEC;EVAL"
    ""
    ${ARGN}
  )

  # Ensure that NO unparsed arguments has been found
  if(RUN_ARGN)
    message(FATAL_ERROR "Unparsed arguments found:\n${RUN_ARGN}")
  endif()

  # Check a EXEC or an EVAL has been provided
  if(NOT RUN_EXEC AND NOT RUN_EVAL)
    message(FATAL_ERROR "Neither explicit EXEC nor EVAL specified:\t${ARGN}"
    )
  elseif(RUN_EXEC AND RUN_EVAL)
    message(FATAL_ERROR "EXEC and EVAL cannot be both specified")
  endif()

  if(RUN_EVAL)
    # Proceed the EVAL (if any)
    system_eval(${RUN_EVAL})
  else()
    # Proceed the EXEC
    unset(_flags)
    foreach(_flag DEBUG NO_LOG)
      if(RUN_${_flag})
        list(APPEND _flags ${_flag})
      endif()
    endforeach()

    system_execute(${_flags}
      WORKING_DIR "${RUN_WORKING_DIR}"
      ${RUN_EXEC}
    )
  endif()
endfunction()

# ------------------------------------------------------------------------------
# system_eval(
#   <command> [<command>...]  # List of instructions to evaluate
# )
#
# This function evaluates the given list of cmake's script lines.
function(system_eval)
  # Create file unique name
  file_create_unique(_tmp_file SUFFIX ".cmake.tmp")

  # Copy given input in the file
  file(WRITE "${_tmp_file}" "# Auto generated file for 'eval' emulation\n")
  foreach(_line ${ARGN})
    file(APPEND "${_tmp_file}" "${_line}\n")
  endforeach()

  # Execute the file and release it
  include("${_tmp_file}")
  file(REMOVE "${_tmp_file}")
endfunction()

# ------------------------------------------------------------------------------
# system_execute(
#   NO_LOG                   # Disable LOG feature. Outputs are not redirected.
#   NO_LOG_DUMP              # Disable automatic dump of log in case of error
#   LOG <file_path>          # Path to the file that will host the execution log
#   LOG_VAR <name>           # Name of variable that will receive execution log
#   DEBUG                    # Enable debug mode
#   WORKING_DIR <dir_path>   # Working directory execution should be performed
#   COMMAND <command_path>   # Path to the command to execute
#   ARGS <arg1> [<arg2 ...]  # List of arguments to associate to command
# )
#
# This function executes the given command with the specified arguments.
# By default, the execution is wrap in a platform's special environment where
# compile tools are available.
#
# N.B.: If the executed command ends with a result that differs from 0, then
#       this command does not return but prints out an error message.
function(system_execute)
  # Parse arguments
  parse_arguments("EXEC"
    "NO_LOG;NO_LOG_DUMP;DEBUG"
    "WORKING_DIR;LOG;LOG_VAR;COMMAND"
    "ARGS;ENV"
    ""
    ${ARGN}
  )

  if(NOT EXEC_COMMAND)
    list(GET EXEC_ARGN 0 EXEC_COMMAND)
    list(REMOVE_AT EXEC_ARGN 0)
  endif()

  if(EXEC_ARGN)
    list(APPEND EXEC_ARGS ${EXEC_ARGN})
    unset(EXEC_ARGN)
  endif()

  if(EXEC_LOG_VAR AND (EXEC_LOG OR EXEC_NO_LOG OR EXEC_NO_LOG_DUMP))
    message(FATAL_ERROR "Invalid use of LOG directives")
  endif()

  unset(EXEC_LOG_AUTO)
  if(EXEC_LOG_VAR OR EXEC_NO_LOG)
    unset(EXEC_LOG)
  elseif(NOT EXEC_LOG)
    file_create_unique(EXEC_LOG_AUTO
      PREFIX "exec_"
      SUFFIX ".log"
    )
    set(EXEC_LOG "${EXEC_LOG_AUTO}")
  endif()

  if(EXEC_DEBUG)
    message(STATUS "*RUNNING COMMAND   : '${EXEC_COMMAND}'")
    foreach(_var NO_LOG LOG WORKING_DIR ARGS ENV)
      message(STATUS "*  ${_var} : ${EXEC_${_var}}")
    endforeach()
  endif()

  set(_command_exec ${EXEC_COMMAND} ${EXEC_ARGS})

  # Execute command
  unset(_log_fields)
  unset(_errors)
  unset(_outputs)
  if(EXEC_LOG)
    list(APPEND _log_fields
      OUTPUT_FILE "${EXEC_LOG}"
      ERROR_VARIABLE _errors
    )
  elseif(EXEC_LOG_VAR)
    list(APPEND _log_fields OUTPUT_VARIABLE _outputs)
  endif()

  unset(_env_vars)
  foreach(_env IN LISTS EXEC_ENV)
    if("${_env}" MATCHES "^([^=]+)=(.*)$")
      set(_var_name "${CMAKE_MATCH_1}")
      set(_var_value "${CMAKE_MATCH_2}")
      unset(_OLD_${_var_name})
      if(NOT "$ENV{${_var_name}}" STREQUAL "")
        set(_OLD_${_var_name} "$ENV{${_var_name}}")
      endif()
      set(ENV{${_var_name}} "${_var_value}")
      list(APPEND _env_vars "${_var_name}")
    endif()
  endforeach()

  execute_process(
    WORKING_DIRECTORY "${EXEC_WORKING_DIR}"
    RESULT_VARIABLE _ret_result
    ${_log_fields}
    COMMAND ${_command_exec}
  )

  foreach(_var_name IN LISTS _env_vars)
    if(DEFINED _OLD_${_var_name})
      set(ENV{${_var_name}} "${_OLD_${_var_name}}")
      unset(_OLD_${_var_name})
    else()
      unset(ENV{${_var_name}})
    endif()
  endforeach()

  if(_errors AND EXEC_LOG)
    file(APPEND "${EXEC_LOG}" "\n---\n--- Error messages\n---\n${_errors}")
  endif()

  # Fail current script in case of execution failure
  if(NOT _ret_result EQUAL 0)
    message("### Error [${_ret_result}] while executing:")
    unset(_error_message)
    foreach(arg ${EXEC_COMMAND} ${EXEC_ARGS})
      set(_error_message "${_error_message}'${arg}' ")
    endforeach()
    message("### ${_error_message}")

    if(EXEC_LOG AND NOT EXEC_NO_LOG_DUMP)
      message("### ===[ ${EXEC_LOG} ]===")
      file(STRINGS "${EXEC_LOG}" _log_lines LENGTH_MAXIMUM  1024)
      foreach(_log_line IN LISTS _log_lines)
        message("### ${_log_line}")
      endforeach()
      unset(_log_lines)
      message("### ===[ ${EXEC_LOG} ]===")
    elseif(EXEC_LOG)
      message("### Please, have a look to log file '${EXEC_LOG}'")
    endif()
    message(FATAL_ERROR "SCRIPT ABORTED")
  endif()

  if(EXEC_LOG_VAR)
    set(${EXEC_LOG_VAR} "${_outputs}")
  endif()

  # Proceed to some clean up
  if(NOT EXEC_DEBUG)
    if(EXEC_LOG_AUTO AND EXISTS "${EXEC_LOG_AUTO}")
      file(REMOVE "${EXEC_LOG_AUTO}")
    endif()
  endif()
endfunction()
