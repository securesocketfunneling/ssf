cmake_minimum_required(VERSION 2.8)

set(network_tests_group_name "Unit Tests/Network")

set(project_BINARY_NETWORK_TESTS_DIR "${project_BINARY_DIR}/src/tests/network")

# --- Copy test certs for tests
set(project_BINARY_NETWORK_TESTS_CERT_DIR ${project_BINARY_NETWORK_TESTS_DIR}/certs)

file(MAKE_DIRECTORY ${project_BINARY_NETWORK_TESTS_CERT_DIR})
file(MAKE_DIRECTORY ${project_BINARY_NETWORK_TESTS_CERT_DIR}/trusted)

file(COPY ${SSF_CERT_TEST_ROOT_FILES}
     DESTINATION ${project_BINARY_NETWORK_TESTS_CERT_DIR})
file(COPY ${SSF_CERT_TEST_TRUSTED_FILES}
     DESTINATION ${project_BINARY_NETWORK_TESTS_CERT_DIR}/trusted)

# --- Network tests

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
project_group(${network_tests_group_name} fiber_asio_tests)

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
project_group(${network_tests_group_name} ssf_client_server_tests)

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
project_group(${network_tests_group_name} ssf_client_server_cipher_suites_tests)

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
project_group(${network_tests_group_name} ssf_server_tests)

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
project_group(${network_tests_group_name} bouncing_tests)
