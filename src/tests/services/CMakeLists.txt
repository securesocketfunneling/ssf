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
  LINKS
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    lib_ssf_network
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
  FILES
    option_parser_tests.cpp
    ${USER_SERVICE_FILES}
    ${COMMON_ERROR_FILES}
)
project_group(${services_tests_group_name} option_parser_tests)

# --- Socks test
add_target(socks_tests
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
    socks_tests.cpp
    ${SERVICE_TESTS_FILES}
    ${SOCKS_HELPERS_FILES}
    ${TCP_HELPERS_FILES}
    ${SSF_SOURCES}
)
project_group(${services_tests_group_name} socks_tests)

# --- Remote socks test
add_target(remote_socks_tests
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
    remote_socks_tests.cpp
    ${SERVICE_TESTS_FILES}
    ${SOCKS_HELPERS_FILES}
    ${TCP_HELPERS_FILES}
    ${SSF_SOURCES}
)
project_group(${services_tests_group_name} remote_socks_tests)

# --- Stream forward test
add_target(stream_forward_tests
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
    stream_forwarding_tests.cpp
    ${SERVICE_TESTS_FILES}
    ${TCP_HELPERS_FILES}
    ${SSF_SOURCES}
)
project_group(${services_tests_group_name} stream_forward_tests)

# --- Remote stream forward test
add_target(remote_stream_forward_tests
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
    remote_stream_forwarding_tests.cpp
    ${SERVICE_TESTS_FILES}
    ${TCP_HELPERS_FILES}
    ${SSF_SOURCES}
)
project_group(${services_tests_group_name} remote_stream_forward_tests)

# --- UDP forward test
add_target(datagram_forward_tests
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
    datagram_forward_tests.cpp
    ${SERVICE_TESTS_FILES}
    ${UDP_HELPERS_FILES}
    ${SSF_SOURCES}
)
project_group(${services_tests_group_name} datagram_forward_tests)

# --- Remote UDP forward test
add_target(remote_datagram_forward_tests
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
    remote_datagram_forward_tests.cpp
    ${SERVICE_TESTS_FILES}
    ${UDP_HELPERS_FILES}
    ${SSF_SOURCES}
)
project_group(${services_tests_group_name} remote_datagram_forward_tests)

# --- File copy from client test
file(MAKE_DIRECTORY files_to_copy)
file(MAKE_DIRECTORY files_copied)

FILE(GLOB_RECURSE TEST_FILES_TO_COPY ${CMAKE_CURRENT_SOURCE_DIR}/files_to_copy/*)

file(COPY ${TEST_FILES_TO_COPY} DESTINATION files_to_copy)

add_target(file_copy_from_client_tests
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
    file_copy_from_client_tests.cpp
    file_copy_fixture_test.h
    file_copy_fixture_test.cpp
    ${SSF_SOURCES}
)
project_group(${services_tests_group_name} file_copy_from_client_tests)

# --- Process test
add_target(process_tests
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
    process_tests.cpp
    ${SERVICE_TESTS_FILES}
    ${SSF_SOURCES}
)
project_group(${services_tests_group_name} process_tests)

# --- Remote process test
add_target(remote_process_tests
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
    remote_process_tests.cpp
    ${SERVICE_TESTS_FILES}
    ${SSF_SOURCES}
)
project_group(${services_tests_group_name} remote_process_tests)
