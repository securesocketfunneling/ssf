set(service_test_group_name "Unit Tests/Services")

add_library(tcp_helpers STATIC EXCLUDE_FROM_ALL
  tcp_helpers.h tcp_helpers.cpp)
target_link_libraries(tcp_helpers ssf_framework)
set_property(TARGET tcp_helpers PROPERTY FOLDER ${service_test_group_name})

add_library(udp_helpers STATIC EXCLUDE_FROM_ALL
  udp_helpers.h udp_helpers.cpp)
target_link_libraries(udp_helpers ssf_framework)
set_property(TARGET udp_helpers PROPERTY FOLDER ${service_test_group_name})

add_library(socks_helpers STATIC EXCLUDE_FROM_ALL
  socks_helpers.h socks_helpers.cpp)
target_link_libraries(socks_helpers ssf_framework)
set_property(TARGET socks_helpers PROPERTY FOLDER ${service_test_group_name})

set(SERVICE_TEST_HEADERS
  datagram_fixture_test.h
  service_fixture_test.h
  shell_fixture_test.h
  socks_fixture_test.h
  stream_fixture_test.h)

# --- Service tests

# --- Option parser

add_executable(option_parser_tests EXCLUDE_FROM_ALL option_parser_tests.cpp)
target_link_libraries(option_parser_tests ssf_framework gtest)
add_unit_test(option_parser_tests)
set_property(TARGET option_parser_tests PROPERTY FOLDER ${service_test_group_name})

# --- Socks test
add_executable(socks_tests EXCLUDE_FROM_ALL socks_tests.cpp ${SERVICE_TEST_HEADERS})
target_link_libraries(socks_tests ssf_framework tls_config_helper socks_helpers tcp_helpers gtest)
add_unit_test(socks_tests)
set_property(TARGET socks_tests PROPERTY FOLDER ${service_test_group_name})

# --- Remote socks test
add_executable(remote_socks_tests EXCLUDE_FROM_ALL remote_socks_tests.cpp ${SERVICE_TEST_HEADERS})
target_link_libraries(remote_socks_tests ssf_framework tls_config_helper socks_helpers tcp_helpers gtest)
add_unit_test(remote_socks_tests)
set_property(TARGET remote_socks_tests PROPERTY FOLDER ${service_test_group_name})

# --- Stream forward test
add_executable(stream_forward_tests EXCLUDE_FROM_ALL stream_forwarding_tests.cpp ${SERVICE_TEST_HEADERS})
target_link_libraries(stream_forward_tests ssf_framework tls_config_helper tcp_helpers gtest)
add_unit_test(stream_forward_tests)
set_property(TARGET stream_forward_tests PROPERTY FOLDER ${service_test_group_name})

# --- Remote stream forward test
add_executable(remote_stream_forward_tests EXCLUDE_FROM_ALL remote_stream_forwarding_tests.cpp ${SERVICE_TEST_HEADERS})
target_link_libraries(remote_stream_forward_tests ssf_framework tls_config_helper tcp_helpers gtest)
add_unit_test(remote_stream_forward_tests)
set_property(TARGET remote_stream_forward_tests PROPERTY FOLDER ${service_test_group_name})

# --- UDP forward test
add_executable(datagram_forward_tests EXCLUDE_FROM_ALL datagram_forward_tests.cpp ${SERVICE_TEST_HEADERS})
target_link_libraries(datagram_forward_tests ssf_framework tls_config_helper udp_helpers gtest)
add_unit_test(datagram_forward_tests)
set_property(TARGET datagram_forward_tests PROPERTY FOLDER ${service_test_group_name})

# --- Remote UDP forward test
add_executable(remote_datagram_forward_tests EXCLUDE_FROM_ALL remote_datagram_forward_tests.cpp ${SERVICE_TEST_HEADERS})
target_link_libraries(remote_datagram_forward_tests ssf_framework tls_config_helper udp_helpers gtest)
add_unit_test(remote_datagram_forward_tests)
set_property(TARGET remote_datagram_forward_tests PROPERTY FOLDER ${service_test_group_name})

# --- File copy from client test
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/files_to_copy)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/files_copied)

add_executable(copy_tests EXCLUDE_FROM_ALL copy_tests.cpp copy_fixture_test.h copy_fixture_test.cpp)
target_link_libraries(copy_tests ssf_framework tls_config_helper gtest)
add_unit_test(copy_tests)
set_property(TARGET copy_tests PROPERTY FOLDER ${service_test_group_name})

# --- Shell test
add_executable(shell_tests EXCLUDE_FROM_ALL shell_tests.cpp ${SERVICE_TEST_HEADERS})
target_link_libraries(shell_tests ssf_framework tls_config_helper gtest)
add_unit_test(shell_tests)
set_property(TARGET shell_tests PROPERTY FOLDER ${service_test_group_name})

# --- Remote shell test
add_executable(remote_shell_tests EXCLUDE_FROM_ALL remote_shell_tests.cpp ${SERVICE_TEST_HEADERS})
target_link_libraries(remote_shell_tests ssf_framework tls_config_helper gtest)
add_unit_test(remote_shell_tests)
set_property(TARGET remote_shell_tests PROPERTY FOLDER ${service_test_group_name})
