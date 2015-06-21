#ifndef SSF_TESTS_STREAM_PROTOCOL_HELPERS_H_
#define SSF_TESTS_STREAM_PROTOCOL_HELPERS_H_

#include <gtest/gtest.h>

#include <cstdint>

#include <array>
#include <list>
#include <map>

#include <boost/asio/io_service.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/asio/spawn.hpp>

#include <boost/asio/socket_base.hpp>

#include <boost/system/error_code.hpp>

#include "core/virtual_network/parameters.h"

#include "virtual_network_helpers.h"

/// Test a stream protocol
/// Bind an acceptor to the endpoint defined by
///   acceptor_parameters
/// Connect a client socket to an endpoint defined
///   by client_parameters
/// Send data in ping pong mode
template <class StreamProtocol>
void TestStreamProtocol(
    virtual_network::ParameterStack client_parameters,
    virtual_network::ParameterStack acceptor_parameters,
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
    socket2.local_endpoint(endpoint_ec);
    ASSERT_EQ(0, endpoint_ec.value())
        << "Local endpoint should be set: " << endpoint_ec.message();

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

    {
      boost::system::error_code endpoint_ec;
      auto local_ep = socket1.local_endpoint(endpoint_ec);
      auto remote_ep = socket2.remote_endpoint(endpoint_ec);
      ASSERT_EQ(local_ep.endpoint_context(), remote_ep.endpoint_context())
          << "Endpoints should be equal";
    }

    {
      boost::system::error_code endpoint_ec;
      auto local_ep = socket2.local_endpoint(endpoint_ec);
      auto remote_ep = socket1.remote_endpoint(endpoint_ec);
      ASSERT_EQ(local_ep.endpoint_context(), remote_ep.endpoint_context())
          << "Endpoints should be equal";
    }

    ++count1;
    if (count1 < max_packets) {
      boost::asio::async_write(socket1, boost::asio::buffer(buffer1),
                               sent_handler1);
    } else {
      ASSERT_EQ(max_packets, count1);
      boost::system::error_code close_ec;

      acceptor.close(close_ec);
      ASSERT_EQ(false, acceptor.is_open());

      socket1.shutdown(boost::asio::socket_base::shutdown_both,
                       close_ec);
      socket1.close(close_ec);
      ASSERT_EQ(false, socket1.is_open());

      socket2.shutdown(boost::asio::socket_base::shutdown_both,
                        close_ec);
      socket2.close(close_ec);
      ASSERT_EQ(false, socket2.is_open());
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

/// Test a stream protocol future interface
template <class StreamProtocol>
void TestStreamProtocolFuture(
    virtual_network::ParameterStack client_parameters,
    virtual_network::ParameterStack acceptor_parameters) {
  typedef std::array<uint8_t, 500> Buffer;
  boost::asio::io_service io_service;
  boost::system::error_code resolve_ec;
  auto p_worker = std::unique_ptr<boost::asio::io_service::work>(
      new boost::asio::io_service::work(io_service));

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

  boost::system::error_code ec;

  acceptor.open();
  acceptor.bind(acceptor_endpoint, ec);
  ASSERT_EQ(0, ec.value()) << "Bind acceptor should not be in error: "
                           << ec.message();
  acceptor.listen(100, ec);
  ASSERT_EQ(0, ec.value()) << "Listen acceptor should not be in error: "
                           << ec.message();

  auto lambda2 = [&]() {
    try {
      auto accepted = acceptor.async_accept(socket2, boost::asio::use_future);
      accepted.get();
      auto read = boost::asio::async_read(
          socket2, boost::asio::buffer(r_buffer2), boost::asio::use_future);
      read.get();
      auto written = boost::asio::async_write(
          socket2, boost::asio::buffer(buffer2), boost::asio::use_future);
      written.get();
    }
    catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
    }
  };

  auto lambda1 = [&]() {
    try {
      auto connected =
          socket1.async_connect(remote_endpoint, boost::asio::use_future);
      connected.get();
      auto written = boost::asio::async_write(
          socket1, boost::asio::buffer(buffer1), boost::asio::use_future);
      written.get();
      auto read = boost::asio::async_read(
          socket1, boost::asio::buffer(r_buffer1), boost::asio::use_future);
      read.get();

      socket1.shutdown(boost::asio::socket_base::shutdown_both);
      socket1.close();
      ASSERT_EQ(false, socket1.is_open());

      socket2.shutdown(boost::asio::socket_base::shutdown_both);
      socket2.close();
      ASSERT_EQ(false, socket2.is_open());

      acceptor.close();
      ASSERT_EQ(false, acceptor.is_open());

      p_worker.reset();
    }
    catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
    }
  };

  boost::thread_group threads;
  threads.create_thread(lambda2);
  threads.create_thread(lambda1);
  for (uint16_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    threads.create_thread([&io_service]() { io_service.run(); });
  }
  threads.join_all();
}

/// Test a stream protocol spawn interface
template <class StreamProtocol>
void TestStreamProtocolSpawn(
    virtual_network::ParameterStack client_parameters,
    virtual_network::ParameterStack acceptor_parameters) {
  typedef std::array<uint8_t, 500> Buffer;
  boost::asio::io_service io_service;
  boost::system::error_code resolve_ec;

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

  boost::system::error_code ec;

  acceptor.open();
  acceptor.bind(acceptor_endpoint, ec);
  ASSERT_EQ(0, ec.value()) << "Bind acceptor should not be in error: "
                           << ec.message();
  acceptor.listen(100, ec);
  ASSERT_EQ(0, ec.value()) << "Listen acceptor should not be in error: "
                           << ec.message();

  auto lambda2 = [&](boost::asio::yield_context yield) {
    try {
      acceptor.async_accept(socket2, yield);
      boost::asio::async_read(
          socket2, boost::asio::buffer(r_buffer2), yield);
      boost::asio::async_write(
          socket2, boost::asio::buffer(buffer2), yield);
    }
    catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
    }
  };

  auto lambda1 = [&](boost::asio::yield_context yield) {
    try {
      socket1.async_connect(remote_endpoint, yield);
      boost::asio::async_write(
          socket1, boost::asio::buffer(buffer1), yield);
      boost::asio::async_read(
          socket1, boost::asio::buffer(r_buffer1), yield);

      socket1.shutdown(boost::asio::socket_base::shutdown_both);
      socket1.close();
      ASSERT_EQ(false, socket1.is_open());

      socket2.shutdown(boost::asio::socket_base::shutdown_both);
      socket2.close();
      ASSERT_EQ(false, socket2.is_open());

      acceptor.close();
      ASSERT_EQ(false, acceptor.is_open());
    }
    catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
    }
  };

  boost::thread_group threads;
  boost::asio::spawn(io_service, lambda2);
  boost::asio::spawn(io_service, lambda1);
  for (uint16_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    threads.create_thread([&io_service]() { io_service.run(); });
  }
  threads.join_all();
}

/// Test a stream protocol synchronous interface
template <class StreamProtocol>
void TestStreamProtocolSynchronous(
    virtual_network::ParameterStack client_parameters,
    virtual_network::ParameterStack acceptor_parameters) {
  typedef std::array<uint8_t, 500> Buffer;
  boost::asio::io_service io_service;
  boost::system::error_code resolve_ec;
  auto p_worker = std::unique_ptr<boost::asio::io_service::work>(
      new boost::asio::io_service::work(io_service));

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

  boost::system::error_code ec;

  acceptor.open();
  acceptor.bind(acceptor_endpoint, ec);
  ASSERT_EQ(0, ec.value()) << "Bind acceptor should not be in error: "
                           << ec.message();
  acceptor.listen(100, ec);
  ASSERT_EQ(0, ec.value()) << "Listen acceptor should not be in error: "
                           << ec.message();

  auto lambda2 = [&]() {
    try {
      acceptor.accept(socket2);
      boost::asio::read(socket2, boost::asio::buffer(r_buffer2));
      boost::asio::write(socket2, boost::asio::buffer(buffer2));
    }
    catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
    }
  };

  auto lambda1 = [&]() {
    try {
      socket1.connect(remote_endpoint);
      boost::asio::write(socket1, boost::asio::buffer(buffer1));
      boost::asio::read(socket1, boost::asio::buffer(r_buffer1));

      socket1.shutdown(boost::asio::socket_base::shutdown_both);
      socket1.close();
      ASSERT_EQ(false, socket1.is_open());

      socket2.shutdown(boost::asio::socket_base::shutdown_both);
      socket2.close();
      ASSERT_EQ(false, socket2.is_open());

      acceptor.close();
      ASSERT_EQ(false, acceptor.is_open());

      p_worker.reset();
    }
    catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
    }
  };

  boost::thread_group threads;
  threads.create_thread(std::move(lambda2));
  threads.create_thread(std::move(lambda1));
  for (uint16_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    threads.create_thread([&io_service]() { io_service.run(); });
  }
  threads.join_all();
}

/// Check connection failure to an endpoint not listening
template <class StreamProtocol>
void TestStreamErrorConnectionProtocol(
    virtual_network::ParameterStack
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
    virtual_network::ParameterStack wrong_parameters) {
  boost::asio::io_service io_service;
  boost::system::error_code resolve_ec;

  typename StreamProtocol::resolver resolver(io_service);

  auto wrong_resolving_endpoint_it =
      resolver.resolve(wrong_parameters, resolve_ec);
  ASSERT_NE(0, resolve_ec.value()) << "Resolving should be in error";
}

#endif  // SSF_TESTS_STREAM_PROTOCOL_HELPERS_H_
