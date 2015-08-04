#include <gtest/gtest.h>

#include "core/virtual_network/basic_empty_stream.h"
#include "core/virtual_network/cryptography_layer/tls/OpenSSL/impl.h"
#include "core/virtual_network/cryptography_layer/basic_crypto_stream.h"

#include "core/virtual_network/data_link_layer/basic_circuit_protocol.h"
#include "core/virtual_network/data_link_layer/simple_circuit_policy.h"
#include "core/virtual_network/data_link_layer/circuit_helpers.h"

#include "core/virtual_network/parameters.h"

#include "core/virtual_network/physical_layer/tcp.h"

#include "tests/circuit_test_fixture.h"
#include "tests/stream_protocol_helpers.h"
#include "tests/virtual_network_helpers.h"

virtual_network::LayerParameters tcp_server_parameters = {{"port", "9000"}};

virtual_network::LayerParameters tcp_client_parameters = {{"addr", "127.0.0.1"},
                                                          {"port", "9000"}};

TEST(LinkLayerTest, TCPBaseLine) {
  virtual_network::LayerParameters acceptor_tcp_parameters;
  acceptor_tcp_parameters["port"] = "9000";
  virtual_network::ParameterStack acceptor_parameters;
  acceptor_parameters.push_back(acceptor_tcp_parameters);

  virtual_network::LayerParameters client_tcp_parameters;
  client_tcp_parameters["addr"] = "127.0.0.1";
  client_tcp_parameters["port"] = "9000";
  virtual_network::ParameterStack client_parameters;
  client_parameters.push_back(client_tcp_parameters);

  TCPPerfTestHalfDuplex(client_parameters, acceptor_parameters, 200);
}

TEST(LinkLayerTest, SimpleCircuitProtocolTest) {
  typedef tests::virtual_network_helpers::Layer<
      virtual_network::physical_layer::tcp> PhysicalProtocol;

  typedef virtual_network::data_link_layer::basic_CircuitProtocol<
      PhysicalProtocol, virtual_network::data_link_layer::CircuitPolicy>
      DataLinkProtocol;

  /// Acceptor endpoint parameters
  virtual_network::ParameterStack acceptor_next_layers_parameters;
  acceptor_next_layers_parameters.push_back(tcp_server_parameters);
  virtual_network::ParameterStack acceptor_parameters(
      virtual_network::data_link_layer::make_acceptor_parameter_stack(
          "server", acceptor_next_layers_parameters));

  /// Client endpoint parameters
  virtual_network::data_link_layer::NodeParameterList nodes;
  nodes.PushFrontNode();
  nodes.AddTopLayerToFrontNode(tcp_client_parameters);

  virtual_network::ParameterStack client_parameters(
      virtual_network::data_link_layer::
          make_client_full_circuit_parameter_stack("server", nodes));

  TestStreamProtocol<DataLinkProtocol>(client_parameters, acceptor_parameters,
                                       100 * 10);

  /* Uncomment after fix on boost build system
  TestStreamProtocolSpawn<DataLinkProtocol>(client_parameters,
                                            acceptor_parameters);*/

  TestStreamProtocolFuture<DataLinkProtocol>(client_parameters,
                                             acceptor_parameters);

  PerfTestStreamProtocolHalfDuplex<DataLinkProtocol>(client_parameters,
                                                     acceptor_parameters, 200);

  PerfTestStreamProtocolFullDuplex<DataLinkProtocol>(client_parameters,
                                                     acceptor_parameters, 200);
}

TEST(LinkLayerTest, SimpleTLSCircuitProtocolTest) {
  typedef tests::virtual_network_helpers::TLSLayer<
      virtual_network::physical_layer::tcp> PhysicalProtocol;

  typedef virtual_network::data_link_layer::basic_CircuitProtocol<
      PhysicalProtocol, virtual_network::data_link_layer::CircuitPolicy>
      DataLinkProtocol;

  // Acceptor endpoint parameters
  virtual_network::ParameterStack acceptor_next_layers_parameters;
  acceptor_next_layers_parameters.push_front(tcp_server_parameters);
  acceptor_next_layers_parameters.push_front(
      tests::virtual_network_helpers::tls_server_parameters);
  virtual_network::ParameterStack acceptor_parameters(
      virtual_network::data_link_layer::make_acceptor_parameter_stack(
          "server", acceptor_next_layers_parameters));

  // Client endpoint parameters
  virtual_network::data_link_layer::NodeParameterList nodes;
  nodes.PushFrontNode();
  nodes.AddTopLayerToFrontNode(tcp_client_parameters);
  nodes.AddTopLayerToFrontNode(
      tests::virtual_network_helpers::tls_server_parameters);

  virtual_network::ParameterStack client_parameters(
      virtual_network::data_link_layer::
          make_client_full_circuit_parameter_stack("server", nodes));

  TestStreamProtocol<DataLinkProtocol>(client_parameters, acceptor_parameters,
                                       100 * 10);

  /* Uncomment after fix on boost build system
  TestStreamProtocolSpawn<DataLinkProtocol>(client_parameters,
                                            acceptor_parameters);*/

  TestStreamProtocolFuture<DataLinkProtocol>(client_parameters,
                                             acceptor_parameters);

  PerfTestStreamProtocolHalfDuplex<DataLinkProtocol>(client_parameters,
                                                     acceptor_parameters, 200);

  PerfTestStreamProtocolFullDuplex<DataLinkProtocol>(client_parameters,
                                                     acceptor_parameters, 200);
}

TEST_F(CircuitTestFixture, CircuitTest) {
  typedef CircuitProtocol DataLinkProtocol;

  // Acceptor endpoint parameters
  virtual_network::ParameterStack acceptor_next_layers_parameters;
  acceptor_next_layers_parameters.push_back(tcp_server_parameters);
  virtual_network::ParameterStack acceptor_parameters(
      virtual_network::data_link_layer::make_acceptor_parameter_stack(
          "server", acceptor_next_layers_parameters));

  // Client endpoint parameters
  virtual_network::data_link_layer::NodeParameterList nodes(
      this->GetClientNodes());
  nodes.PushBackNode();
  nodes.AddTopLayerToBackNode(tcp_client_parameters);

  virtual_network::ParameterStack client_parameters(
      virtual_network::data_link_layer::
          make_client_full_circuit_parameter_stack("server", nodes));

  TestStreamProtocol<DataLinkProtocol>(client_parameters, acceptor_parameters,
                                       100 * 10);

  /* Uncomment after fix on boost build system
  TestStreamProtocolSpawn<DataLinkProtocol>(client_parameters,
                                            acceptor_parameters);*/

  TestStreamProtocolFuture<DataLinkProtocol>(client_parameters,
                                             acceptor_parameters);

  PerfTestStreamProtocolHalfDuplex<DataLinkProtocol>(client_parameters,
                                                     acceptor_parameters, 200);

  PerfTestStreamProtocolFullDuplex<DataLinkProtocol>(client_parameters,
                                                     acceptor_parameters, 200);
}

TEST_F(CircuitTestFixture, CircuitTLSTest) {
  typedef TLSCircuitProtocol DataLinkProtocol;

  // Acceptor endpoint parameters
  virtual_network::ParameterStack acceptor_next_layers_parameters;
  acceptor_next_layers_parameters.push_front(tcp_server_parameters);
  acceptor_next_layers_parameters.push_front(
      tests::virtual_network_helpers::tls_server_parameters);
  virtual_network::ParameterStack acceptor_parameters(
      virtual_network::data_link_layer::make_acceptor_parameter_stack(
          "server", acceptor_next_layers_parameters));

  // Client endpoint parameters
  virtual_network::data_link_layer::NodeParameterList nodes(
      this->GetClientTLSNodes());
  nodes.PushBackNode();
  nodes.AddTopLayerToBackNode(tcp_client_parameters);
  nodes.AddTopLayerToBackNode(
      tests::virtual_network_helpers::tls_server_parameters);

  virtual_network::ParameterStack client_parameters(
      virtual_network::data_link_layer::
          make_client_full_circuit_parameter_stack("server", nodes));

  TestStreamProtocol<DataLinkProtocol>(client_parameters, acceptor_parameters,
                                       100 * 10);

  /* Uncomment after fix on boost build system
  TestStreamProtocolSpawn<DataLinkProtocol>(client_parameters,
                                            acceptor_parameters);*/

  TestStreamProtocolFuture<DataLinkProtocol>(client_parameters,
                                             acceptor_parameters);

  PerfTestStreamProtocolHalfDuplex<DataLinkProtocol>(client_parameters,
                                                     acceptor_parameters, 200);

  PerfTestStreamProtocolFullDuplex<DataLinkProtocol>(client_parameters,
                                                     acceptor_parameters, 200);
}
