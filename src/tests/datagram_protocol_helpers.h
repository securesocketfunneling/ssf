#ifndef SSF_TESTS_DATAGRAM_PROTOCOL_HELPERS_H_
#define SSF_TESTS_DATAGRAM_PROTOCOL_HELPERS_H_

#include <gtest/gtest.h>

#include <cstdint>

#include <array>
#include <list>
#include <map>

#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include "virtual_network_helpers.h"

/// Bind two sockets to the endpoints resolved by given parameters
/// Send data in ping pong mode
template <class DatagramProtocol>
void TestNoConnectionDatagramProtocol(
    tests::virtual_network_helpers::ParametersList socket1_parameters,
    tests::virtual_network_helpers::ParametersList socket2_parameters,
    uint64_t max_packets) {
  typedef std::array<uint8_t, DatagramProtocol::mtu> Buffer;
  boost::asio::io_service io_service;

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

  boost::system::error_code resolve_ec;
  typename DatagramProtocol::resolver resolver(io_service);

  auto socket1_endpoint_it = resolver.resolve(socket1_parameters, resolve_ec);
  ASSERT_EQ(0, resolve_ec.value())
      << "Resolving should not be in error: " << resolve_ec.message();
  typename DatagramProtocol::endpoint socket1_endpoint(*socket1_endpoint_it);

  auto socket2_endpoint_it = resolver.resolve(socket2_parameters, resolve_ec);
  ASSERT_EQ(0, resolve_ec.value())
      << "Resolving should not be in error: " << resolve_ec.message();
  typename DatagramProtocol::endpoint socket2_endpoint(*socket2_endpoint_it);

  typename DatagramProtocol::endpoint socket1_r_endpoint;
  typename DatagramProtocol::endpoint socket2_r_endpoint;

  typename DatagramProtocol::socket socket1(io_service);
  typename DatagramProtocol::socket socket2(io_service);

  tests::virtual_network_helpers::ReceiveHandler received_handler1;
  tests::virtual_network_helpers::ReceiveHandler received_handler2;
  tests::virtual_network_helpers::SendHandler sent_handler1;
  tests::virtual_network_helpers::SendHandler sent_handler2;

  received_handler1 = [&](const boost::system::error_code& ec,
                          std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Receive handler should not be in error: " << ec.message();
    ASSERT_TRUE(
        tests::virtual_network_helpers::CheckBuffers(buffer2, r_buffer1));
    ASSERT_TRUE(socket2_endpoint == socket1_r_endpoint)
        << "Received endpoint from socket1 is not socket2 endpoint";
    ++count1;
    if (count1 < max_packets) {
      socket1.async_send_to(boost::asio::buffer(buffer1), socket2_endpoint,
                            sent_handler1);
    } else {
      ASSERT_EQ(max_packets, count1);
      socket1.close();
    }
  };

  received_handler2 = [&](const boost::system::error_code& ec,
                          std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Receive handler should not be in error: " << ec.message();
    ASSERT_TRUE(
        tests::virtual_network_helpers::CheckBuffers(buffer1, r_buffer2));
    ASSERT_TRUE(socket1_endpoint == socket2_r_endpoint)
        << "Received endpoint from socket2 is not socket1 endpoint";
    ++count2;
    if (count2 < max_packets) {
      socket2.async_send_to(boost::asio::buffer(buffer2), socket1_endpoint,
                            sent_handler2);
    } else {
      ASSERT_EQ(max_packets, count2);
      socket2.close();
    }
  };

  sent_handler1 = [&](const boost::system::error_code& ec, std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Sent handler should not be in error: " << ec.message();
    tests::virtual_network_helpers::ResetBuffer(&r_buffer1, 0);
    socket1.async_receive_from(boost::asio::buffer(r_buffer1),
                               socket1_r_endpoint, received_handler1);
  };

  sent_handler2 = [&](const boost::system::error_code& ec, std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Sent handler should not be in error: " << ec.message();
    tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0);
    socket2.async_receive_from(boost::asio::buffer(r_buffer2),
                               socket2_r_endpoint, received_handler2);
  };

  boost::system::error_code bind_ec;
  socket1.open();
  socket1.bind(socket1_endpoint, bind_ec);
  ASSERT_EQ(0, bind_ec.value())
      << "Bind socket1 should not be in error: " << bind_ec.message();
  socket2.open();
  socket2.bind(socket2_endpoint, bind_ec);
  ASSERT_EQ(0, bind_ec.value())
      << "Bind socket2 should not be in error: " << bind_ec.message();

  std::cout << "Transfering " << DatagramProtocol::next_layer_protocol::mtu
            << " * " << max_packets << " = "
            << DatagramProtocol::next_layer_protocol::mtu* max_packets
            << " bytes in both direction" << std::endl;

  socket1.async_send_to(boost::asio::buffer(buffer1), socket2_endpoint,
                        sent_handler1);
  socket2.async_send_to(boost::asio::buffer(buffer2), socket1_endpoint,
                        sent_handler2);

  boost::thread_group threads;
  for (uint16_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    threads.create_thread([&io_service]() { io_service.run(); });
  }
  threads.join_all();
}

/// Connect socket1 to the endpoint defined by socket1_d_parameters
/// Bind socket2 to the endpoint defined by socket2_parameters
/// async_send, async_receive on socket1
/// async_send_to, async_receive_from on socket2
template <class DatagramProtocol>
void TestConnectionDatagramProtocol(
    tests::virtual_network_helpers::ParametersList socket1_d_parameters,
    tests::virtual_network_helpers::ParametersList socket2_parameters,
    uint64_t max_packets) {
  typedef std::array<uint8_t, DatagramProtocol::mtu> Buffer;
  boost::asio::io_service io_service;

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

  boost::system::error_code resolve_ec;
  typename DatagramProtocol::resolver resolver(io_service);

  auto socket1_d_endpoint_it =
      resolver.resolve(socket1_d_parameters, resolve_ec);
  ASSERT_EQ(0, resolve_ec.value())
      << "Resolving should not be in error: " << resolve_ec.message();
  typename DatagramProtocol::endpoint socket1_d_endpoint(
      *socket1_d_endpoint_it);

  auto socket2_endpoint_it = resolver.resolve(socket2_parameters, resolve_ec);
  ASSERT_EQ(0, resolve_ec.value())
      << "Resolving should not be in error: " << resolve_ec.message();
  typename DatagramProtocol::endpoint socket2_endpoint(*socket2_endpoint_it);

  typename DatagramProtocol::endpoint socket1_endpoint;
  typename DatagramProtocol::endpoint socket2_r_endpoint;

  typename DatagramProtocol::socket socket1(io_service);
  typename DatagramProtocol::socket socket2(io_service);

  tests::virtual_network_helpers::ConnectHandler connected1;
  tests::virtual_network_helpers::ReceiveHandler received_handler1;
  tests::virtual_network_helpers::ReceiveHandler received_handler2;
  tests::virtual_network_helpers::SendHandler sent_handler1;
  tests::virtual_network_helpers::SendHandler sent_handler2;

  connected1 = [&](const boost::system::error_code& ec) {
    ASSERT_EQ(0, ec.value())
        << "Connect handler should not be in error: " << ec.message();

    boost::system::error_code local_endpoint_ec;
    socket1_endpoint = socket1.local_endpoint(local_endpoint_ec);
    ASSERT_EQ(0, local_endpoint_ec.value())
        << "Local endpoint should not be in error: "
        << local_endpoint_ec.message();

    socket1.async_send(boost::asio::buffer(buffer1), sent_handler1);
    socket2.async_send_to(boost::asio::buffer(buffer2), socket1_endpoint,
                          sent_handler2);
  };

  received_handler1 = [&](const boost::system::error_code& ec,
                          std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Receive handler should not be in error: " << ec.message();
    ASSERT_TRUE(
        tests::virtual_network_helpers::CheckBuffers(buffer2, r_buffer1));

    ++count1;
    if (count1 < max_packets) {
      socket1.async_send(boost::asio::buffer(buffer1), sent_handler1);
    } else {
      ASSERT_EQ(max_packets, count1);
      socket1.close();
    }
  };

  received_handler2 = [&](const boost::system::error_code& ec,
                          std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Receive handler should not be in error: " << ec.message();
    ASSERT_TRUE(
        tests::virtual_network_helpers::CheckBuffers(buffer1, r_buffer2));
    ASSERT_TRUE(socket1_endpoint == socket2_r_endpoint)
        << "Received endpoint from socket2 is not socket1 endpoint";
    ++count2;
    if (count2 < max_packets) {
      socket2.async_send_to(boost::asio::buffer(buffer2), socket2_r_endpoint,
                            sent_handler2);
    } else {
      ASSERT_EQ(max_packets, count2);
      socket2.close();
    }
  };

  sent_handler1 = [&](const boost::system::error_code& ec, std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Send handler should not be in error: " << ec.message();
    tests::virtual_network_helpers::ResetBuffer(&r_buffer1, 0);
    socket1.async_receive(boost::asio::buffer(r_buffer1), received_handler1);
  };

  sent_handler2 = [&](const boost::system::error_code& ec, std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Send handler should not be in error: " << ec.message();
    tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0);
    socket2.async_receive_from(boost::asio::buffer(r_buffer2),
                               socket2_r_endpoint, received_handler2);
  };

  boost::system::error_code bind_ec;
  socket2.open();
  socket2.bind(socket2_endpoint, bind_ec);
  ASSERT_EQ(0, bind_ec.value())
      << "Bind socket2 should not be in error: " << bind_ec.message();

  std::cout << "Transfering " << DatagramProtocol::next_layer_protocol::mtu
            << " * " << max_packets << " = "
            << DatagramProtocol::next_layer_protocol::mtu* max_packets
            << " bytes in both direction" << std::endl;

  socket1.async_connect(socket1_d_endpoint, connected1);

  boost::thread_group threads;
  for (uint16_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    threads.create_thread([&io_service]() { io_service.run(); });
  }
  threads.join_all();
}

template <class DatagramProtocol>
void TestBindSendLocalDatagramProtocol(
    tests::virtual_network_helpers::ParametersList socket1_parameters,
    tests::virtual_network_helpers::ParametersList socket2_parameters,
    uint64_t max_packets) {
  typedef std::array<uint8_t, DatagramProtocol::mtu> Buffer;
  boost::asio::io_service io_service;

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

  boost::system::error_code resolve_ec;
  typename DatagramProtocol::resolver resolver(io_service);

  auto socket1_endpoint_it = resolver.resolve(socket1_parameters, resolve_ec);
  ASSERT_EQ(0, resolve_ec.value())
      << "Resolving should not be in error: " << resolve_ec.message();
  typename DatagramProtocol::endpoint socket1_endpoint(*socket1_endpoint_it);

  auto socket2_endpoint_it = resolver.resolve(socket2_parameters, resolve_ec);
  ASSERT_EQ(0, resolve_ec.value())
      << "Resolving should not be in error: " << resolve_ec.message();
  typename DatagramProtocol::endpoint socket2_endpoint(*socket2_endpoint_it);

  typename DatagramProtocol::endpoint socket1_r_endpoint;
  typename DatagramProtocol::endpoint socket2_r_endpoint;

  typename DatagramProtocol::socket socket1(io_service);
  typename DatagramProtocol::socket socket2(io_service);

  tests::virtual_network_helpers::ReceiveHandler received_handler1;
  tests::virtual_network_helpers::ReceiveHandler received_handler2;
  tests::virtual_network_helpers::SendHandler sent_handler1;
  tests::virtual_network_helpers::SendHandler sent_handler2;

  received_handler1 = [&](const boost::system::error_code& ec,
                          std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Receive handler should not be in error: " << ec.message();
    ASSERT_TRUE(
        tests::virtual_network_helpers::CheckBuffers(buffer2, r_buffer1));
    ++count1;
    if (count1 < max_packets) {
      socket1.async_send(boost::asio::buffer(buffer1), sent_handler1);
    } else {
      ASSERT_EQ(max_packets, count1);
      socket1.close();
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
      socket2.async_send(boost::asio::buffer(buffer2), sent_handler2);
    } else {
      ASSERT_EQ(max_packets, count2);
      socket2.close();
    }
  };

  sent_handler1 = [&](const boost::system::error_code& ec, std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Sent handler should not be in error: " << ec.message();
    tests::virtual_network_helpers::ResetBuffer(&r_buffer1, 0);
    if (!ec) {
      socket1.async_receive(boost::asio::buffer(r_buffer1), received_handler1);
    }
  };

  sent_handler2 = [&](const boost::system::error_code& ec, std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Sent handler should not be in error: " << ec.message();
    tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0);
    if (!ec) {
      socket2.async_receive(boost::asio::buffer(r_buffer2), received_handler2);
    }
  };

  boost::system::error_code bind_ec;
  socket1.open();
  socket1.bind(socket1_endpoint, bind_ec);
  ASSERT_EQ(0, bind_ec.value())
      << "Bind socket1 should not be in error: " << bind_ec.message();
  socket2.open();
  socket2.bind(socket2_endpoint, bind_ec);
  ASSERT_EQ(0, bind_ec.value())
      << "Bind socket2 should not be in error: " << bind_ec.message();

  std::cout << "Transfering " << DatagramProtocol::next_layer_protocol::mtu
            << " * " << max_packets << " = "
            << DatagramProtocol::next_layer_protocol::mtu* max_packets
            << " bytes in both direction" << std::endl;

  socket1.async_send(boost::asio::buffer(buffer1), sent_handler1);
  socket2.async_send(boost::asio::buffer(buffer2), sent_handler2);

  boost::thread_group threads;
  for (uint16_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    threads.create_thread([&io_service]() { io_service.run(); });
  }
  threads.join_all();
}

#endif  // SSF_TESTS_DATAGRAM_PROTOCOL_HELPERS_H_
