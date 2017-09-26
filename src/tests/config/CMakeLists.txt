cmake_minimum_required(VERSION 2.8)

set(config_tests_group_name "Unit Tests/Config")

file(GLOB_RECURSE CONFIG_TEST_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/config_files/*.json")

file(MAKE_DIRECTORY config_files)
file(COPY ${CONFIG_TEST_FILES} DESTINATION config_files)

# --- Config tests

# --- Load config test
set(load_config_test_source_files
  "load_config_tests.cpp"
  ${COMMON_CONFIG_FILES}
  ${COMMON_ERROR_FILES}
  ${SERVICES_FILES}
)

add_target("load_config_tests"
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
    ${load_config_test_source_files}
)
project_group(${config_tests_group_name} load_config_tests)
