cmake_minimum_required(VERSION 2.8)

set(services_tests_group_name "Unit Tests/Services")

set(project_BINARY_SERVICES_TESTS_DIR "${project_BINARY_DIR}/src/tests/services")

# --- Copy test certs for tests
set(project_BINARY_SERVICES_TESTS_CERT_DIR ${project_BINARY_SERVICES_TESTS_DIR}/certs)

file(MAKE_DIRECTORY ${project_BINARY_SERVICES_TESTS_CERT_DIR})
file(MAKE_DIRECTORY ${project_BINARY_SERVICES_TESTS_CERT_DIR}/trusted)

file(COPY ${SSF_CERT_TEST_ROOT_FILES}
     DESTINATION ${project_BINARY_SERVICES_TESTS_CERT_DIR})
file(COPY ${SSF_CERT_TEST_TRUSTED_FILES}
     DESTINATION ${project_BINARY_SERVICES_TESTS_CERT_DIR}/trusted)

set(TCP_HELPERS_FILES
  "tcp_helpers.h"
  "tcp_helpers.cpp")

set(UDP_HELPERS_FILES
  "udp_helpers.h"
  "udp_helpers.cpp")

set(SOCKS_HELPERS_FILES
  "socks_helpers.h"
  "socks_helpers.cpp")

set(SERVICE_TESTS_FILES
  "service_fixture_test.h"
  "process_fixture_test.h"
)

# --- Service tests

# --- Socks test
set(socks_test_source_files
  "socks_tests.cpp"
  ${SERVICE_TESTS_FILES}
  ${SOCKS_HELPERS_FILES}
  ${TCP_HELPERS_FILES}
  ${SSF_SOURCES}
)

add_target("socks_tests"
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
    ${socks_test_source_files}
)
project_group(${services_tests_group_name} socks_tests)

# --- Remote socks test
set(remote_socks_test_source_files
  "remote_socks_tests.cpp"
  ${SERVICE_TESTS_FILES}
  ${SOCKS_HELPERS_FILES}
  ${TCP_HELPERS_FILES}
  ${SSF_SOURCES}
)

add_target("remote_socks_tests"
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
    ${remote_socks_test_source_files}
)
project_group(${services_tests_group_name} remote_socks_tests)

# --- Stream forward test
set(stream_forwarding_test_source_files
  "stream_forwarding_tests.cpp"
  ${SERVICE_TESTS_FILES}
  ${TCP_HELPERS_FILES}
  ${SSF_SOURCES}
)

add_target("stream_forward_tests"
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
    ${stream_forwarding_test_source_files}
)
project_group(${services_tests_group_name} stream_forward_tests)

# --- Remote stream forward test
set(remote_stream_forwarding_test_source_files
  "remote_stream_forwarding_tests.cpp"
  ${SERVICE_TESTS_FILES}
  ${TCP_HELPERS_FILES}
  ${SSF_SOURCES}
)

add_target("remote_stream_forward_tests"
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
    ${remote_stream_forwarding_test_source_files}
)
project_group(${services_tests_group_name} remote_stream_forward_tests)

# --- UDP forward test
set(udp_test_source_files
  "udp_forwarding_tests.cpp"
  ${SERVICE_TESTS_FILES}
  ${UDP_HELPERS_FILES}
  ${SSF_SOURCES}
)

add_target("udp_forward_tests"
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
    ${udp_test_source_files}
)
project_group(${services_tests_group_name} udp_forward_tests)

# --- Remote UDP forward test
set(remote_udp_test_source_files
  "remote_udp_forwarding_tests.cpp"
  ${SERVICE_TESTS_FILES}
  ${UDP_HELPERS_FILES}
  ${SSF_SOURCES}
)

add_target("remote_udp_forward_tests"
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
    ${remote_udp_test_source_files}
)
project_group(${services_tests_group_name} remote_udp_forward_tests)

# --- File copy from client test
file(MAKE_DIRECTORY ${project_BINARY_DIR}/src/tests/services/files_to_copy)
file(MAKE_DIRECTORY ${project_BINARY_DIR}/src/tests/services/files_copied)

FILE(GLOB_RECURSE TEST_FILES_TO_COPY
  "${project_SRC_DIR}/tests/services/files_to_copy/*"
)

file(COPY ${TEST_FILES_TO_COPY} DESTINATION ${project_BINARY_DIR}/src/tests/services/files_to_copy)

set(file_copy_from_client_files
  "file_copy_from_client_tests.cpp"
  "file_copy_fixture_test.h"
  "file_copy_fixture_test.cpp"
  ${SSF_SOURCES}
)

add_target("file_copy_from_client_tests"
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
    ${file_copy_from_client_files}
)
project_group(${services_tests_group_name} file_copy_from_client_tests)

# --- Process test
set(process_test_source_files
  "process_tests.cpp"
  ${SERVICE_TESTS_FILES}
  ${SSF_SOURCES}
)

add_target("process_tests"
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
    ${process_test_source_files}
)
project_group(${services_tests_group_name} process_tests)

# --- Remote process test
set(remote_process_test_source_files
  "remote_process_tests.cpp"
  ${SERVICE_TESTS_FILES}
  ${SSF_SOURCES}
)

add_target("remote_process_tests"
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
    ${remote_process_test_source_files}
)
project_group(${services_tests_group_name} remote_process_tests)
