set(SSF_NETWORK_LAYER_TEST_FIXTURES_FILES
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
add_executable(log_tests EXCLUDE_FROM_ALL log_tests.cpp)
target_link_libraries(log_tests ssf_network gtest)
add_unit_test(log_tests)
set_property(TARGET log_tests PROPERTY FOLDER "Unit Tests/Network layers")

# --- Queue tests
add_executable(queue_tests EXCLUDE_FROM_ALL queue_tests.cpp)
target_link_libraries(queue_tests ssf_network gtest)
add_unit_test(queue_tests)
set_property(TARGET queue_tests PROPERTY FOLDER "Unit Tests/Network layers")

# --- Physical layer tests
add_executable(physical_layer_tests EXCLUDE_FROM_ALL physical_layer_tests.cpp ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES})
target_link_libraries(physical_layer_tests ssf_network gtest)
add_unit_test(physical_layer_tests)
set_property(TARGET physical_layer_tests PROPERTY FOLDER "Unit Tests/Network layers")

# --- Proxy layer tests
add_executable(proxy_layer_tests EXCLUDE_FROM_ALL proxy_layer_tests.cpp ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES})
target_link_libraries(proxy_layer_tests ssf_network gtest)
add_unit_test(proxy_layer_tests)
set_property(TARGET proxy_layer_tests PROPERTY FOLDER "Unit Tests/Network layers" )

add_executable(proxy_auth_strategies_tests EXCLUDE_FROM_ALL proxy_auth_strategies_tests.cpp)
target_link_libraries(proxy_auth_strategies_tests ssf_network gtest)
add_unit_test(proxy_auth_strategies_tests)
set_property(TARGET proxy_auth_strategies_tests PROPERTY FOLDER "Unit Tests/Network layers" )

# --- Link layer tests
add_executable(link_layer_tests EXCLUDE_FROM_ALL link_layer_tests.cpp ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES})
target_link_libraries(link_layer_tests ssf_network gtest)
add_unit_test(link_layer_tests)
set_property(TARGET link_layer_tests PROPERTY FOLDER "Unit Tests/Network layers")

# --- Interface layer tests
#add_executable(interface_layer_tests EXCLUDE_FROM_ALL interface_layer_tests.cpp ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES})
#target_link_libraries(interface_layer_tests ssf_network gtest)
#add_unit_test(interface_layer_tests)
#set_property(TARGET interface_layer_tests PROPERTY FOLDER "Unit Tests/Network layers")

# --- Routing layer tests
#add_executable(routing_layer_tests EXCLUDE_FROM_ALL routing_layer_tests.cpp ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES})
#target_link_libraries(routing_layer_tests ssf_network gtest)
#add_unit_test(routing_layer_tests)
#set_property(TARGET routing_layer_tests PROPERTY FOLDER "Unit Tests/Network layers")

# --- Transport layer tests
#add_executable(transport_layer_tests EXCLUDE_FROM_ALL transport_layer_tests.cpp ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES})
#target_link_libraries(transport_layer_tests ssf_network gtest)
#add_unit_test(transport_layer_tests)
#set_property(TARGET transport_layer_tests PROPERTY FOLDER "Unit Tests/Network layers")

# --- Interface System tests
#add_executable(interfaces_system_tests EXCLUDE_FROM_ALL interfaces_system_tests.cpp ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES})
#target_link_libraries(interfaces_system_tests ssf_network gtest)
#add_unit_test(interfaces_system_tests)
#set_property(TARGET interfaces_system_tests PROPERTY FOLDER "Unit Tests/Network layers")

# --- Router System tests
#add_executable(router_system_tests EXCLUDE_FROM_ALL router_system_tests.cpp ${SSF_NETWORK_LAYER_TEST_FIXTURES_FILES})
#target_link_libraries(router_system_tests ssf_network gtest)
#add_unit_test(router_system_tests)
#set_property(TARGET router_system_tests PROPERTY FOLDER "Unit Tests/Network layers")
