set(config_tests_group_name "Unit Tests/Config")

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
    lib_ssf_framework
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group(${config_tests_group_name} load_config_tests)
