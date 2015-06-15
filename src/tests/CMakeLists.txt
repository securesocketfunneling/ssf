project(RUN_TESTS CXX)

enable_testing()
  
include_directories(
  ${OpenSSL_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS})

include_directories(
  ${GTEST_ROOT_DIR}/include
)

set(LAYER_TEST_FEATURES_FILES
      "${project_SRC_DIR}/tests/virtual_network_helpers.h"
      "${project_SRC_DIR}/tests/stream_protocol_helpers.h"
      "${project_SRC_DIR}/tests/datagram_protocol_helpers.h")
 
# --- Load config test
set(load_config_test_source_files
  "load_config_tests.cpp"
  ${COMMON_CONFIG_FILES}
  ${COMMON_ERROR_FILES}
)

add_target("load_config_tests"
  TYPE
    executable
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    ${load_config_test_source_files}
)
target_link_libraries(load_config_tests gtest gtest_main)
add_test(
	NAME load_config_tests 
	COMMAND $<TARGET_FILE:load_config_tests>)

# --- Fiber asio tests
set(fiber_asio_test_source_files
  "fiber_asio_tests.cpp"
  ${COMMON_ERROR_FILES}
)

add_target("fiber_asio_tests"
  TYPE
    executable
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    ${fiber_asio_test_source_files}
)
target_link_libraries(fiber_asio_tests gtest gtest_main)
add_test(
	NAME fiber_asio_tests 
	COMMAND $<TARGET_FILE:fiber_asio_tests>)

# --- SSF Client Server tests
set(ssf_client_server_source_files
  "ssf_client_server_tests.cpp"
  ${VERSIONS_FILES}
  ${COMMON_CONFIG_FILES}
  ${COMMON_ERROR_FILES}
  ${CORE_INIT_PACKETS}
  ${SERVICES_SOCKS_FILES}
)

add_target("ssf_client_server_tests"
  TYPE
    executable
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    ${ssf_client_server_source_files}
)
target_link_libraries(ssf_client_server_tests gtest gtest_main)
add_test(
	NAME ssf_client_server_tests 
	COMMAND $<TARGET_FILE:ssf_client_server_tests>)

# --- SSF Client Server cipher suites tests
set(ssf_client_server_cipher_suites_source_files
  "ssf_client_server_cipher_suites_tests.cpp"
  ${VERSIONS_FILES}
  ${COMMON_CONFIG_FILES}
  ${COMMON_ERROR_FILES}
  ${CORE_INIT_PACKETS}
  ${SERVICES_SOCKS_FILES}
)

add_target("ssf_client_server_cipher_suites_tests"
  TYPE
    executable
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    ${ssf_client_server_cipher_suites_source_files}
)
target_link_libraries(ssf_client_server_cipher_suites_tests gtest gtest_main)
add_test(
	NAME ssf_client_server_cipher_suites_tests 
	COMMAND $<TARGET_FILE:ssf_client_server_cipher_suites_tests>)

# --- Socks test
set(socks_test_source_files
  "socks_tests.cpp"
  ${VERSIONS_FILES}
  ${COMMON_CONFIG_FILES}
  ${COMMON_ERROR_FILES}
  ${CORE_INIT_PACKETS}
  ${SERVICES_SOCKS_FILES}
)

add_target("socks_tests"
  TYPE
    executable
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    ${socks_test_source_files}
)
target_link_libraries(socks_tests gtest gtest_main)
add_test(
	NAME socks_tests 
	COMMAND $<TARGET_FILE:socks_tests>)

# --- Remote socks test
set(remote_socks_test_source_files
  "remote_socks_tests.cpp"
  ${VERSIONS_FILES}
  ${COMMON_CONFIG_FILES}
  ${COMMON_ERROR_FILES}
  ${CORE_INIT_PACKETS}
  ${SERVICES_SOCKS_FILES}
)

add_target("remote_socks_tests"
  TYPE
    executable
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    ${remote_socks_test_source_files}
)
target_link_libraries(remote_socks_tests gtest gtest_main)
add_test(
	NAME remote_socks_tests 
	COMMAND $<TARGET_FILE:remote_socks_tests>)

# --- Stream forward test
set(stream_forwarding_test_source_files
  "stream_forwarding_tests.cpp"
  ${VERSIONS_FILES}
  ${COMMON_CONFIG_FILES}
  ${COMMON_ERROR_FILES}
  ${CORE_INIT_PACKETS}
  ${SERVICES_SOCKS_FILES}
)

add_target("stream_forward_tests"
  TYPE
    executable
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    ${stream_forwarding_test_source_files}
)
target_link_libraries(stream_forward_tests gtest gtest_main)
add_test(
	NAME stream_forward_tests 
	COMMAND $<TARGET_FILE:stream_forward_tests>)

# --- Remote stream forward test
set(remote_stream_forwarding_test_source_files
  "remote_stream_forwarding_tests.cpp"
  ${VERSIONS_FILES}
  ${COMMON_CONFIG_FILES}
  ${COMMON_ERROR_FILES}
  ${CORE_INIT_PACKETS}
  ${SERVICES_SOCKS_FILES}
)

add_target("remote_stream_forward_tests"
  TYPE
    executable
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    ${remote_stream_forwarding_test_source_files}
)
target_link_libraries(remote_stream_forward_tests gtest gtest_main)
add_test(
	NAME remote_stream_forward_tests 
	COMMAND $<TARGET_FILE:remote_stream_forward_tests>)

# --- UDP forward test
set(udp_test_source_files
  "udp_forwarding_tests.cpp"
  ${VERSIONS_FILES}
  ${COMMON_CONFIG_FILES}
  ${COMMON_ERROR_FILES}
  ${CORE_INIT_PACKETS}
  ${SERVICES_SOCKS_FILES}
)

add_target("udp_forward_tests"
  TYPE
    executable
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    ${udp_test_source_files}
)
target_link_libraries(udp_forward_tests gtest gtest_main)
add_test(
	NAME udp_forward_tests 
	COMMAND $<TARGET_FILE:udp_forward_tests>)

# --- Remote UDP forward test
set(remote_udp_test_source_files
  "remote_udp_forwarding_tests.cpp"
  ${VERSIONS_FILES}
  ${COMMON_CONFIG_FILES}
  ${COMMON_ERROR_FILES}
  ${CORE_INIT_PACKETS}
  ${SERVICES_SOCKS_FILES}
)

add_target("remote_udp_forward_tests"
  TYPE
    executable
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    ${remote_udp_test_source_files}
)
target_link_libraries(remote_udp_forward_tests gtest gtest_main)
add_test(
	NAME remote_udp_forward_tests 
	COMMAND $<TARGET_FILE:remote_udp_forward_tests>)

# --- Bouncing chain test
set(bouncing_test_source_files
  "bouncing_tests.cpp"
  ${VERSIONS_FILES}
  ${COMMON_CONFIG_FILES}
  ${COMMON_ERROR_FILES}
  ${CORE_INIT_PACKETS}
  ${SERVICES_SOCKS_FILES}
)

add_target("bouncing_tests"
  TYPE
    executable
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    ${bouncing_test_source_files}
)
target_link_libraries(bouncing_tests gtest gtest_main)
add_test(
	NAME bouncing_tests 
	COMMAND $<TARGET_FILE:bouncing_tests>)

# --- Physical layer tests
set(physical_layer_source_files
  "physical_layer_tests.cpp"
  ${VERSIONS_FILES}
  ${COMMON_CONFIG_FILES}
  ${COMMON_ERROR_FILES}
  ${COMMON_IO_FILES}
  ${CORE_INIT_PACKETS}
  ${CORE_VIRTUAL_NETWORK_FILES}
  ${SERVICES_SOCKS_FILES}
  ${LAYER_TEST_FEATURES_FILES}
)

add_target("physical_layer_tests"
  TYPE
    executable
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    ${physical_layer_source_files}
)
target_link_libraries(physical_layer_tests gtest gtest_main)
add_test(
	NAME physical_layer_tests 
	COMMAND $<TARGET_FILE:physical_layer_tests>)
  

# Group all tests in one directory
  project_group(
    "Unit tests"
    load_config_tests
    fiber_asio_tests
    ssf_client_server_tests
    ssf_client_server_cipher_suites_tests
    socks_tests
    remote_socks_tests
    stream_forward_tests
    remote_stream_forward_tests
    udp_forward_tests
    remote_udp_forward_tests
    bouncing_tests
    physical_layer_tests
  )
