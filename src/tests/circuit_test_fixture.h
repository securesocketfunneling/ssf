#ifndef SSF_TESTS_CIRCUIT_TEST_FIXTURE_H_
#define SSF_TESTS_CIRCUIT_TEST_FIXTURE_H_

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <string>

#include <boost/asio/io_service.hpp>

#include <boost/thread.hpp>

#include "tests/virtual_network_helpers.h"

#include "core/virtual_network/parameters.h"

#include "core/virtual_network/data_link_layer/circuit_helpers.h"
#include "core/virtual_network/data_link_layer/basic_circuit_protocol.h"
#include "core/virtual_network/data_link_layer/simple_circuit_policy.h"
#include "core/virtual_network/physical_layer/tcp.h"
#include "core/virtual_network/physical_layer/tlsotcp.h"

#include "tests/virtual_network_helpers.h"

class CircuitTestFixture : public ::testing::Test {
 public:
  typedef virtual_network::physical_layer::TLSboTCPPhysicalLayer
      TLSPhysicalProtocol;
  typedef virtual_network::data_link_layer::basic_CircuitProtocol<
      TLSPhysicalProtocol, virtual_network::data_link_layer::CircuitPolicy>
      TLSCircuitProtocol;

  typedef virtual_network::physical_layer::TCPPhysicalLayer PhysicalProtocol;
  typedef virtual_network::data_link_layer::basic_CircuitProtocol<
      PhysicalProtocol, virtual_network::data_link_layer::CircuitPolicy>
      CircuitProtocol;

 protected:
  typedef std::map<uint32_t, TLSCircuitProtocol::acceptor> TLSAcceptorsMap;
  typedef TLSCircuitProtocol::resolver TLSResolver;

  typedef std::map<uint32_t, CircuitProtocol::acceptor> AcceptorsMap;
  typedef CircuitProtocol::resolver Resolver;

 protected:
   CircuitTestFixture()
      : io_service_(),
        ssl_circuit_acceptors_(),
        circuit_acceptors_(),
        ssl_resolver_(io_service_),
        resolver_(io_service_),
        threads_(),
        p_work_(new boost::asio::io_service::work((io_service_))) {}

  virtual void SetUp() {
    InitTLSCircuitNodes();
    InitCircuitNodes();
    auto lambda = [this]() { this->io_service_.run(); };

    for (uint16_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
      threads_.create_thread(lambda);
    }
  }

  virtual void TearDown() {
    boost::system::error_code ec;
    for (auto& ssl_circuit_pair : ssl_circuit_acceptors_) {
      ssl_circuit_pair.second.close(ec);
    }
    ssl_circuit_acceptors_.clear();
    for (auto& circuit_pair : circuit_acceptors_) {
      circuit_pair.second.close(ec);
    }
    circuit_acceptors_.clear();
    p_work_.reset();
    threads_.join_all();
  }

  virtual_network::data_link_layer::NodeParameterList
  GetClientTLSNodes() {
    virtual_network::data_link_layer::NodeParameterList nodes;
    nodes.PushBackNode();
    nodes.AddTopLayerToBackNode({{"addr", "127.0.0.1"}, {"port", "8000"}});
    nodes.AddTopLayerToBackNode(
        tests::virtual_network_helpers::tls_client_parameters);
    nodes.PushBackNode();
    nodes.AddTopLayerToBackNode({{"addr", "127.0.0.1"}, {"port", "8001"}});
    nodes.AddTopLayerToBackNode(
        tests::virtual_network_helpers::tls_client_parameters);

    return nodes;
  }

  virtual_network::data_link_layer::NodeParameterList GetClientNodes() {
    virtual_network::data_link_layer::NodeParameterList nodes;
    nodes.PushBackNode();
    nodes.AddTopLayerToBackNode({{"addr", "127.0.0.1"}, {"port", "7000"}});
    nodes.PushBackNode();
    nodes.AddTopLayerToBackNode({{"addr", "127.0.0.1"}, {"port", "7001"}});

    return nodes;
  }

 private:
  void InitTLSCircuitNodes() {
    boost::system::error_code ec;
    // TLS Circuit node listening on 8000
    virtual_network::LayerParameters hop1_physical_parameters;
    hop1_physical_parameters["port"] = "8000";

    virtual_network::ParameterStack hop1_next_layers_parameters;
    hop1_next_layers_parameters.push_front(hop1_physical_parameters);
    hop1_next_layers_parameters.push_front(
        tests::virtual_network_helpers::tls_server_parameters);

    virtual_network::ParameterStack hop1_parameters(
        virtual_network::data_link_layer::
            make_forwarding_acceptor_parameter_stack(
                "ssl_hop1", hop1_next_layers_parameters));

    auto hop1_endpoint_it = ssl_resolver_.resolve(hop1_parameters, ec);
    TLSCircuitProtocol::endpoint hop1_endpoint(*hop1_endpoint_it);

    // TLS Circuit node listening on 8001
    virtual_network::LayerParameters hop2_physical_parameters;
    hop2_physical_parameters["port"] = "8001";

    virtual_network::ParameterStack hop2_next_layers_parameters;
    hop2_next_layers_parameters.push_front(hop2_physical_parameters);
    hop2_next_layers_parameters.push_front(
        tests::virtual_network_helpers::tls_server_parameters);

    virtual_network::ParameterStack hop2_parameters(
        virtual_network::data_link_layer::
            make_forwarding_acceptor_parameter_stack(
                "ssl_hop2", hop2_next_layers_parameters));

    auto hop2_endpoint_it = ssl_resolver_.resolve(hop2_parameters, ec);
    TLSCircuitProtocol::endpoint hop2_endpoint(*hop2_endpoint_it);

    auto hop1_it = ssl_circuit_acceptors_.emplace(
        1, TLSCircuitProtocol::acceptor(io_service_));
    boost::system::error_code hop1_ec;
    hop1_it.first->second.open();
    hop1_it.first->second.set_option(boost::asio::socket_base::reuse_address(true),
                                     ec);
    hop1_it.first->second.bind(hop1_endpoint, hop1_ec);
    hop1_it.first->second.listen(100, hop1_ec);

    auto hop2_it = ssl_circuit_acceptors_.emplace(
        2, TLSCircuitProtocol::acceptor(io_service_));
    boost::system::error_code hop2_ec;
    hop2_it.first->second.open();
    hop2_it.first->second.set_option(boost::asio::socket_base::reuse_address(true),
                                     ec);
    hop2_it.first->second.bind(hop2_endpoint, hop2_ec);
    hop2_it.first->second.listen(100, hop2_ec);
  }

  void InitCircuitNodes() {
    boost::system::error_code ec;
    // Circuit node listening on 7000
    virtual_network::LayerParameters hop1_physical_parameters;
    hop1_physical_parameters["port"] = "7000";

    virtual_network::ParameterStack hop1_next_layers_parameters;
    hop1_next_layers_parameters.push_front(hop1_physical_parameters);

    virtual_network::ParameterStack hop1_parameters(
        virtual_network::data_link_layer::
            make_forwarding_acceptor_parameter_stack(
                "hop1", hop1_next_layers_parameters));

    auto hop1_endpoint_it = resolver_.resolve(hop1_parameters, ec);
    CircuitProtocol::endpoint hop1_endpoint(*hop1_endpoint_it);

    // Circuit node listening on 7001
    virtual_network::LayerParameters hop2_physical_parameters;
    hop2_physical_parameters["port"] = "7001";

    virtual_network::ParameterStack hop2_next_layers_parameters;
    hop2_next_layers_parameters.push_front(hop2_physical_parameters);

    virtual_network::ParameterStack hop2_parameters(
        virtual_network::data_link_layer::
            make_forwarding_acceptor_parameter_stack(
                "hop2", hop2_next_layers_parameters));

    auto hop2_endpoint_it = resolver_.resolve(hop2_parameters, ec);
    CircuitProtocol::endpoint hop2_endpoint(*hop2_endpoint_it);

    auto hop1_it =
        circuit_acceptors_.emplace(1, CircuitProtocol::acceptor(io_service_));
    boost::system::error_code hop1_ec;
    hop1_it.first->second.open();
    hop1_it.first->second.set_option(boost::asio::socket_base::reuse_address(true),
                                     ec);
    hop1_it.first->second.bind(hop1_endpoint, hop1_ec);
    hop1_it.first->second.listen(100, hop1_ec);

    auto hop2_it =
        circuit_acceptors_.emplace(2, CircuitProtocol::acceptor(io_service_));
    boost::system::error_code hop2_ec;
    hop2_it.first->second.open();
    hop2_it.first->second.set_option(boost::asio::socket_base::reuse_address(true),
                                     ec);
    hop2_it.first->second.bind(hop2_endpoint, hop2_ec);
    hop2_it.first->second.listen(100, hop2_ec);
  }

 protected:
  boost::asio::io_service io_service_;
  TLSAcceptorsMap ssl_circuit_acceptors_;
  AcceptorsMap circuit_acceptors_;
  TLSResolver ssl_resolver_;
  Resolver resolver_;
  boost::thread_group threads_;
  std::unique_ptr<boost::asio::io_service::work> p_work_;
};

#endif  // SSF_TESTS_CIRCUIT_TEST_FIXTURE_H_
