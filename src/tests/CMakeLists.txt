project(RUN_TESTS CXX)

enable_testing()
  
include_directories(
  ${OpenSSL_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS})

include_directories(
  ${GTEST_ROOT_DIR}/include
)

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
  ${COMMON_BOOST_FIBER_FILES}
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
  ${SSF_SOURCES}
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
  ${SSF_SOURCES}
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
  ${SSF_SOURCES}
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
  ${SSF_SOURCES}
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
  ${SSF_SOURCES}
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
  ${SSF_SOURCES}
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
  ${SSF_SOURCES}
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
  ${SSF_SOURCES}
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
  ${SSF_SOURCES}
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
  
  
# --- File copy from client test

file(MAKE_DIRECTORY ${project_BINARY_DIR}/src/tests/files_to_copy)
file(MAKE_DIRECTORY ${project_BINARY_DIR}/src/tests/files_copied)

FILE(GLOB_RECURSE TEST_FILES_TO_COPY
  "${project_SRC_DIR}/tests/files_to_copy/*"
)

file(COPY ${TEST_FILES_TO_COPY} DESTINATION ${project_BINARY_DIR}/src/tests/files_to_copy)

set(file_copy_from_client_files
  "file_copy_from_client_tests.cpp"
  ${SSF_SOURCES}
)

add_target("file_copy_from_client_tests"
  TYPE
    executable
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    ${file_copy_from_client_files}
)
target_link_libraries(file_copy_from_client_tests gtest gtest_main)
add_test(
	NAME file_copy_from_client_tests 
	COMMAND $<TARGET_FILE:file_copy_from_client_tests>)


# --- Unit tests list
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
  file_copy_from_client_tests
)