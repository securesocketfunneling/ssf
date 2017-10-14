add_library(fixture_test STATIC EXCLUDE_FROM_ALL ssf_fixture_test.h ssf_fixture_test.cpp)
target_link_libraries(fixture_test gtest ssf_framework)

# --- Fiber asio tests
add_executable(fiber_asio_tests EXCLUDE_FROM_ALL fiber_asio_tests.cpp)
target_link_libraries(fiber_asio_tests ssf_framework tls_config_helper)
add_unit_test(fiber_asio_tests)
set_property(TARGET fiber_asio_tests PROPERTY FOLDER "Unit Tests/Network")

# --- SSF Client Server tests
add_executable(ssf_client_server_tests EXCLUDE_FROM_ALL ssf_client_server_tests.cpp)
target_link_libraries(ssf_client_server_tests ssf_framework tls_config_helper)
add_unit_test(ssf_client_server_tests)
set_property(TARGET ssf_client_server_tests PROPERTY FOLDER "Unit Tests/Network")

# --- SSF Client Server cipher suites tests
add_executable(ssf_client_server_cipher_suites_tests EXCLUDE_FROM_ALL ssf_client_server_cipher_suites_tests.cpp)
target_link_libraries(ssf_client_server_cipher_suites_tests ssf_framework tls_config_helper)
add_unit_test(ssf_client_server_cipher_suites_tests)
set_property(TARGET ssf_client_server_cipher_suites_tests PROPERTY FOLDER "Unit Tests/Network")

# --- SSF Client tests
add_executable(ssf_client_tests EXCLUDE_FROM_ALL ssf_client_tests.cpp)
target_link_libraries(ssf_client_tests ssf_framework fixture_test tls_config_helper)
add_unit_test(ssf_client_tests)
set_property(TARGET ssf_client_tests PROPERTY FOLDER "Unit Tests/Network")

# --- SSF Server tests
add_executable(ssf_server_tests EXCLUDE_FROM_ALL ssf_server_tests.cpp)
target_link_libraries(ssf_server_tests ssf_framework fixture_test tls_config_helper)
add_unit_test(ssf_server_tests)
set_property(TARGET ssf_server_tests PROPERTY FOLDER "Unit Tests/Network")

# --- Circuit tests
add_executable(circuit_tests EXCLUDE_FROM_ALL circuit_tests.cpp)
target_link_libraries(circuit_tests ssf_framework tls_config_helper)
add_unit_test(circuit_tests)
set_property(TARGET circuit_tests PROPERTY FOLDER "Unit Tests/Network")
