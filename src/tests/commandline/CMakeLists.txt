set(commandline_tests_group_name "Unit Tests/CommandLine")

# --- Fiber asio tests
set(commandline_tests_source_files
  "commandline_tests.cpp"
  ${SSF_SOURCES}
  ${CORE_COMMAND_LINE_STANDARD_FILES}
  ${CORE_COMMAND_LINE_COPY_FILES}
)

add_target("commandline_tests"
  TYPE
    executable ${EXEC_FLAG} TEST
  LINKS
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    lib_ssf_network
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
  FILES
    ${commandline_tests_source_files}
)
project_group(${commandline_tests_group_name} commandline_tests)
