#ifndef SSF_TESTS_TRANSPORT_TEST_FIXTURE_H_
#define SSF_TESTS_TRANSPORT_TEST_FIXTURE_H_

#include <gtest/gtest.h>

#include <memory>

#include <boost/asio/io_service.hpp>

#include "tests/routing_test_fixture.h"

#include "ssf/layer/parameters.h"

#include "ssf/layer/multiplexing/basic_multiplexer_protocol.h"
#include "ssf/layer/multiplexing/protocol_multiplex_id.h"
#include "ssf/layer/multiplexing/port_multiplex_id.h"

#include "ssf/layer/congestion/drop_tail_policy.h"

class TransportTestFixture : public RoutingTestFixture {
 protected:
  using NetworkProtocol =
      ssf::layer::network::basic_NetworkProtocol<InterfaceProtocol>;
  using RouterType =
      ssf::layer::routing::basic_Router<NetworkProtocol>;

  using StubTransportProtocol =
      ssf::layer::multiplexing::basic_MultiplexedProtocol<
          NetworkProtocol, ssf::layer::multiplexing::ProtocolID,
          ssf::layer::congestion::DropTailPolicy<100>>;

  using DatagramTransportProtocol =
      ssf::layer::multiplexing::basic_MultiplexedProtocol<
          StubTransportProtocol, ssf::layer::multiplexing::PortID,
          ssf::layer::congestion::DropTailPolicy<100>>;

 protected:
  TransportTestFixture()
      : RoutingTestFixture(),
        socket_1_(this->io_service_),
        socket_3_(this->io_service_),
        socket_5_(this->io_service_),
        socket_7_(this->io_service_) {}

   virtual ~TransportTestFixture() {}

  virtual void SetUp() {
    RoutingTestFixture::SetUp();

    boost::system::error_code ec;
    NetworkProtocol::resolver resolver(this->io_service_);

    {
      ssf::layer::LayerParameters socket1_network_parameters;
      socket1_network_parameters["network_id"] = "11";
      ssf::layer::LayerParameters socket1_interface_parameters;
      socket1_interface_parameters["interface_id"] = "simple_lo2";
      ssf::layer::ParameterStack parameters1;
      parameters1.push_back(socket1_network_parameters);
      parameters1.push_back(socket1_interface_parameters);
      auto socket1_endpoint_it = resolver.resolve(parameters1, ec);
      NetworkProtocol::endpoint socket1_local_endpoint(*socket1_endpoint_it);

      socket_1_.open();
      socket_1_.bind(socket1_local_endpoint, ec);
    }
    {
      ssf::layer::LayerParameters socket1_network_parameters;
      socket1_network_parameters["network_id"] = "33";
      ssf::layer::LayerParameters socket1_interface_parameters;
      socket1_interface_parameters["interface_id"] = "simple_tls_lo1";
      ssf::layer::ParameterStack parameters1;
      parameters1.push_back(socket1_network_parameters);
      parameters1.push_back(socket1_interface_parameters);
      auto socket1_endpoint_it = resolver.resolve(parameters1, ec);
      NetworkProtocol::endpoint socket1_local_endpoint(*socket1_endpoint_it);

      socket_3_.open();
      socket_3_.bind(socket1_local_endpoint, ec);
    }
    {
      ssf::layer::LayerParameters socket1_network_parameters;
      socket1_network_parameters["network_id"] = "55";
      ssf::layer::LayerParameters socket1_interface_parameters;
      socket1_interface_parameters["interface_id"] = "circuit_lo1";
      ssf::layer::ParameterStack parameters1;
      parameters1.push_back(socket1_network_parameters);
      parameters1.push_back(socket1_interface_parameters);
      auto socket1_endpoint_it = resolver.resolve(parameters1, ec);
      NetworkProtocol::endpoint socket1_local_endpoint(*socket1_endpoint_it);

      socket_5_.open();
      socket_5_.bind(socket1_local_endpoint, ec);
    }
    {
      ssf::layer::LayerParameters socket1_network_parameters;
      socket1_network_parameters["network_id"] = "77";
      ssf::layer::LayerParameters socket1_interface_parameters;
      socket1_interface_parameters["interface_id"] = "circuit_tls_lo1";
      ssf::layer::ParameterStack parameters1;
      parameters1.push_back(socket1_network_parameters);
      parameters1.push_back(socket1_interface_parameters);
      auto socket1_endpoint_it = resolver.resolve(parameters1, ec);
      NetworkProtocol::endpoint socket1_local_endpoint(*socket1_endpoint_it);

      socket_7_.open();
      socket_7_.bind(socket1_local_endpoint, ec);
    }

    Wait();
  }

  virtual void TearDown() {
    RoutingTestFixture::TearDown();
  }

 private:
  bool Wait() { return true; }

protected:
  NetworkProtocol::socket socket_1_;
  NetworkProtocol::socket socket_3_;
  NetworkProtocol::socket socket_5_;
  NetworkProtocol::socket socket_7_;
};

#endif  // SSF_TESTS_TRANSPORT_TEST_FIXTURE_H_
