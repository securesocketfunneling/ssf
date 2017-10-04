#include <gtest/gtest.h>

#include "tests/datagram_protocol_helpers.h"
#include "tests/stream_protocol_helpers.h"
#include "tests/virtual_network_helpers.h"

#include "ssf/layer/parameters.h"

#include "ssf/layer/physical/tcp.h"
#include "ssf/layer/physical/tlsotcp.h"
#include "ssf/layer/physical/udp.h"

ssf::layer::LayerParameters tcp_server_parameters = {{"port", "9000"}};

ssf::layer::LayerParameters tcp_client_parameters = {{"addr", "127.0.0.1"},
                                                     {"port", "9000"}};

TEST(PhysicalLayerTest, TCPBaseLine) {
  ssf::layer::ParameterStack acceptor_parameters;
  acceptor_parameters.push_back(tcp_server_parameters);

  ssf::layer::ParameterStack client_parameters;
  client_parameters.push_back(tcp_client_parameters);

  TCPPerfTestHalfDuplex(client_parameters, acceptor_parameters, 200);
}

TEST(PhysicalLayerTest, EmptyStreamProtocolStackOverTCPTest) {
  typedef ssf::layer::physical::TCPPhysicalLayer StreamStackProtocol;

  ssf::layer::ParameterStack acceptor_parameters;
  acceptor_parameters.push_back(tcp_server_parameters);

  ssf::layer::ParameterStack client_parameters;
  client_parameters.push_back(tcp_client_parameters);

  ssf::layer::LayerParameters client_error_tcp_parameters;
  client_error_tcp_parameters["addr"] = "127.0.0.1";
  client_error_tcp_parameters["port"] = "9001";
  ssf::layer::ParameterStack client_error_connection_parameters;
  client_error_connection_parameters.push_back(client_error_tcp_parameters);

  ssf::layer::ParameterStack client_wrong_number_parameters;

  TestStreamProtocol<StreamStackProtocol>(client_parameters,
                                          acceptor_parameters, 1024);

  TestMultiConnectionsProtocol<StreamStackProtocol>(client_parameters,
                                                    acceptor_parameters);

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

//TEST(PhysicalLayerTest, TLSLayerProtocolStackOverTCPTest) {
//  using TLSStackProtocol = ssf::layer::physical::TLSoTCPPhysicalLayer;
//
//  ssf::layer::ParameterStack acceptor_parameters;
//  acceptor_parameters.push_back(
//      tests::virtual_network_helpers::GetServerTLSParametersAsFile());
//  acceptor_parameters.push_back(tcp_server_parameters);
//
//  ssf::layer::ParameterStack client_parameters;
//  client_parameters.push_back(
//      tests::virtual_network_helpers::GetClientTLSParametersAsBuffer());
//  client_parameters.push_back(tcp_client_parameters);
//
//  ssf::layer::LayerParameters client_error_tcp_parameters;
//  client_error_tcp_parameters["addr"] = "127.0.0.1";
//  client_error_tcp_parameters["port"] = "9001";
//  ssf::layer::ParameterStack client_error_parameters;
//  client_error_parameters.push_back(
//      tests::virtual_network_helpers::GetClientTLSParametersAsFile());
//  client_error_parameters.push_back(client_error_tcp_parameters);
//
//  ssf::layer::ParameterStack client_wrong_number_parameters;
//  client_wrong_number_parameters.push_back(tcp_client_parameters);
//
//  TestStreamProtocol<TLSStackProtocol>(client_parameters, acceptor_parameters,
//                                       1024);
//
//  TestMultiConnectionsProtocol<TLSStackProtocol>(client_parameters,
//                                                 acceptor_parameters);
//
//  TestStreamProtocolFuture<TLSStackProtocol>(client_parameters,
//                                             acceptor_parameters);
//
//  /* Uncomment after fix on boost build system
//  TestStreamProtocolSpawn<TLSStackProtocol>(client_parameters,
//                                            acceptor_parameters);*/
//
//  TestStreamProtocolSynchronous<TLSStackProtocol>(client_parameters,
//                                                  acceptor_parameters);
//
//  TestStreamErrorConnectionProtocol<TLSStackProtocol>(client_error_parameters);
//
//  TestEndpointResolverError<TLSStackProtocol>(client_wrong_number_parameters);
//
//  PerfTestStreamProtocolHalfDuplex<TLSStackProtocol>(client_parameters,
//                                                     acceptor_parameters, 200);
//
//  PerfTestStreamProtocolFullDuplex<TLSStackProtocol>(client_parameters,
//                                                     acceptor_parameters, 200);
//}

TEST(PhysicalLayerTest, TLSLayerProtocolBufferStackOverTCPTest) {
  using TLSBufferedStackProtocol = ssf::layer::physical::TLSboTCPPhysicalLayer;

  ssf::layer::ParameterStack acceptor_parameters;
  acceptor_parameters.push_back(
      tests::virtual_network_helpers::GetServerTLSParametersAsBuffer());
  acceptor_parameters.push_back(tcp_server_parameters);

  ssf::layer::ParameterStack client_parameters;
  client_parameters.push_back(
      tests::virtual_network_helpers::GetClientTLSParametersAsFile());
  client_parameters.push_back(tcp_client_parameters);

  ssf::layer::LayerParameters client_error_tcp_parameters;
  client_error_tcp_parameters["addr"] = "127.0.0.1";
  client_error_tcp_parameters["port"] = "9001";
  ssf::layer::ParameterStack client_error_parameters;
  client_error_parameters.push_back(
      tests::virtual_network_helpers::GetClientTLSParametersAsFile());
  client_error_parameters.push_back(client_error_tcp_parameters);

  ssf::layer::ParameterStack client_wrong_number_parameters;
  client_wrong_number_parameters.push_back(tcp_client_parameters);

  TestStreamProtocol<TLSBufferedStackProtocol>(client_parameters,
                                               acceptor_parameters, 1024);

  TestMultiConnectionsProtocol<TLSBufferedStackProtocol>(client_parameters,
                                                         acceptor_parameters);

  TestStreamProtocolFuture<TLSBufferedStackProtocol>(client_parameters,
                                                     acceptor_parameters);

  /* Uncomment after fix on boost build system
  TestStreamProtocolSpawn<TLSStackProtocol>(client_parameters,
                                            acceptor_parameters);*/

  TestStreamProtocolSynchronous<TLSBufferedStackProtocol>(client_parameters,
                                                          acceptor_parameters);

  TestStreamErrorConnectionProtocol<TLSBufferedStackProtocol>(
      client_error_parameters);

  TestEndpointResolverError<TLSBufferedStackProtocol>(
      client_wrong_number_parameters);

  PerfTestStreamProtocolHalfDuplex<TLSBufferedStackProtocol>(
      client_parameters, acceptor_parameters, 200);

  PerfTestStreamProtocolFullDuplex<TLSBufferedStackProtocol>(
      client_parameters, acceptor_parameters, 200);
}

TEST(PhysicalLayerTest, EmptyDatagramProtocolStackOverUDPTest) {
  typedef ssf::layer::physical::UDPPhysicalLayer DatagramStackProtocol;

  ssf::layer::LayerParameters socket1_udp_parameters;
  socket1_udp_parameters["addr"] = "127.0.0.1";
  socket1_udp_parameters["port"] = "8000";
  ssf::layer::ParameterStack socket1_parameters;
  socket1_parameters.push_back(socket1_udp_parameters);

  ssf::layer::LayerParameters socket2_udp_parameters;
  socket2_udp_parameters["addr"] = "127.0.0.1";
  socket2_udp_parameters["port"] = "9000";
  ssf::layer::ParameterStack socket2_parameters;
  socket2_parameters.push_back(socket2_udp_parameters);

  TestDatagramProtocolPerfHalfDuplex<DatagramStackProtocol>(
      socket1_parameters, socket2_parameters, 100000);

  TestDatagramProtocolPerfFullDuplex<DatagramStackProtocol>(
      socket1_parameters, socket2_parameters, 100000);

  TestConnectionDatagramProtocol<DatagramStackProtocol>(
      socket1_parameters, socket1_parameters, 100);
}