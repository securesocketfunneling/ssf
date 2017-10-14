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

add_target(option_parser_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "option_parser_tests.cpp"
  LINKS
    ssf_framework
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET option_parser_tests PROPERTY FOLDER ${services_tests_group_name})

# --- Socks test
add_target(socks_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "socks_tests.cpp"
    ${SERVICE_TESTS_FILES}
    ${SOCKS_HELPERS_FILES}
    ${TCP_HELPERS_FILES}
  LINKS
    ssf_framework
    tls_config_helper
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET socks_tests PROPERTY FOLDER ${services_tests_group_name})

# --- Remote socks test
add_target(remote_socks_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "remote_socks_tests.cpp"
    ${SERVICE_TESTS_FILES}
    ${SOCKS_HELPERS_FILES}
    ${TCP_HELPERS_FILES}
  LINKS
    ssf_framework
    tls_config_helper
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET remote_socks_tests PROPERTY FOLDER ${services_tests_group_name})

# --- Stream forward test
add_target(stream_forward_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "stream_forwarding_tests.cpp"
    ${SERVICE_TESTS_FILES}
    ${TCP_HELPERS_FILES}
  LINKS
    ssf_framework
    tls_config_helper
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET stream_forward_tests PROPERTY FOLDER ${services_tests_group_name})

# --- Remote stream forward test
add_target(remote_stream_forward_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "remote_stream_forwarding_tests.cpp"
    ${SERVICE_TESTS_FILES}
    ${TCP_HELPERS_FILES}
  LINKS
    ssf_framework
    tls_config_helper
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET remote_stream_forward_tests PROPERTY FOLDER ${services_tests_group_name})

# --- UDP forward test
add_target(datagram_forward_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "datagram_forward_tests.cpp"
    ${SERVICE_TESTS_FILES}
    ${UDP_HELPERS_FILES}
  LINKS
    ssf_framework
    tls_config_helper
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET datagram_forward_tests PROPERTY FOLDER ${services_tests_group_name})

# --- Remote UDP forward test
add_target(remote_datagram_forward_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "remote_datagram_forward_tests.cpp"
    ${SERVICE_TESTS_FILES}
    ${UDP_HELPERS_FILES}
  LINKS
    ssf_framework
    tls_config_helper
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET remote_datagram_forward_tests PROPERTY FOLDER ${services_tests_group_name})

# --- File copy from client test
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/files_to_copy)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/files_copied)

FILE(GLOB_RECURSE TEST_FILES_TO_COPY ${CMAKE_CURRENT_SOURCE_DIR}/files_to_copy/*)

file(COPY ${TEST_FILES_TO_COPY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/files_to_copy)

add_target(file_copy_from_client_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "file_copy_from_client_tests.cpp"
    "file_copy_fixture_test.h"
    "file_copy_fixture_test.cpp"
  LINKS
    ssf_framework
    tls_config_helper
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET file_copy_from_client_tests PROPERTY FOLDER ${services_tests_group_name})

# --- Process test
add_target(process_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "process_tests.cpp"
    ${SERVICE_TESTS_FILES}
  LINKS
    ssf_framework
    tls_config_helper
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET process_tests PROPERTY FOLDER ${services_tests_group_name})

# --- Remote process test
add_target(remote_process_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "remote_process_tests.cpp"
    ${SERVICE_TESTS_FILES}
  LINKS
    ssf_framework
    tls_config_helper
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET remote_process_tests PROPERTY FOLDER ${services_tests_group_name})
