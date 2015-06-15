#include "stream_protocol_helpers.h"
#include "datagram_protocol_helpers.h"

#include <boost/system/error_code.hpp>

#include "virtual_network_helpers.h"

#include "core/virtual_network/physical_layer/tcp.h"
#include "core/virtual_network/physical_layer/udp.h"

#include "core/virtual_network/cryptography_layer/ssl.h"

#include "core/virtual_network/basic_empty_stream.h"
#include "core/virtual_network/basic_empty_datagram.h"
#include "core/virtual_network/cryptography_layer/basic_empty_ssl_stream.h"

tests::virtual_network_helpers::Parameters ssl_server_parameters = {
    {"ca_src", "file"},
    {"crt_src", "file"},
    {"key_src", "file"},
    {"dhparam_src", "file"},
    {"ca_file", "./certs/trusted/ca.crt"},
    {"crt_file", "./certs/certificate.crt"},
    {"key_file", "./certs/private.key"},
    {"dhparam_file", "./certs/dh4096.pem"}};

tests::virtual_network_helpers::Parameters ssl_client_parameters = {
    {"ca_src", "file"},
    {"crt_src", "file"},
    {"key_src", "file"},
    {"ca_file", "./certs/trusted/ca.crt"},
    {"crt_file", "./certs/certificate.crt"},
    {"key_file", "./certs/private.key"}};

template <class NextLayer>
using Layer = virtual_network::VirtualEmptyStreamProtocol<NextLayer>;

template <class NextLayer>
using DatagramLayer = virtual_network::VirtualEmptyDatagramProtocol<NextLayer>;

template <class NextLayer>
using SSLLayer =
    virtual_network::cryptography_layer::VirtualEmptySSLStreamProtocol<
        virtual_network::cryptography_layer::buffered_ssl<NextLayer>>;

TEST(PhysicalLayerTest, EmptyStreamProtocolStackOverTCPTest) {
  typedef Layer<Layer<Layer<Layer<virtual_network::physical_layer::tcp>>>>
      StreamStackProtocol;

  tests::virtual_network_helpers::Parameters acceptor_tcp_parameters;
  acceptor_tcp_parameters["port"] = "9000";
  tests::virtual_network_helpers::ParametersList acceptor_parameters;
  acceptor_parameters.push_back(acceptor_tcp_parameters);

  tests::virtual_network_helpers::Parameters client_tcp_parameters;
  client_tcp_parameters["addr"] = "127.0.0.1";
  client_tcp_parameters["port"] = "9000";
  tests::virtual_network_helpers::ParametersList client_parameters;
  client_parameters.push_back(client_tcp_parameters);

  TestStreamProtocol<StreamStackProtocol>(std::move(client_parameters),
                                          std::move(acceptor_parameters), 100);

  tests::virtual_network_helpers::Parameters client_error_tcp_parameters;
  client_error_tcp_parameters["addr"] = "127.0.0.1";
  client_error_tcp_parameters["port"] = "9001";
  tests::virtual_network_helpers::ParametersList
      client_error_connection_parameters;
  client_error_connection_parameters.push_back(client_error_tcp_parameters);

  TestStreamErrorConnectionProtocol<StreamStackProtocol>(
      std::move(client_error_connection_parameters));
}

TEST(PhysicalLayerTest, SSLLayerProtocolStackOverTCPTest) {
  typedef SSLLayer<SSLLayer<Layer<Layer<virtual_network::physical_layer::tcp>>>>
      SSLStackProtocol;

  tests::virtual_network_helpers::Parameters acceptor_ssl_parameters =
      ssl_server_parameters;
  tests::virtual_network_helpers::Parameters acceptor_tcp_parameters;
  acceptor_tcp_parameters["port"] = "9000";
  tests::virtual_network_helpers::ParametersList acceptor_parameters;
  acceptor_parameters.push_back(acceptor_ssl_parameters);
  acceptor_parameters.push_back(acceptor_ssl_parameters);
  acceptor_parameters.push_back(acceptor_tcp_parameters);

  tests::virtual_network_helpers::Parameters client_ssl_parameters =
      ssl_client_parameters;
  tests::virtual_network_helpers::Parameters client_tcp_parameters;
  client_tcp_parameters["addr"] = "127.0.0.1";
  client_tcp_parameters["port"] = "9000";
  tests::virtual_network_helpers::ParametersList client_parameters;
  client_parameters.push_back(ssl_client_parameters);
  client_parameters.push_back(ssl_client_parameters);
  client_parameters.push_back(client_tcp_parameters);

  TestStreamProtocol<SSLStackProtocol>(std::move(client_parameters),
                                       std::move(acceptor_parameters), 100);

  tests::virtual_network_helpers::Parameters client_error_tcp_parameters;
  client_error_tcp_parameters["addr"] = "127.0.0.1";
  client_error_tcp_parameters["port"] = "9001";
  tests::virtual_network_helpers::ParametersList client_error_parameters;
  client_error_parameters.push_back(ssl_client_parameters);
  client_error_parameters.push_back(ssl_client_parameters);
  client_error_parameters.push_back(client_error_tcp_parameters);

  TestStreamErrorConnectionProtocol<SSLStackProtocol>(
      std::move(client_error_parameters));

  tests::virtual_network_helpers::Parameters client_correct_tcp_parameters;
  client_correct_tcp_parameters["addr"] = "127.0.0.1";
  client_correct_tcp_parameters["port"] = "9000";
  tests::virtual_network_helpers::ParametersList client_wrong_number_parameters;
  client_wrong_number_parameters.push_back(ssl_client_parameters);
  client_wrong_number_parameters.push_back(client_correct_tcp_parameters);
  TestEndpointResolverError<SSLStackProtocol>(client_wrong_number_parameters);
}

TEST(PhysicalLayerTest, EmptyDatagramProtocolStackOverUDPTest) {
  typedef DatagramLayer<DatagramLayer<
      DatagramLayer<DatagramLayer<virtual_network::physical_layer::udp>>>>
      DatagramStackProtocol;

  tests::virtual_network_helpers::Parameters socket1_udp_parameters;
  socket1_udp_parameters["addr"] = "127.0.0.1";
  socket1_udp_parameters["port"] = "8000";
  tests::virtual_network_helpers::ParametersList socket1_parameters;
  socket1_parameters.push_back(socket1_udp_parameters);

  tests::virtual_network_helpers::Parameters socket2_udp_parameters;
  socket2_udp_parameters["addr"] = "127.0.0.1";
  socket2_udp_parameters["port"] = "9000";
  tests::virtual_network_helpers::ParametersList socket2_parameters;
  socket2_parameters.push_back(socket2_udp_parameters);

  TestNoConnectionDatagramProtocol<DatagramStackProtocol>(
      socket1_parameters, socket2_parameters, 100);

  TestConnectionDatagramProtocol<DatagramStackProtocol>(
      socket1_parameters, socket1_parameters, 100);
}
