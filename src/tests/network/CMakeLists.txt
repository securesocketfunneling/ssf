set(ssf_fixture_test_file
  "ssf_fixture_test.h"
  "ssf_fixture_test.cpp"
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# --- Fiber asio tests
add_target(fiber_asio_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    fiber_asio_tests.cpp
  LINKS
    lib_ssf_framework
    tls_config_helper
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group("Unit Tests/Network" fiber_asio_tests)

# --- SSF Client Server tests
add_target(ssf_client_server_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    ssf_client_server_tests.cpp
  LINKS
    lib_ssf_framework
    tls_config_helper
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group("Unit Tests/Network" ssf_client_server_tests)

# --- SSF Client Server cipher suites tests
add_target(ssf_client_server_cipher_suites_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    ssf_client_server_cipher_suites_tests.cpp
  LINKS
    lib_ssf_framework
    tls_config_helper
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group("Unit Tests/Network" ssf_client_server_cipher_suites_tests)

# --- SSF Client tests
add_target(ssf_client_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    ssf_client_tests.cpp
    ${ssf_fixture_test_file}
  LINKS
    lib_ssf_framework
    tls_config_helper
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group("Unit Tests/Network" ssf_client_tests)

# --- SSF Server tests
add_target(ssf_server_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    ssf_server_tests.cpp
    ${ssf_fixture_test_file}
  LINKS
    lib_ssf_framework
    tls_config_helper
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group("Unit Tests/Network" ssf_server_tests)

# --- Circuit tests
add_target("circuit_tests"
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "circuit_tests.cpp"
  LINKS
    lib_ssf_framework
    tls_config_helper
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group("Unit Tests/Network" circuit_tests)
