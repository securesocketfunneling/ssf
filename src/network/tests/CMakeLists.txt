enable_testing()

set(SSF_NETWORK_LAYER_TEST_FIXTURES_FILES
  "circuit_test_fixture.h"
  "datagram_protocol_helpers.h"
  "interface_protocol_helpers.h"
  "interface_test_fixture.h"
  "proxy_test_fixture.h"
  "proxy_test_fixture.cpp"
  "routing_test_fixture.h"
  "stream_protocol_helpers.h"
  "tools.h"
  "transport_test_fixture.h"
  "virtual_network_helpers.h"
  "virtual_network_helpers.cpp")

# --- Log tests
add_target(log_tests
  TYPE executable ${EXEC_FLAG} TEST
  FILES
    log_tests.cpp
  LINKS
    ssf_network
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET log_tests PROPERTY FOLDER "Unit Tests/Network layers")

# --- Queue tests
add_target(queue_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    queue_tests.cpp
  LINKS
    ssf_network
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET queue_tests PROPERTY FOLDER "Unit Tests/Network layers")

# --- Physical layer tests
add_target(physical_layer_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    physical_layer_tests.cpp
    ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES}
  LINKS
    ssf_network
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET physical_layer_tests PROPERTY FOLDER "Unit Tests/Network layers")

# --- Proxy layer tests
add_target(proxy_layer_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    proxy_layer_tests.cpp
    ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES}
  LINKS
    ssf_network
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET proxy_layer_tests PROPERTY FOLDER "Unit Tests/Network layers" )

add_target(proxy_auth_strategies_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    proxy_auth_strategies_tests.cpp
  LINKS
    ssf_network
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET proxy_auth_strategies_tests PROPERTY FOLDER "Unit Tests/Network layers" )

# --- Link layer tests
add_target(link_layer_tests
  TYPE
    executable ${EXEC_FLAG} TEST
  FILES
    link_layer_tests.cpp
    ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES}
  LINKS
    ssf_network
    gtest
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)
set_property(TARGET link_layer_tests PROPERTY FOLDER "Unit Tests/Network layers")

# --- Interface layer tests
#add_target("interface_layer_tests"
#  TYPE
#    executable ${EXEC_FLAG} TEST
#  FILES
#    interface_layer_tests.cpp
#    ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES}
#  LINKS
#    ssf_network
#    gtest
#  PREFIX_SKIP     .*/src
#  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
#)
#set_property(TARGET interface_layer_tests PROPERTY FOLDER "Unit Tests/Network layers")

# --- Routing layer tests
#add_target(routing_layer_tests
#  TYPE
#    executable ${EXEC_FLAG} TEST
#  FILES
#    routing_layer_tests.cpp
#    ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES}
#  LINKS
#    ssf_network
#    gtest
#  PREFIX_SKIP     .*/src
#  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
#)
#set_property(TARGET routing_layer_tests PROPERTY FOLDER "Unit Tests/Network layers")

# --- Transport layer tests
#add_target(transport_layer_tests
#  TYPE
#    executable ${EXEC_FLAG} TEST
#  FILES
#    "transport_layer_tests.cpp"
#    ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES}
#  LINKS
#    ssf_network
#    gtest
#  PREFIX_SKIP     .*/src
#  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
#)
#set_property(TARGET transport_layer_tests PROPERTY FOLDER "Unit Tests/Network layers")

# --- Interface System tests
#add_target(interfaces_system_tests
#  TYPE
#    executable ${EXEC_FLAG} TEST
#  FILES
#    interfaces_system_tests.cpp
#    ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES}
#  LINKS
#    ssf_network
#    gtest
#  PREFIX_SKIP     .*/src
#  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
#)
#set_property(TARGET interfaces_system_tests PROPERTY FOLDER "Unit Tests/Network layers")

# --- Router System tests
#add_target(router_system_tests
#  TYPE
#    executable ${EXEC_FLAG} TEST
#  FILES
#    router_system_tests.cpp
#    ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES}
#  LINKS
#    ssf_network
#    gtest
#  PREFIX_SKIP     .*/src
#  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
#)
#set_property(TARGET router_system_tests PROPERTY FOLDER "Unit Tests/Network layers")
