set(services_tests_group_name "Unit Tests/Services")

set(TCP_HELPERS_FILES
  tcp_helpers.h
  tcp_helpers.cpp)

set(UDP_HELPERS_FILES
  udp_helpers.h
  udp_helpers.cpp)

set(SOCKS_HELPERS_FILES
  socks_helpers.h
  socks_helpers.cpp)

set(SERVICE_TESTS_FILES
  datagram_fixture_test.h
  service_fixture_test.h
  process_fixture_test.h
  socks_fixture_test.h
  stream_fixture_test.h)

# --- Service tests

# --- Option parser

add_executable(option_parser_tests EXCLUDE_FROM_ALL option_parser_tests.cpp)
target_link_libraries(option_parser_tests ssf_framework gtest)
add_unit_test(option_parser_tests)
set_property(TARGET option_parser_tests PROPERTY FOLDER ${services_tests_group_name})

# --- Socks test
add_executable(socks_tests EXCLUDE_FROM_ALL socks_tests.cpp ${SERVICE_TESTS_FILES} ${SOCKS_HELPERS_FILES} ${TCP_HELPERS_FILES})
target_link_libraries(socks_tests ssf_framework tls_config_helper gtest)
add_unit_test(socks_tests)
set_property(TARGET socks_tests PROPERTY FOLDER ${services_tests_group_name})

# --- Remote socks test
add_executable(remote_socks_tests EXCLUDE_FROM_ALL remote_socks_tests.cpp ${SERVICE_TESTS_FILES} ${SOCKS_HELPERS_FILES} ${TCP_HELPERS_FILES})
target_link_libraries(remote_socks_tests ssf_framework tls_config_helper gtest)
add_unit_test(remote_socks_tests)
set_property(TARGET remote_socks_tests PROPERTY FOLDER ${services_tests_group_name})

# --- Stream forward test
add_executable(stream_forward_tests EXCLUDE_FROM_ALL stream_forwarding_tests.cpp ${SERVICE_TESTS_FILES} ${TCP_HELPERS_FILES})
target_link_libraries(stream_forward_tests ssf_framework tls_config_helper gtest)
add_unit_test(stream_forward_tests)
set_property(TARGET stream_forward_tests PROPERTY FOLDER ${services_tests_group_name})

# --- Remote stream forward test
add_executable(remote_stream_forward_tests EXCLUDE_FROM_ALL remote_stream_forwarding_tests.cpp ${SERVICE_TESTS_FILES} ${TCP_HELPERS_FILES})
target_link_libraries(remote_stream_forward_tests ssf_framework tls_config_helper gtest)
add_unit_test(remote_stream_forward_tests)
set_property(TARGET remote_stream_forward_tests PROPERTY FOLDER ${services_tests_group_name})

# --- UDP forward test
add_executable(datagram_forward_tests EXCLUDE_FROM_ALL datagram_forward_tests.cpp ${SERVICE_TESTS_FILES} ${UDP_HELPERS_FILES})
target_link_libraries(datagram_forward_tests ssf_framework tls_config_helper gtest)
add_unit_test(datagram_forward_tests)
set_property(TARGET datagram_forward_tests PROPERTY FOLDER ${services_tests_group_name})

# --- Remote UDP forward test
add_executable(remote_datagram_forward_tests EXCLUDE_FROM_ALL remote_datagram_forward_tests.cpp ${SERVICE_TESTS_FILES} ${UDP_HELPERS_FILES})
target_link_libraries(remote_datagram_forward_tests ssf_framework tls_config_helper gtest)
add_unit_test(remote_datagram_forward_tests)
set_property(TARGET remote_datagram_forward_tests PROPERTY FOLDER ${services_tests_group_name})

# --- File copy from client test
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/files_to_copy)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/files_copied)

FILE(GLOB_RECURSE TEST_FILES_TO_COPY ${CMAKE_CURRENT_SOURCE_DIR}/files_to_copy/*)

file(COPY ${TEST_FILES_TO_COPY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/files_to_copy)

add_executable(file_copy_from_client_tests EXCLUDE_FROM_ALL file_copy_from_client_tests.cpp file_copy_fixture_test.h file_copy_fixture_test.cpp)
target_link_libraries(file_copy_from_client_tests ssf_framework tls_config_helper gtest)
add_unit_test(file_copy_from_client_tests)
set_property(TARGET file_copy_from_client_tests PROPERTY FOLDER ${services_tests_group_name})

# --- Process test
add_executable(process_tests EXCLUDE_FROM_ALL process_tests.cpp ${SERVICE_TESTS_FILES})
target_link_libraries(process_tests ssf_framework tls_config_helper gtest)
add_unit_test(process_tests)
set_property(TARGET process_tests PROPERTY FOLDER ${services_tests_group_name})

# --- Remote process test
add_executable(remote_process_tests EXCLUDE_FROM_ALL remote_process_tests.cpp ${SERVICE_TESTS_FILES})
target_link_libraries(remote_process_tests ssf_framework tls_config_helper gtest)
add_unit_test(remote_process_tests)
set_property(TARGET remote_process_tests PROPERTY FOLDER ${services_tests_group_name})
