cmake_minimum_required(VERSION 2.8)

enable_testing()

set(project_BINARY_CONFIG_TESTS_DIR "${project_BINARY_DIR}/src/tests")
     
# --- copy test config files
set(project_BINARY_CONFIG_TESTS_CONF_DIR ${project_BINARY_CONFIG_TESTS_DIR}/config_files)

file(GLOB_RECURSE CONFIG_TEST_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/config_files/*.json")
  
file(MAKE_DIRECTORY ${project_BINARY_CONFIG_TESTS_CONF_DIR})
file(COPY ${CONFIG_TEST_FILES}
     DESTINATION ${project_BINARY_CONFIG_TESTS_CONF_DIR})

# --- Config tests

# --- Load config test
set(load_config_test_source_files
  "load_config_tests.cpp"
  ${COMMON_CONFIG_FILES}
  ${COMMON_ERROR_FILES}
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
project_group("Unit Tests/Config" load_config_tests)