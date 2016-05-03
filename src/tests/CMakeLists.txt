cmake_minimum_required(VERSION 2.8)

enable_testing()

# --- Copy test certs for tests
set(project_BINARY_TESTS_DIR "${project_BINARY_DIR}/src/tests")
set(project_BINARY_TESTS_CERT_DIR ${project_BINARY_TESTS_DIR}/certs)

file(MAKE_DIRECTORY ${project_BINARY_TESTS_CERT_DIR})
file(MAKE_DIRECTORY ${project_BINARY_TESTS_CERT_DIR}/trusted)

file(COPY ${SSF_CERT_TEST_ROOT_FILES}
     DESTINATION ${project_BINARY_TESTS_CERT_DIR})
file(COPY ${SSF_CERT_TEST_TRUSTED_FILES}
     DESTINATION ${project_BINARY_TESTS_CERT_DIR}/trusted)

include_directories(
  ${OpenSSL_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS})

include_directories(
  ${GTEST_ROOT_DIR}/include
)

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

# --- Fiber asio tests
set(fiber_asio_test_source_files
  "fiber_asio_tests.cpp"
  ${SSF_SOURCES}
)

add_target("fiber_asio_tests"
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
    ${fiber_asio_test_source_files}
)
project_group("Unit Tests/Network" fiber_asio_tests)

# --- SSF Client Server tests
set(ssf_client_server_source_files
  "ssf_client_server_tests.cpp"
  ${SSF_SOURCES}
)

add_target("ssf_client_server_tests"
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
    ${ssf_client_server_source_files}
)
project_group("Unit Tests/Network" ssf_client_server_tests)

# --- SSF Client Server cipher suites tests
set(ssf_client_server_cipher_suites_source_files
  "ssf_client_server_cipher_suites_tests.cpp"
  ${SSF_SOURCES}
)

add_target("ssf_client_server_cipher_suites_tests"
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
    ${ssf_client_server_cipher_suites_source_files}
)
project_group("Unit Tests/Network" ssf_client_server_cipher_suites_tests)

# --- SSF Server tests
set(ssf_server_source_files
  "ssf_server_tests.cpp"
  ${SSF_SOURCES}
)

add_target("ssf_server_tests"
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
    ${ssf_server_source_files}
)
project_group("Unit Tests/Network" ssf_server_tests)

# --- Socks test
set(socks_test_source_files
  "socks_tests.cpp"
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
project_group("Unit Tests/Services" socks_tests)

# --- Remote socks test
set(remote_socks_test_source_files
  "remote_socks_tests.cpp"
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
project_group("Unit Tests/Services" remote_socks_tests)

# --- Stream forward test
set(stream_forwarding_test_source_files
  "stream_forwarding_tests.cpp"
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
project_group("Unit Tests/Services" stream_forward_tests)

# --- Remote stream forward test
set(remote_stream_forwarding_test_source_files
  "remote_stream_forwarding_tests.cpp"
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
project_group("Unit Tests/Services" remote_stream_forward_tests)

# --- UDP forward test
set(udp_test_source_files
  "udp_forwarding_tests.cpp"
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
project_group("Unit Tests/Services" udp_forward_tests)

# --- Remote UDP forward test
set(remote_udp_test_source_files
  "remote_udp_forwarding_tests.cpp"
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
project_group("Unit Tests/Services" remote_udp_forward_tests)

# --- File copy from client test
file(MAKE_DIRECTORY ${project_BINARY_DIR}/src/tests/files_to_copy)
file(MAKE_DIRECTORY ${project_BINARY_DIR}/src/tests/files_copied)

FILE(GLOB_RECURSE TEST_FILES_TO_COPY
  "${project_SRC_DIR}/tests/files_to_copy/*"
)

file(COPY ${TEST_FILES_TO_COPY} DESTINATION ${project_BINARY_DIR}/src/tests/files_to_copy)

set(file_copy_from_client_files
  "file_copy_from_client_tests.cpp"
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
project_group("Unit Tests/Services" file_copy_from_client_tests)

# --- Bouncing chain test
set(bouncing_test_source_files
  "bouncing_tests.cpp"
  ${SSF_SOURCES}
)

add_target("bouncing_tests"
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
    ${bouncing_test_source_files}
)
project_group("Unit Tests/Network" bouncing_tests)
