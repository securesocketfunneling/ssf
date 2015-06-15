#ifndef SSF_TESTS_STREAM_PROTOCOL_HELPERS_H_
#define SSF_TESTS_STREAM_PROTOCOL_HELPERS_H_

#include <gtest/gtest.h>

#include <cstdint>

#include <array>
#include <list>
#include <map>

#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include "virtual_network_helpers.h"

/// Test a stream protocol
/// Bind an acceptor to the endpoint defined by
///   acceptor_parameters
/// Connect a client socket to an endpoint defined
///   by client_parameters
/// Send data in ping pong mode
template <class StreamProtocol>
void TestStreamProtocol(
    tests::virtual_network_helpers::ParametersList client_parameters,
    tests::virtual_network_helpers::ParametersList acceptor_parameters,
    uint64_t max_packets) {
  typedef std::array<uint8_t, 500> Buffer;
  boost::asio::io_service io_service;
  boost::system::error_code resolve_ec;

  uint64_t count1 = 0;
  uint64_t count2 = 0;

  Buffer buffer1;
  Buffer buffer2;
  Buffer r_buffer1;
  Buffer r_buffer2;
  tests::virtual_network_helpers::ResetBuffer(&buffer1, 1);
  tests::virtual_network_helpers::ResetBuffer(&buffer2, 2);
  tests::virtual_network_helpers::ResetBuffer(&r_buffer1, 0);
  tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0);

  typename StreamProtocol::socket socket1(io_service);
  typename StreamProtocol::socket socket2(io_service);
  typename StreamProtocol::acceptor acceptor(io_service);
  typename StreamProtocol::resolver resolver(io_service);

  auto acceptor_endpoint_it = resolver.resolve(acceptor_parameters, resolve_ec);
  ASSERT_EQ(0, resolve_ec.value())
      << "Resolving acceptor endpoint should not be in error: "
      << resolve_ec.message();

  typename StreamProtocol::endpoint acceptor_endpoint(*acceptor_endpoint_it);

  auto remote_endpoint_it = resolver.resolve(client_parameters, resolve_ec);
  ASSERT_EQ(0, resolve_ec.value())
      << "Resolving remote endpoint should not be in error: "
      << resolve_ec.message();
  typename StreamProtocol::endpoint remote_endpoint(*remote_endpoint_it);

  tests::virtual_network_helpers::AcceptHandler accepted;
  tests::virtual_network_helpers::ConnectHandler connected;
  tests::virtual_network_helpers::SendHandler sent_handler1;
  tests::virtual_network_helpers::SendHandler sent_handler2;
  tests::virtual_network_helpers::ReceiveHandler received_handler1;
  tests::virtual_network_helpers::ReceiveHandler received_handler2;

  accepted = [&](const boost::system::error_code& ec) {
    ASSERT_EQ(0, ec.value())
        << "Accept handler should not be in error: " << ec.message();

    boost::system::error_code endpoint_ec;
    auto local_ep = socket2.local_endpoint(endpoint_ec);
    ASSERT_EQ(0, endpoint_ec.value())
        << "Local endpoint should be set: " << endpoint_ec.message();
    ASSERT_EQ(acceptor_endpoint, local_ep) << "Local endpoint invalid";

    socket2.remote_endpoint(endpoint_ec);
    ASSERT_EQ(0, endpoint_ec.value())
        << "Remote endpoint should be set: " << endpoint_ec.message();

    boost::asio::async_read(socket2, boost::asio::buffer(r_buffer2),
                            received_handler2);
  };

  connected = [&](const boost::system::error_code& ec) {
    ASSERT_EQ(0, ec.value())
        << "Connect handler should not be in error: " << ec.message();

    boost::system::error_code endpoint_ec;
    socket1.local_endpoint(endpoint_ec);
    ASSERT_EQ(0, endpoint_ec.value())
        << "Local endpoint should be set: " << endpoint_ec.message();

    auto remote_ep = socket1.remote_endpoint(endpoint_ec);
    ASSERT_EQ(0, endpoint_ec.value())
        << "Remote endpoint should be set: " << endpoint_ec.message();
    ASSERT_EQ(remote_endpoint, remote_ep) << "Remote endpoint invalid";

    boost::asio::async_write(socket1, boost::asio::buffer(buffer1),
                             sent_handler1);
  };

  received_handler1 = [&](const boost::system::error_code& ec,
                          std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Receive handler should not be in error: " << ec.message();
    ASSERT_TRUE(
        tests::virtual_network_helpers::CheckBuffers(buffer2, r_buffer1));

    boost::system::error_code endpoint_ec;
    auto local_ep = socket1.local_endpoint(endpoint_ec);
    auto remote_ep = socket2.remote_endpoint(endpoint_ec);
    ASSERT_EQ(local_ep, remote_ep) << "Endpoints should be equal";

    ++count1;
    if (count1 < max_packets) {
      boost::asio::async_write(socket1, boost::asio::buffer(buffer1),
                               sent_handler1);
    } else {
      ASSERT_EQ(max_packets, count1);
      boost::system::error_code close_ec;
      acceptor.close(close_ec);
      socket1.close(close_ec);
      socket2.close(close_ec);
    }
  };

  received_handler2 = [&](const boost::system::error_code& ec,
                          std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Receive handler should not be in error: " << ec.message();
    ASSERT_TRUE(
        tests::virtual_network_helpers::CheckBuffers(buffer1, r_buffer2));
    ++count2;
    if (count2 < max_packets) {
      boost::asio::async_write(socket2, boost::asio::buffer(buffer2),
                               sent_handler2);
    } else {
      boost::asio::async_write(
          socket2, boost::asio::buffer(buffer2),
          [](const boost::system::error_code&, std::size_t) {});
      ASSERT_EQ(max_packets, count2);
    }
  };

  sent_handler1 = [&](const boost::system::error_code& ec, std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Send handler should not be in error: " << ec.message();
    tests::virtual_network_helpers::ResetBuffer(&r_buffer1, 0);
    boost::asio::async_read(socket1, boost::asio::buffer(r_buffer1),
                            received_handler1);
  };

  sent_handler2 = [&](const boost::system::error_code& ec, std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Send handler should not be in error: " << ec.message();
    tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0);
    boost::asio::async_read(socket2, boost::asio::buffer(r_buffer2),
                            received_handler2);
  };

  boost::system::error_code ec;

  acceptor.open();
  acceptor.bind(acceptor_endpoint, ec);
  ASSERT_EQ(0, ec.value()) << "Bind acceptor should not be in error: "
                           << ec.message();
  acceptor.listen(100, ec);
  ASSERT_EQ(0, ec.value()) << "Listen acceptor should not be in error: "
                           << ec.message();
  acceptor.async_accept(socket2, accepted);

  std::cout << "Transfering " << StreamProtocol::next_layer_protocol::mtu
            << " * " << max_packets << " = "
            << StreamProtocol::next_layer_protocol::mtu* max_packets
            << " bytes in both direction" << std::endl;

  socket1.async_connect(remote_endpoint, connected);

  boost::thread_group threads;
  for (uint16_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    threads.create_thread([&io_service]() { io_service.run(); });
  }
  threads.join_all();
}

/// Check connection failure to an endpoint not listening
template <class StreamProtocol>
void TestStreamErrorConnectionProtocol(
    tests::virtual_network_helpers::ParametersList
        client_no_connection_parameters) {
  boost::asio::io_service io_service;
  boost::system::error_code resolve_ec;

  typename StreamProtocol::socket socket_client_no_connection(io_service);
  typename StreamProtocol::resolver resolver(io_service);

  auto remote_no_connect_endpoint_it =
      resolver.resolve(client_no_connection_parameters, resolve_ec);
  ASSERT_EQ(0, resolve_ec.value())
      << "Resolving should not be in error: " << resolve_ec.message();

  typename StreamProtocol::endpoint remote_no_connect_endpoint(
      *remote_no_connect_endpoint_it);

  tests::virtual_network_helpers::ConnectHandler connection_error_handler = [](
      const boost::system::error_code& ec) {
    ASSERT_NE(0, ec.value()) << "Handler should be in error";
  };

  socket_client_no_connection.async_connect(remote_no_connect_endpoint,
                                            connection_error_handler);

  boost::thread_group threads;
  for (uint16_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    threads.create_thread([&io_service]() { io_service.run(); });
  }
  threads.join_all();
}

/// Check endpoint resolving
template <class StreamProtocol>
void TestEndpointResolverError(
    tests::virtual_network_helpers::ParametersList wrong_parameters) {
  boost::asio::io_service io_service;
  boost::system::error_code resolve_ec;

  typename StreamProtocol::resolver resolver(io_service);

  auto wrong_resolving_endpoint_it =
      resolver.resolve(wrong_parameters, resolve_ec);
  ASSERT_NE(0, resolve_ec.value()) << "Resolving should be in error";
}

#endif  // SSF_TESTS_STREAM_PROTOCOL_HELPERS_H_
