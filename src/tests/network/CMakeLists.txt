set(network_tests_group_name "Unit Tests/Network")

# --- Fiber asio tests
add_target(fiber_asio_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "fiber_asio_tests.cpp"
  LINKS
    lib_ssf_framework
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group(${network_tests_group_name} fiber_asio_tests)

# --- SSF Client Server tests
add_target(ssf_client_server_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "ssf_client_server_tests.cpp"
  LINKS
    lib_ssf_framework
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group(${network_tests_group_name} ssf_client_server_tests)

# --- SSF Client Server cipher suites tests
add_target(ssf_client_server_cipher_suites_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "ssf_client_server_cipher_suites_tests.cpp"
  LINKS
    lib_ssf_framework
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group(${network_tests_group_name} ssf_client_server_cipher_suites_tests)

# --- SSF Client tests
add_target(ssf_client_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "ssf_client_tests.cpp"
    ${ssf_fixture_test_file}
  LINKS
    lib_ssf_framework
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group(${network_tests_group_name} ssf_client_tests)

# --- SSF Server tests
add_target(ssf_server_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "ssf_server_tests.cpp"
    ${ssf_fixture_test_file}
  LINKS
    lib_ssf_framework
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"

add_target("circuit_tests"
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "circuit_tests.cpp"
  LINKS
    lib_ssf_framework
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group(${network_tests_group_name} circuit_tests)
