# --- Command line tests

add_target(commandline_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "commandline_tests.cpp"
    ${CORE_COMMAND_LINE_STANDARD_FILES}
    ${CORE_COMMAND_LINE_COPY_FILES}
  LINKS
    ssf_framework
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET commandline_tests PROPERTY FOLDER "Unit Tests/CommandLine")
