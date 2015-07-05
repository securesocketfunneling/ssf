#include <gtest/gtest.h>

#include "stream_protocol_helpers.h"
#include "circuit_test_fixture.h"
#include "virtual_network_helpers.h"

#include "core/virtual_network/parameters.h"

#include "core/virtual_network/physical_layer/tcp.h"

#include "core/virtual_network/basic_empty_stream.h"
#include "core/virtual_network/cryptography_layer/ssl.h"
#include "core/virtual_network/cryptography_layer/basic_empty_ssl_stream.h"

#include "core/virtual_network/data_link_layer/basic_circuit_protocol.h"
#include "core/virtual_network/data_link_layer/simple_circuit_policy.h"
#include "core/virtual_network/data_link_layer/circuit_helpers.h"

virtual_network::LayerParameters ssl_acceptor_parameters = {
    {"ca_src", "file"},
    {"crt_src", "file"},
    {"key_src", "file"},
    {"dhparam_src", "file"},
    {"ca_file", "./certs/trusted/ca.crt"},
    {"crt_file", "./certs/certificate.crt"},
    {"key_file", "./certs/private.key"},
    {"dhparam_file", "./certs/dh4096.pem"}};

virtual_network::LayerParameters ssl_client_parameters = {
    {"ca_src", "file"},
    {"crt_src", "file"},
    {"key_src", "file"},
    {"ca_file", "./certs/trusted/ca.crt"},
    {"crt_file", "./certs/certificate.crt"},
    {"key_file", "./certs/private.key"}};

virtual_network::LayerParameters tcp_acceptor_parameters = {{"port", "9000"}};

virtual_network::LayerParameters tcp_client_parameters = {{"addr", "127.0.0.1"},
                                                          {"port", "9000"}};

TEST(LinkLayerTest, SimpleCircuitProtocolTest) {
  typedef tests::virtual_network_helpers::Layer<
      virtual_network::physical_layer::tcp> PhysicalProtocol;

  typedef virtual_network::data_link_layer::basic_CircuitProtocol<
      PhysicalProtocol, virtual_network::data_link_layer::CircuitPolicy>
      DataLinkProtocol;

  /// Acceptor endpoint parameters
  virtual_network::ParameterStack acceptor_next_layers_parameters;
  acceptor_next_layers_parameters.push_back(tcp_acceptor_parameters);
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
}

TEST(LinkLayerTest, SSLSimpleCircuitProtocolTest) {
  typedef tests::virtual_network_helpers::SSLLayer<
      virtual_network::physical_layer::tcp> PhysicalProtocol;

  typedef virtual_network::data_link_layer::basic_CircuitProtocol<
      PhysicalProtocol, virtual_network::data_link_layer::CircuitPolicy>
      DataLinkProtocol;

  // Acceptor endpoint parameters
  virtual_network::ParameterStack acceptor_next_layers_parameters;
  acceptor_next_layers_parameters.push_front(tcp_acceptor_parameters);
  acceptor_next_layers_parameters.push_front(ssl_acceptor_parameters);
  virtual_network::ParameterStack acceptor_parameters(
      virtual_network::data_link_layer::make_acceptor_parameter_stack(
          "server", acceptor_next_layers_parameters));

  // Client endpoint parameters
  virtual_network::data_link_layer::NodeParameterList nodes;
  nodes.PushFrontNode();
  nodes.AddTopLayerToFrontNode(tcp_client_parameters);
  nodes.AddTopLayerToFrontNode(ssl_client_parameters);

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
}

TEST_F(CircuitTestFixture, MultipleNodesCircuitTest) {
  typedef CircuitProtocol DataLinkProtocol;

  // Acceptor endpoint parameters
  virtual_network::ParameterStack acceptor_next_layers_parameters;
  acceptor_next_layers_parameters.push_back(tcp_acceptor_parameters);
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
}

TEST_F(CircuitTestFixture, MultipleNodeSSLCircuitTest) {
  typedef SSLCircuitProtocol DataLinkProtocol;

  // Acceptor endpoint parameters
  virtual_network::ParameterStack acceptor_next_layers_parameters;
  acceptor_next_layers_parameters.push_front(tcp_acceptor_parameters);
  acceptor_next_layers_parameters.push_front(ssl_acceptor_parameters);
  virtual_network::ParameterStack acceptor_parameters(
      virtual_network::data_link_layer::make_acceptor_parameter_stack(
          "server", acceptor_next_layers_parameters));

  // Client endpoint parameters
  virtual_network::data_link_layer::NodeParameterList nodes(
      this->GetClientSSLNodes());
  nodes.PushBackNode();
  nodes.AddTopLayerToBackNode(tcp_client_parameters);
  nodes.AddTopLayerToBackNode(ssl_client_parameters);

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
}
