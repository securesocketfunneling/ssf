cmake_minimum_required(VERSION 2.8)

enable_testing()

set(project_BINARY_TESTS_DIR "${project_BINARY_DIR}/src/tests")

# --- Copy test certs for tests
set(project_BINARY_TESTS_CERT_DIR ${project_BINARY_TESTS_DIR}/certs)

file(MAKE_DIRECTORY ${project_BINARY_TESTS_CERT_DIR})
file(MAKE_DIRECTORY ${project_BINARY_TESTS_CERT_DIR}/trusted)

file(COPY ${SSF_CERT_TEST_ROOT_FILES}
     DESTINATION ${project_BINARY_TESTS_CERT_DIR})
file(COPY ${SSF_CERT_TEST_TRUSTED_FILES}
     DESTINATION ${project_BINARY_TESTS_CERT_DIR}/trusted)

# --- copy test config files
set(project_BINARY_TESTS_CONF_DIR ${project_BINARY_TESTS_DIR}/config_files)

file(GLOB_RECURSE CONFIG_TEST_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/config_files/*.json")

file(MAKE_DIRECTORY ${project_BINARY_TESTS_CONF_DIR})
file(COPY ${CONFIG_TEST_FILES}
     DESTINATION ${project_BINARY_TESTS_CONF_DIR})

include_directories(
  ${OpenSSL_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS})

include_directories(
  ${GTEST_ROOT_DIR}/include
)

add_subdirectory(config)
add_subdirectory(network)
add_subdirectory(services)