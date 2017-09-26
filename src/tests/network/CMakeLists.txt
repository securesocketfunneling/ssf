set(network_tests_group_name "Unit Tests/Network")


# --- Network tests

set(ssf_fixture_test_file
  "ssf_fixture_test.h"
  "ssf_fixture_test.cpp"
)

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

# --- SSF Client tests
set(ssf_client_source_files
  "ssf_client_tests.cpp"
  ${SSF_SOURCES}
  ${ssf_fixture_test_file}
)

add_target("ssf_client_tests"
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
    ${ssf_client_source_files}
)
project_group(${network_tests_group_name} ssf_client_tests)

# --- SSF Server tests
set(ssf_server_source_files
  "ssf_server_tests.cpp"
  ${SSF_SOURCES}
  ${ssf_fixture_test_file}
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
