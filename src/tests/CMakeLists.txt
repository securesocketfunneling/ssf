project(RUN_TESTS CXX)

enable_testing()
  
include_directories(
  ${OpenSSL_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS})

include_directories(
  ${GTEST_ROOT_DIR}/include
)

file(GLOB_RECURSE LAYER_TEST_FIXTURES_FILES
      "${project_SRC_DIR}/tests/*.h")

# --- Load config test
add_target("load_config_tests"
  TYPE
    executable ${EXEC_FLAG} TEST
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    "load_config_tests.cpp"
    ${COMMON_CONFIG_FILES}
    ${COMMON_ERROR_FILES}
)

# --- Fiber asio tests
add_target("fiber_asio_tests"
  TYPE
    executable ${EXEC_FLAG} TEST
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    "fiber_asio_tests.cpp"
    ${COMMON_ERROR_FILES}
)

# --- SSF Client Server tests
add_target("ssf_client_server_tests"
  TYPE
    executable ${EXEC_FLAG} TEST
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    "ssf_client_server_tests.cpp"
    ${VERSIONS_FILES}
    ${COMMON_CONFIG_FILES}
    ${COMMON_ERROR_FILES}
    ${CORE_INIT_PACKETS}
    ${SERVICES_SOCKS_FILES}
)

# --- SSF Client Server cipher suites tests
add_target("ssf_client_server_cipher_suites_tests"
  TYPE
    executable ${EXEC_FLAG} TEST
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    "ssf_client_server_cipher_suites_tests.cpp"
    ${VERSIONS_FILES}
    ${COMMON_CONFIG_FILES}
    ${COMMON_ERROR_FILES}
    ${CORE_INIT_PACKETS}
    ${SERVICES_SOCKS_FILES}
)

# --- Socks test
add_target("socks_tests"
  TYPE
    executable ${EXEC_FLAG} TEST
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    "socks_tests.cpp"
    ${VERSIONS_FILES}
    ${COMMON_CONFIG_FILES}
    ${COMMON_ERROR_FILES}
    ${CORE_INIT_PACKETS}
    ${SERVICES_SOCKS_FILES}
)

# --- Remote socks test
add_target("remote_socks_tests"
  TYPE
    executable ${EXEC_FLAG} TEST
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    "remote_socks_tests.cpp"
    ${VERSIONS_FILES}
    ${COMMON_CONFIG_FILES}
    ${COMMON_ERROR_FILES}
    ${CORE_INIT_PACKETS}
    ${SERVICES_SOCKS_FILES}
)

# --- Stream forward test
add_target("stream_forward_tests"
  TYPE
    executable ${EXEC_FLAG} TEST
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    "stream_forwarding_tests.cpp"
    ${VERSIONS_FILES}
    ${COMMON_CONFIG_FILES}
    ${COMMON_ERROR_FILES}
    ${CORE_INIT_PACKETS}
    ${SERVICES_SOCKS_FILES}
)

# --- Remote stream forward test
add_target("remote_stream_forward_tests"
  TYPE
    executable ${EXEC_FLAG} TEST
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    "remote_stream_forwarding_tests.cpp"
    ${VERSIONS_FILES}
    ${COMMON_CONFIG_FILES}
    ${COMMON_ERROR_FILES}
    ${CORE_INIT_PACKETS}
    ${SERVICES_SOCKS_FILES}
)

# --- UDP forward test
add_target("udp_forward_tests"
  TYPE
    executable ${EXEC_FLAG} TEST
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    "udp_forwarding_tests.cpp"
    ${VERSIONS_FILES}
    ${COMMON_CONFIG_FILES}
    ${COMMON_ERROR_FILES}
    ${CORE_INIT_PACKETS}
    ${SERVICES_SOCKS_FILES}
)

# --- Remote UDP forward test
add_target("remote_udp_forward_tests"
  TYPE
    executable ${EXEC_FLAG} TEST
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    "remote_udp_forwarding_tests.cpp"
    ${VERSIONS_FILES}
    ${COMMON_CONFIG_FILES}
    ${COMMON_ERROR_FILES}
    ${CORE_INIT_PACKETS}
    ${SERVICES_SOCKS_FILES}
)

# --- Bouncing chain test
add_target("bouncing_tests"
  TYPE
    executable ${EXEC_FLAG} TEST
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    "bouncing_tests.cpp"
    ${VERSIONS_FILES}
    ${COMMON_CONFIG_FILES}
    ${COMMON_ERROR_FILES}
    ${CORE_INIT_PACKETS}
    ${SERVICES_SOCKS_FILES}
)

# --- Physical layer tests
add_target("physical_layer_tests"
  TYPE
    executable ${EXEC_FLAG} TEST
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    "physical_layer_tests.cpp"
    ${VERSIONS_FILES}
    ${COMMON_CONFIG_FILES}
    ${COMMON_ERROR_FILES}
    ${COMMON_IO_FILES}
    ${CORE_INIT_PACKETS}
    ${CORE_VIRTUAL_NETWORK_FILES}
    ${SERVICES_SOCKS_FILES}
    ${LAYER_TEST_FIXTURES_FILES}
)

# --- Link layer tests
add_target("link_layer_tests"
  TYPE
    executable ${EXEC_FLAG} TEST
  LINKS 
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    PREFIX_SKIP     .*/src
    HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?" 
  FILES
    "link_layer_tests.cpp"
    ${VERSIONS_FILES}
    ${COMMON_CONFIG_FILES}
    ${COMMON_IO_FILES}
    ${COMMON_ERROR_FILES}
    ${CORE_INIT_PACKETS}
    ${CORE_VIRTUAL_NETWORK_FILES}
    ${SERVICES_SOCKS_FILES}
    ${LAYER_TEST_FIXTURES_FILES}
)