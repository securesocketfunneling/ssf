#include <gtest/gtest.h>

#include "ssf/layer/parameters.h"

#include "tests/datagram_protocol_helpers.h"
#include "tests/routing_test_fixture.h"

TEST_F(RoutingTestFixture, NetworkResolvingTest) {
  boost::asio::io_service io_service;
  boost::system::error_code ec;
  RoutedProtocol::resolver resolver(io_service);

  {
    ssf::layer::LayerParameters routed_socket1_parameters;
    routed_socket1_parameters["network_address"] = "11";
    routed_socket1_parameters["router"] = "router1";
    ssf::layer::ParameterStack parameters_socket1;
    parameters_socket1.push_back(routed_socket1_parameters);
    resolver.resolve(parameters_socket1, ec);
    ASSERT_NE(0, ec.value());
  }

  {
    ssf::layer::LayerParameters routed_socket1_parameters;
    routed_socket1_parameters["network_address"] = "1";
    routed_socket1_parameters["router"] = "router1";
    ssf::layer::ParameterStack parameters_socket1;
    parameters_socket1.push_back(routed_socket1_parameters);
    resolver.resolve(parameters_socket1, ec);
    ASSERT_EQ(0, ec.value());
  }
}

TEST_F(RoutingTestFixture, RoutingPacketLocallyTest) {
  ssf::layer::LayerParameters routed_socket1_parameters;
  routed_socket1_parameters["network_address"] = "7";
  routed_socket1_parameters["router"] = "router1";
  ssf::layer::ParameterStack parameters_socket1;
  parameters_socket1.push_back(routed_socket1_parameters);

  ssf::layer::LayerParameters routed_socket2_parameters;
  routed_socket2_parameters["network_address"] = "5";
  routed_socket2_parameters["router"] = "router1";
  ssf::layer::ParameterStack parameters_socket2;
  parameters_socket2.push_back(routed_socket2_parameters);

  TestDatagramProtocolPerfHalfDuplex<RoutedProtocol>(parameters_socket1,
                                                     parameters_socket2, 1000);

  TestDatagramProtocolPerfFullDuplex<RoutedProtocol>(parameters_socket1,
                                                     parameters_socket2, 1000);
}

TEST_F(RoutingTestFixture, RoutingPacketRemotelyTest) {
  ssf::layer::LayerParameters routed_socket1_parameters;
  routed_socket1_parameters["network_address"] = "7";
  routed_socket1_parameters["router"] = "router1";
  ssf::layer::ParameterStack parameters_socket1;
  parameters_socket1.push_back(routed_socket1_parameters);

  ssf::layer::LayerParameters routed_socket2_parameters;
  routed_socket2_parameters["network_address"] = "2";
  routed_socket2_parameters["router"] = "router2";
  ssf::layer::ParameterStack parameters_socket2;
  parameters_socket2.push_back(routed_socket2_parameters);

  TestDatagramProtocolPerfHalfDuplex<RoutedProtocol>(parameters_socket1,
                                                     parameters_socket2, 1000);

  TestDatagramProtocolPerfFullDuplex<RoutedProtocol>(parameters_socket1,
                                                     parameters_socket2, 1000);
}
