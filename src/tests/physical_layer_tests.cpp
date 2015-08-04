#include <gtest/gtest.h>

#include "tests/datagram_protocol_helpers.h"
#include "tests/stream_protocol_helpers.h"
#include "tests/virtual_network_helpers.h"

#include "core/virtual_network/parameters.h"

#include "core/virtual_network/physical_layer/tcp.h"
#include "core/virtual_network/physical_layer/tlsotcp.h"
#include "core/virtual_network/physical_layer/udp.h"

virtual_network::LayerParameters tcp_server_parameters = {{"port", "9000"}};

virtual_network::LayerParameters tcp_client_parameters = {{"addr", "127.0.0.1"},
                                                          {"port", "9000"}};

TEST(PhysicalLayerTest, TCPBaseLine) {
  virtual_network::ParameterStack acceptor_parameters;
  acceptor_parameters.push_back(tcp_server_parameters);

  virtual_network::ParameterStack client_parameters;
  client_parameters.push_back(tcp_client_parameters);

  TCPPerfTestHalfDuplex(client_parameters, acceptor_parameters, 200);
}

 TEST(PhysicalLayerTest, EmptyStreamProtocolStackOverTCPTest) {
   typedef virtual_network::physical_layer::TCPPhysicalLayer
       StreamStackProtocol;

  virtual_network::ParameterStack acceptor_parameters;
  acceptor_parameters.push_back(tcp_server_parameters);

  virtual_network::ParameterStack client_parameters;
  client_parameters.push_back(tcp_client_parameters);

  virtual_network::LayerParameters client_error_tcp_parameters;
  client_error_tcp_parameters["addr"] = "127.0.0.1";
  client_error_tcp_parameters["port"] = "9001";
  virtual_network::ParameterStack client_error_connection_parameters;
  client_error_connection_parameters.push_back(client_error_tcp_parameters);

  virtual_network::ParameterStack client_wrong_number_parameters;

  TestStreamProtocol<StreamStackProtocol>(client_parameters,
                                          acceptor_parameters, 1024);

  TestStreamProtocolFuture<StreamStackProtocol>(client_parameters,
                                                acceptor_parameters);

  /*  Uncomment after fix on boost build system
  TestStreamProtocolSpawn<StreamStackProtocol>(client_parameters,
                                               acceptor_parameters);*/

  TestStreamProtocolSynchronous<StreamStackProtocol>(client_parameters,
                                                     acceptor_parameters);

  TestStreamErrorConnectionProtocol<StreamStackProtocol>(
      client_error_connection_parameters);

  TestEndpointResolverError<StreamStackProtocol>(
      client_wrong_number_parameters);

  PerfTestStreamProtocolHalfDuplex<StreamStackProtocol>(
      client_parameters, acceptor_parameters, 200);

  PerfTestStreamProtocolFullDuplex<StreamStackProtocol>(
      client_parameters, acceptor_parameters, 200);
}

TEST(PhysicalLayerTest, TLSLayerProtocolStackOverTCPTest) {
  typedef virtual_network::physical_layer::TLSboTCPPhysicalLayer
      TLSStackProtocol;

  virtual_network::ParameterStack acceptor_parameters;
  acceptor_parameters.push_back(
      tests::virtual_network_helpers::tls_server_parameters);
  acceptor_parameters.push_back(tcp_server_parameters);

  virtual_network::ParameterStack client_parameters;
  client_parameters.push_back(
      tests::virtual_network_helpers::tls_client_parameters);
  client_parameters.push_back(tcp_client_parameters);

  virtual_network::LayerParameters client_error_tcp_parameters;
  client_error_tcp_parameters["addr"] = "127.0.0.1";
  client_error_tcp_parameters["port"] = "9001";
  virtual_network::ParameterStack client_error_parameters;
  client_error_parameters.push_back(
      tests::virtual_network_helpers::tls_client_parameters);
  client_error_parameters.push_back(client_error_tcp_parameters);

  virtual_network::ParameterStack client_wrong_number_parameters;
  client_wrong_number_parameters.push_back(tcp_client_parameters);

  TestStreamProtocol<TLSStackProtocol>(client_parameters, acceptor_parameters,
                                       1024);

  TestStreamProtocolFuture<TLSStackProtocol>(client_parameters,
                                             acceptor_parameters);

  /* Uncomment after fix on boost build system
  TestStreamProtocolSpawn<TLSStackProtocol>(client_parameters,
                                            acceptor_parameters);*/

  TestStreamProtocolSynchronous<TLSStackProtocol>(client_parameters,
                                                  acceptor_parameters);

  TestStreamErrorConnectionProtocol<TLSStackProtocol>(client_error_parameters);

  TestEndpointResolverError<TLSStackProtocol>(client_wrong_number_parameters);

  PerfTestStreamProtocolHalfDuplex<TLSStackProtocol>(client_parameters,
                                                     acceptor_parameters, 200);

  PerfTestStreamProtocolFullDuplex<TLSStackProtocol>(client_parameters,
                                                     acceptor_parameters, 200);
}

TEST(PhysicalLayerTest, EmptyDatagramProtocolStackOverUDPTest) {
  typedef virtual_network::physical_layer::UDPPhysicalLayer
      DatagramStackProtocol;

  virtual_network::LayerParameters socket1_udp_parameters;
  socket1_udp_parameters["addr"] = "127.0.0.1";
  socket1_udp_parameters["port"] = "8000";
  virtual_network::ParameterStack socket1_parameters;
  socket1_parameters.push_back(socket1_udp_parameters);

  virtual_network::LayerParameters socket2_udp_parameters;
  socket2_udp_parameters["addr"] = "127.0.0.1";
  socket2_udp_parameters["port"] = "9000";
  virtual_network::ParameterStack socket2_parameters;
  socket2_parameters.push_back(socket2_udp_parameters);

  TestDatagramProtocolPerfHalfDuplex<DatagramStackProtocol>(
    socket1_parameters, socket2_parameters, 100000);

  TestDatagramProtocolPerfFullDuplex<DatagramStackProtocol>(
      socket1_parameters, socket2_parameters, 100000);

  TestConnectionDatagramProtocol<DatagramStackProtocol>(
      socket1_parameters, socket1_parameters, 100);
}
