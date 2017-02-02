#include <gtest/gtest.h>

#include "ssf/layer/basic_empty_stream.h"
#include "ssf/layer/cryptography/tls/OpenSSL/impl.h"
#include "ssf/layer/cryptography/basic_crypto_stream.h"

#include "ssf/layer/data_link/basic_circuit_protocol.h"
#include "ssf/layer/data_link/simple_circuit_policy.h"
#include "ssf/layer/data_link/circuit_helpers.h"

#include "ssf/layer/parameters.h"

#include "ssf/layer/physical/tcp.h"

#include "tests/circuit_test_fixture.h"
#include "tests/stream_protocol_helpers.h"
#include "tests/virtual_network_helpers.h"

ssf::layer::LayerParameters tcp_server_parameters = {{"port", "9000"}};

ssf::layer::LayerParameters tcp_client_parameters = {{"addr", "127.0.0.1"},
                                                          {"port", "9000"}};

TEST(LinkLayerTest, TCPBaseLine) {
  ssf::layer::LayerParameters acceptor_tcp_parameters;
  acceptor_tcp_parameters["port"] = "9000";
  ssf::layer::ParameterStack acceptor_parameters;
  acceptor_parameters.push_back(acceptor_tcp_parameters);

  ssf::layer::LayerParameters client_tcp_parameters;
  client_tcp_parameters["addr"] = "127.0.0.1";
  client_tcp_parameters["port"] = "9000";
  ssf::layer::ParameterStack client_parameters;
  client_parameters.push_back(client_tcp_parameters);

  TCPPerfTestHalfDuplex(client_parameters, acceptor_parameters, 200);
}

TEST(LinkLayerTest, SimpleCircuitProtocolTest) {
  using PhysicalProtocol = ssf::layer::physical::TCPPhysicalLayer;
  using DataLinkProtocol = ssf::layer::data_link::basic_CircuitProtocol<
      PhysicalProtocol, ssf::layer::data_link::CircuitPolicy>;

  /// Acceptor endpoint parameters
  ssf::layer::ParameterStack acceptor_default_parameters = {{}, {}};
  ssf::layer::ParameterStack acceptor_next_layers_parameters;
  acceptor_next_layers_parameters.push_back(tcp_server_parameters);
  ssf::layer::ParameterStack acceptor_parameters(
      ssf::layer::data_link::make_acceptor_parameter_stack(
          "server", acceptor_default_parameters,
          acceptor_next_layers_parameters));

  /// Client endpoint parameters
  ssf::layer::data_link::NodeParameterList nodes;
  nodes.PushFrontNode();
  nodes.AddTopLayerToFrontNode(tcp_client_parameters);

  ssf::layer::ParameterStack client_parameters(
      ssf::layer::data_link::
          make_client_full_circuit_parameter_stack("server", nodes));

  TestStreamProtocol<DataLinkProtocol>(client_parameters, acceptor_parameters,
                                       100 * 10);

  /* Uncomment after fix on boost build system
  TestStreamProtocolSpawn<DataLinkProtocol>(client_parameters,
                                            acceptor_parameters);*/

  TestStreamProtocolFuture<DataLinkProtocol>(client_parameters,
                                             acceptor_parameters);

  /* Not implemented yet
  TestStreamProtocolSynchronous<DataLinkProtocol>(client_parameters,
                                                  acceptor_parameters);*/

  PerfTestStreamProtocolHalfDuplex<DataLinkProtocol>(client_parameters,
                                                     acceptor_parameters, 200);

  PerfTestStreamProtocolFullDuplex<DataLinkProtocol>(client_parameters,
                                                     acceptor_parameters, 200);
}

TEST(LinkLayerTest, SimpleTLSCircuitProtocolTest) {
  using TLSPhysicalProtocol = ssf::layer::physical::TLSboTCPPhysicalLayer;
  using DataLinkProtocol = ssf::layer::data_link::basic_CircuitProtocol<
      TLSPhysicalProtocol, ssf::layer::data_link::CircuitPolicy>;

  // Acceptor endpoint parameters
  ssf::layer::ParameterStack acceptor_default_parameters = {{}, {}, {}};
  ssf::layer::ParameterStack acceptor_next_layers_parameters;
  acceptor_next_layers_parameters.push_front(tcp_server_parameters);
  acceptor_next_layers_parameters.push_front(
      tests::virtual_network_helpers::GetServerTLSParametersAsFile());
  ssf::layer::ParameterStack acceptor_parameters(
      ssf::layer::data_link::make_acceptor_parameter_stack(
          "server", acceptor_default_parameters,
          acceptor_next_layers_parameters));

  // Client endpoint parameters
  ssf::layer::data_link::NodeParameterList nodes;
  nodes.PushFrontNode();
  nodes.AddTopLayerToFrontNode(tcp_client_parameters);
  nodes.AddTopLayerToFrontNode(
      tests::virtual_network_helpers::GetServerTLSParametersAsFile());

  ssf::layer::ParameterStack client_parameters(
      ssf::layer::data_link::
          make_client_full_circuit_parameter_stack("server", nodes));

  TestStreamProtocol<DataLinkProtocol>(client_parameters, acceptor_parameters,
                                       100 * 10);

  /* Uncomment after fix on boost build system
  TestStreamProtocolSpawn<DataLinkProtocol>(client_parameters,
                                            acceptor_parameters);*/

  TestStreamProtocolFuture<DataLinkProtocol>(client_parameters,
                                             acceptor_parameters);

  /* Not implemented yet
  TestStreamProtocolSynchronous<DataLinkProtocol>(client_parameters,
                                                  acceptor_parameters);*/

  PerfTestStreamProtocolHalfDuplex<DataLinkProtocol>(client_parameters,
                                                     acceptor_parameters, 200);

  PerfTestStreamProtocolFullDuplex<DataLinkProtocol>(client_parameters,
                                                     acceptor_parameters, 200);
}

TEST_F(CircuitTestFixture, CircuitTest) {
  using DataLinkProtocol = CircuitProtocol;

  // Acceptor endpoint parameters
  ssf::layer::ParameterStack acceptor_default_parameters = {{}, {}};
  ssf::layer::ParameterStack acceptor_next_layers_parameters;
  acceptor_next_layers_parameters.push_back(tcp_server_parameters);
  ssf::layer::ParameterStack acceptor_parameters(
      ssf::layer::data_link::make_acceptor_parameter_stack(
          "server", acceptor_default_parameters,
          acceptor_next_layers_parameters));

  // Client endpoint parameters
  ssf::layer::data_link::NodeParameterList nodes(
      this->GetClientNodes());
  nodes.PushBackNode();
  nodes.AddTopLayerToBackNode(tcp_client_parameters);

  ssf::layer::ParameterStack client_parameters(
      ssf::layer::data_link::
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
  using DataLinkProtocol = TLSCircuitProtocol;

  // Acceptor endpoint parameters
  ssf::layer::ParameterStack acceptor_default_parameters = {{}, {}, {}};
  ssf::layer::ParameterStack acceptor_next_layers_parameters;
  acceptor_next_layers_parameters.push_front(tcp_server_parameters);
  acceptor_next_layers_parameters.push_front(
      tests::virtual_network_helpers::GetServerTLSParametersAsFile());
  ssf::layer::ParameterStack acceptor_parameters(
      ssf::layer::data_link::make_acceptor_parameter_stack(
          "server", acceptor_default_parameters,
          acceptor_next_layers_parameters));

  // Client endpoint parameters
  ssf::layer::data_link::NodeParameterList nodes(
      this->GetClientTLSNodes());
  nodes.PushBackNode();
  nodes.AddTopLayerToBackNode(tcp_client_parameters);
  nodes.AddTopLayerToBackNode(
      tests::virtual_network_helpers::GetServerTLSParametersAsFile());

  ssf::layer::ParameterStack client_parameters(
      ssf::layer::data_link::
          make_client_full_circuit_parameter_stack("server", nodes));

  TestStreamProtocol<DataLinkProtocol>(client_parameters, acceptor_parameters,
                                       100 * 10);

  /* Uncomment after fix on boost build system
  TestStreamProtocolSpawn<DataLinkProtocol>(client_parameters,
                                            acceptor_parameters);*/

  TestStreamProtocolFuture<DataLinkProtocol>(client_parameters,
                                             acceptor_parameters);

  /* Not implemented yet
  TestStreamProtocolSynchronous<DataLinkProtocol>(client_parameters,
                                                  acceptor_parameters);*/

  PerfTestStreamProtocolHalfDuplex<DataLinkProtocol>(client_parameters,
                                                     acceptor_parameters, 200);

  PerfTestStreamProtocolFullDuplex<DataLinkProtocol>(client_parameters,
                                                     acceptor_parameters, 200);
}

TEST_F(CircuitTestFixture, CircuitDefaultTLSTest) {
  using DataLinkProtocol = TLSCircuitProtocol;

  // Acceptor endpoint parameters
  ssf::layer::ParameterStack acceptor_default_parameters = {{}, {}, {}};
  ssf::layer::ParameterStack acceptor_next_layers_parameters;
  acceptor_next_layers_parameters.push_front(tcp_server_parameters);
  acceptor_next_layers_parameters.push_front(
      tests::virtual_network_helpers::GetServerTLSParametersAsFile());

  ssf::layer::ParameterStack acceptor_parameters(
      ssf::layer::data_link::make_acceptor_parameter_stack(
          "server", acceptor_default_parameters,
          acceptor_next_layers_parameters));

  // Client endpoint parameters
  ssf::layer::data_link::NodeParameterList nodes(
      this->GetClientDefaultTLSNodes());
  nodes.PushBackNode();
  nodes.AddTopLayerToBackNode(tcp_client_parameters);
  nodes.AddTopLayerToBackNode(
      tests::virtual_network_helpers::GetDefaultServerParameters());

  ssf::layer::ParameterStack client_parameters(
      ssf::layer::data_link::
          make_client_full_circuit_parameter_stack("server", nodes));

  TestStreamProtocol<DataLinkProtocol>(client_parameters, acceptor_parameters,
                                       100 * 10);

  /* Uncomment after fix on boost build system
  TestStreamProtocolSpawn<DataLinkProtocol>(client_parameters,
                                            acceptor_parameters);*/

  TestStreamProtocolFuture<DataLinkProtocol>(client_parameters,
                                             acceptor_parameters);

  /* Not implemented yet
  TestStreamProtocolSynchronous<DataLinkProtocol>(client_parameters,
                                                  acceptor_parameters);*/

  PerfTestStreamProtocolHalfDuplex<DataLinkProtocol>(client_parameters,
                                                     acceptor_parameters, 200);

  PerfTestStreamProtocolFullDuplex<DataLinkProtocol>(client_parameters,
                                                     acceptor_parameters, 200);
}
