enable_testing()

# Copy test configuration files
file(GLOB_RECURSE SYSTEM_CONFIG_TEST_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/system/*.json")
set(PROXY_CONFIG_TEST_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/proxy/proxy.json")

file(MAKE_DIRECTORY proxy)
file(MAKE_DIRECTORY system)

file(COPY ${PROXY_CONFIG_TEST_FILES} DESTINATION proxy)
file(COPY ${SYSTEM_CONFIG_TEST_FILES} DESTINATION system)

include_directories(${OpenSSL_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS}
  ${SSF_FRAMEWORK_GTEST_ROOT_DIR}/include)

set(FIXTURES_FILES
  circuit_test_fixture.h
  datagram_protocol_helpers.h
  interface_protocol_helpers.h
  interface_test_fixture.h
  proxy_test_fixture.h
  proxy_test_fixture.cpp
  routing_test_fixture.h
  stream_protocol_helpers.h
  tools.h
  transport_test_fixture.h
  virtual_network_helpers.h
  virtual_network_helpers.cpp)

# --- Log tests
add_target(log_tests
  TYPE executable ${EXEC_FLAG} TEST
  LINKS
    lib_ssf_network  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
  FILES
    log_tests.cpp
)
project_group("Unit Tests/Network layers" log_tests)

# --- Queue tests
add_target(queue_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "queue_tests.cpp"
  LINKS
    lib_ssf_network
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group("Unit Tests/Network layers" queue_tests)

# --- Physical layer tests
add_target(physical_layer_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "physical_layer_tests.cpp"
    ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES}
  LINKS
    lib_ssf_network
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group("Unit Tests/Network layers" physical_layer_tests)

# --- Proxy layer tests
add_target(proxy_layer_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "proxy_layer_tests.cpp"
    ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES}
  LINKS
    lib_ssf_network
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group("Unit Tests/Network layers"  proxy_layer_tests)

add_target(proxy_auth_strategies_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "proxy_auth_strategies_tests.cpp"
  LINKS
    lib_ssf_network
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group("Unit Tests/Network layers"  proxy_auth_strategies_tests)

# --- Link layer tests
add_target(link_layer_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "link_layer_tests.cpp"
    ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES}
  LINKS
    lib_ssf_network
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group("Unit Tests/Network layers" link_layer_tests)

# --- Interface layer tests
add_target(interface_layer_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    "interface_layer_tests.cpp"
    ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES}
  LINKS
    lib_ssf_network
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
project_group("Unit Tests/Network layers" interface_layer_tests)

# --- Routing layer tests
#add_target(routing_layer_tests
#  TYPE
#    executable ${EXEC_FLAG} TEST
#  FILES
#    "routing_layer_tests.cpp"
#    ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES}
#  LINKS
#    lib_ssf_network
#    gtest
#  PREFIX_SKIP     .*/src
#  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
#)
#project_group("Unit Tests/Network layers" routing_layer_tests)

# --- Transport layer tests
#add_target(transport_layer_tests
#  TYPE
#    executable ${EXEC_FLAG} TEST
#  FILES
#    "transport_layer_tests.cpp"
#    ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES}
#  LINKS
#    lib_ssf_network
#    gtest
#  PREFIX_SKIP     .*/src
#  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
#)
#project_group("Unit Tests/Network layers" transport_layer_tests)

# --- Interface System tests
#add_target(interfaces_system_tests
#  TYPE
#    executable ${EXEC_FLAG} TEST
#  FILES
#    "interfaces_system_tests.cpp"
#    ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES}
#  LINKS
#    lib_ssf_network
#    gtest
#  PREFIX_SKIP     .*/src
#  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
#)
#project_group("Unit Tests/Network layers" interfaces_system_tests)

# --- Router System tests
#add_target(router_system_tests
#  TYPE
#    executable ${EXEC_FLAG} TEST
#  FILES
#    "router_system_tests.cpp"
#    ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES}
#  LINKS
#    lib_ssf_network
#    gtest
#  PREFIX_SKIP     .*/src
#  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
#)
#project_group("Unit Tests/Network layers" router_system_tests)
