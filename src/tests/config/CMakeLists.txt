file(GLOB_RECURSE CONFIG_TEST_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/config_files/*.json")

file(MAKE_DIRECTORY config_files)
file(COPY ${CONFIG_TEST_FILES} DESTINATION config_files)

# --- Config tests

# --- Load config test
add_target(load_config_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "load_config_tests.cpp"
  LINKS
    ssf_framework
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group("Unit Tests/Config" load_config_tests)
