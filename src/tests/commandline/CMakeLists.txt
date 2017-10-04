set(commandline_tests_group_name "Unit Tests/CommandLine")

# --- Command line tests

add_target(commandline_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "commandline_tests.cpp"
    ${CORE_COMMAND_LINE_STANDARD_FILES}
    ${CORE_COMMAND_LINE_COPY_FILES}
  LINKS
    lib_ssf_framework
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group(${commandline_tests_group_name} commandline_tests)
