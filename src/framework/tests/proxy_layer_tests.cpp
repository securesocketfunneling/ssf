#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "tests/stream_protocol_helpers.h"
#include "tests/virtual_network_helpers.h"
#include "tests/proxy_test_fixture.h"

#include "ssf/layer/parameters.h"

#include "ssf/layer/physical/tcp.h"
#include "ssf/layer/cryptography/basic_crypto_stream.h"
#include "ssf/layer/cryptography/tls/OpenSSL/impl.h"
#include "ssf/layer/proxy/basic_proxy_protocol.h"

ssf::layer::LayerParameters tcp_server_parameters = {{"port", "9000"}};

ssf::layer::LayerParameters empty_layer = {};

TEST_F(ProxyTestFixture, ProxySetTest) {
  typedef ssf::layer::proxy::basic_ProxyProtocol<ssf::layer::physical::tcp>
      StreamStackProtocol;

  ASSERT_TRUE(Initialized()) << "Proxy test fixture was not initialized. "
                             << "Please check that the config file '"
                             << config_file_
                             << "' exists (cf. proxy/README.md)";

  ssf::tests::Address client_error_tcp_addr("127.0.0.1", "9001");
  ssf::tests::Address client_error_proxy_addr("192.168.0.0.10", "9000");

  ssf::layer::ParameterStack acceptor_parameters;
  acceptor_parameters.push_back(empty_layer);
  acceptor_parameters.push_back(tcp_server_parameters);

  ssf::layer::ParameterStack client_parameters;
  client_parameters.push_back(GetProxyParam());
  client_parameters.push_back(GetProxyTcpParam());

  ssf::layer::ParameterStack client_error_connection_parameters;
  client_error_connection_parameters.push_back(GetProxyTcpParam());
  client_error_connection_parameters.push_back(
      client_error_tcp_addr.ToTCPParam());

  ssf::layer::ParameterStack client_wrong_number_parameters;
  ssf::layer::ParameterStack client_invalid_proxy_parameters;
  client_invalid_proxy_parameters.push_back(
      client_error_proxy_addr.ToProxyParam());
  client_invalid_proxy_parameters.push_back(GetProxyTcpParam());

  TestStreamErrorConnectionProtocol<StreamStackProtocol>(
      client_error_connection_parameters);

  TestEndpointResolverError<StreamStackProtocol>(
      client_wrong_number_parameters);

  TestEndpointResolverError<StreamStackProtocol>(
      client_invalid_proxy_parameters);

  TestStreamProtocol<StreamStackProtocol>(client_parameters,
                                          acceptor_parameters, 1024);

  TestStreamProtocolFuture<StreamStackProtocol>(client_parameters,
                                                acceptor_parameters);

  /*  Uncomment after fix on boost build system
  TestStreamProtocolSpawn<StreamStackProtocol>(client_parameters,
                                               acceptor_parameters);*/

  TestStreamProtocolSynchronous<StreamStackProtocol>(client_parameters,
                                                     acceptor_parameters);

  PerfTestStreamProtocolHalfDuplex<StreamStackProtocol>(
      client_parameters, acceptor_parameters, 200);

  PerfTestStreamProtocolFullDuplex<StreamStackProtocol>(
      client_parameters, acceptor_parameters, 200);
}

TEST_F(ProxyTestFixture, TLSOverProxyTCPTest) {
  using ProxyTCPProtocol =
      ssf::layer::proxy::basic_ProxyProtocol<ssf::layer::physical::tcp>;
  // using TLSStackProtocol =
  // ssf::layer::cryptography::basic_CryptoStreamProtocol<
  //    ProxyTCPProtocol, ssf::layer::cryptography::tls>;
  using TLSStackProtocol = ssf::layer::cryptography::basic_CryptoStreamProtocol<
      ProxyTCPProtocol, ssf::layer::cryptography::buffered_tls>;

  ASSERT_TRUE(Initialized()) << "Proxy test fixture was not initialized. "
                             << "Please check that the config file '"
                             << config_file_
                             << "' exists (cf. proxy/README.md)";

  ssf::tests::Address client_error_tcp_addr("127.0.0.1", "9001");

  ssf::layer::ParameterStack acceptor_parameters;
  acceptor_parameters.push_back(
      tests::virtual_network_helpers::tls_server_parameters);
  acceptor_parameters.push_back(empty_layer);
  acceptor_parameters.push_back(tcp_server_parameters);

  ssf::layer::ParameterStack client_parameters;
  client_parameters.push_back(
      tests::virtual_network_helpers::tls_client_parameters);
  client_parameters.push_back(GetProxyParam());
  client_parameters.push_back(GetProxyTcpParam());

  ssf::layer::ParameterStack client_error_parameters;
  client_error_parameters.push_back(
      tests::virtual_network_helpers::tls_client_parameters);
  client_error_parameters.push_back(GetProxyTcpParam());
  client_error_parameters.push_back(client_error_tcp_addr.ToTCPParam());

  ssf::layer::ParameterStack client_wrong_number_parameters;
  client_wrong_number_parameters.push_back(empty_layer);
  client_wrong_number_parameters.push_back(GetProxyTcpParam());

  TestEndpointResolverError<TLSStackProtocol>(client_wrong_number_parameters);

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

  PerfTestStreamProtocolHalfDuplex<TLSStackProtocol>(client_parameters,
                                                     acceptor_parameters, 200);

  PerfTestStreamProtocolFullDuplex<TLSStackProtocol>(client_parameters,
                                                     acceptor_parameters, 200);
}

TEST_F(ProxyTestFixture, Socks4ProxySetTest) {
  typedef ssf::layer::proxy::basic_ProxyProtocol<ssf::layer::physical::tcp>
      StreamStackProtocol;

  ASSERT_TRUE(Initialized()) << "Proxy test fixture was not initialized. "
                             << "Please check that the config file '"
                             << config_file_
                             << "' exists (cf. proxy/README.md)";

  ssf::layer::ParameterStack acceptor_parameters;
  acceptor_parameters.push_back(empty_layer);
  acceptor_parameters.push_back(tcp_server_parameters);

  auto socks_proxy_param = GetSocksProxyParam();
  socks_proxy_param["socks_version"] = "4";

  ssf::layer::ParameterStack client_parameters;
  client_parameters.push_back(socks_proxy_param);
  client_parameters.push_back(GetProxyTcpParam());

  TestStreamProtocol<StreamStackProtocol>(client_parameters,
                                          acceptor_parameters, 1024);

  TestStreamProtocolFuture<StreamStackProtocol>(client_parameters,
                                                acceptor_parameters);

  /*  Uncomment after fix on boost build system
  TestStreamProtocolSpawn<StreamStackProtocol>(client_parameters,
                                               acceptor_parameters);*/

  TestStreamProtocolSynchronous<StreamStackProtocol>(client_parameters,
                                                     acceptor_parameters);

  PerfTestStreamProtocolHalfDuplex<StreamStackProtocol>(
      client_parameters, acceptor_parameters, 200);

  PerfTestStreamProtocolFullDuplex<StreamStackProtocol>(
      client_parameters, acceptor_parameters, 200);
}

TEST_F(ProxyTestFixture, Socks5ProxySetTest) {
  typedef ssf::layer::proxy::basic_ProxyProtocol<ssf::layer::physical::tcp>
      StreamStackProtocol;

  ASSERT_TRUE(Initialized()) << "Proxy test fixture was not initialized. "
                             << "Please check that the config file '"
                             << config_file_
                             << "' exists (cf. proxy/README.md)";

  ssf::layer::ParameterStack acceptor_parameters;
  acceptor_parameters.push_back(empty_layer);
  acceptor_parameters.push_back(tcp_server_parameters);

  auto socks_proxy_param = GetSocksProxyParam();
  socks_proxy_param["socks_version"] = "5";

  ssf::layer::ParameterStack client_parameters;
  client_parameters.push_back(socks_proxy_param);
  client_parameters.push_back(GetProxyTcpParam());

  TestStreamProtocol<StreamStackProtocol>(client_parameters,
                                          acceptor_parameters, 1024);

  TestStreamProtocolFuture<StreamStackProtocol>(client_parameters,
                                                acceptor_parameters);

  /*  Uncomment after fix on boost build system
  TestStreamProtocolSpawn<StreamStackProtocol>(client_parameters,
                                               acceptor_parameters);*/

  TestStreamProtocolSynchronous<StreamStackProtocol>(client_parameters,
                                                     acceptor_parameters);

  PerfTestStreamProtocolHalfDuplex<StreamStackProtocol>(
      client_parameters, acceptor_parameters, 200);

  PerfTestStreamProtocolFullDuplex<StreamStackProtocol>(
      client_parameters, acceptor_parameters, 200);
}

TEST_F(ProxyTestFixture, ProxyNotSetTest) {
  typedef ssf::layer::proxy::basic_ProxyProtocol<
      ssf::layer::physical::TCPPhysicalLayer> StreamStackProtocol;

  ASSERT_TRUE(Initialized()) << "Proxy test fixture was not initialized. "
                             << "Please check that the config file '"
                             << config_file_
                             << "' exists (cf. proxy/README.md)";

  ssf::tests::Address client_error_tcp_addr("127.0.0.1", "9001");

  ssf::layer::LayerParameters empty_layer = {};
  ssf::layer::ParameterStack acceptor_parameters;
  acceptor_parameters.push_back(empty_layer);
  acceptor_parameters.push_back(tcp_server_parameters);

  ssf::layer::ParameterStack client_parameters;
  client_parameters.push_back(empty_layer);
  client_parameters.push_back(GetLocalTcpParam());

  ssf::layer::ParameterStack client_error_connection_parameters;
  client_error_connection_parameters.push_back(empty_layer);
  client_error_connection_parameters.push_back(
      client_error_tcp_addr.ToTCPParam());

  ssf::layer::ParameterStack client_wrong_number_parameters;

  TestEndpointResolverError<StreamStackProtocol>(
      client_wrong_number_parameters);

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

  PerfTestStreamProtocolHalfDuplex<StreamStackProtocol>(
      client_parameters, acceptor_parameters, 200);

  PerfTestStreamProtocolFullDuplex<StreamStackProtocol>(
      client_parameters, acceptor_parameters, 200);
}
