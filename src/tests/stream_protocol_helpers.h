#ifndef SSF_TESTS_STREAM_PROTOCOL_HELPERS_H_
#define SSF_TESTS_STREAM_PROTOCOL_HELPERS_H_

#include <gtest/gtest.h>

#include <cstdint>

#include <array>
#include <memory>
#include <vector>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/use_future.hpp>

#include <boost/system/error_code.hpp>

#include "core/virtual_network/physical_layer/tcp_helpers.h"

#include "core/virtual_network/parameters.h"

#include "tests/tools.h"
#include "tests/virtual_network_helpers.h"

const std::size_t buffer_size = 1024;

void TCPPerfTestHalfDuplex(virtual_network::ParameterStack client_parameters,
                           virtual_network::ParameterStack acceptor_parameters,
                           uint64_t mbytes) {
  using Buffer = std::vector<uint8_t>;

  boost::asio::io_service io_service;
  boost::system::error_code resolve_ec;

  uint64_t data_size_to_send = mbytes * 1024ULL * 1024ULL;

  auto p_bandwidth =
      std::make_shared<ScopedBandWidth>(data_size_to_send * 8ULL);

  uint64_t count1 = 0;
  uint64_t count2 = 0;

  Buffer buffer1(1024 * 1024);
  Buffer r_buffer2(1024 * 1024);
  tests::virtual_network_helpers::ResetBuffer(&buffer1, 1);
  tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0);

  boost::asio::ip::tcp::socket socket1(io_service);
  boost::asio::ip::tcp::socket socket2(io_service);
  boost::asio::ip::tcp::acceptor acceptor(io_service);

  boost::asio::ip::tcp::endpoint acceptor_endpoint(
      virtual_network::physical_layer::detail::make_tcp_endpoint(
          io_service, acceptor_parameters.front(), resolve_ec));

  boost::asio::ip::tcp::endpoint remote_endpoint(
      virtual_network::physical_layer::detail::make_tcp_endpoint(
          io_service, client_parameters.front(), resolve_ec));

  tests::virtual_network_helpers::AcceptHandler accepted;
  tests::virtual_network_helpers::ConnectHandler connected;
  tests::virtual_network_helpers::SendHandler sent_handler1;
  tests::virtual_network_helpers::ReceiveHandler received_handler2;

  connected = [&, p_bandwidth](const boost::system::error_code& ec) {
    ASSERT_EQ(0, ec.value())
        << "Connect handler should not be in error: " << ec.message();

    p_bandwidth->ResetTime();

    if (data_size_to_send - count1 >= buffer1.size()) {
      socket1.async_send(boost::asio::buffer(buffer1), sent_handler1);
    } else {
      socket1.async_send(
          boost::asio::buffer(&buffer1[0], static_cast<std::size_t>(
                                               data_size_to_send - count1)),
          sent_handler1);
    }
  };

  sent_handler1 = [&, p_bandwidth](const boost::system::error_code& ec,
                                   std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Send handler should not be in error: " << ec.message();

    count1 += length;
    if (count1 < data_size_to_send) {
      if (data_size_to_send - count1 >= buffer1.size()) {
        socket1.async_send(boost::asio::buffer(buffer1), sent_handler1);
      } else {
        socket1.async_send(
            boost::asio::buffer(&buffer1[0], static_cast<std::size_t>(
                                                 data_size_to_send - count1)),
            sent_handler1);
      }
    }
  };

  accepted = [&](const boost::system::error_code& ec) {
    ASSERT_EQ(0, ec.value())
        << "Accept handler should not be in error: " << ec.message();

    socket2.async_receive(boost::asio::buffer(r_buffer2), received_handler2);
  };

  received_handler2 = [&](const boost::system::error_code& ec,
                          std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Receive handler should not be in error: " << ec.message();
    ASSERT_TRUE(tests::virtual_network_helpers::CheckBuffers(buffer1, r_buffer2,
                                                             length));
    tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0, length);

    count2 += length;
    if (count2 < data_size_to_send) {
      socket2.async_receive(boost::asio::buffer(r_buffer2), received_handler2);
    } else {
      ASSERT_EQ(data_size_to_send, count2);
      boost::system::error_code close_ec;
      socket1.close(close_ec);
      socket2.close(close_ec);
      acceptor.close(close_ec);
    }
  };

  boost::system::error_code ec;

  acceptor.open(boost::asio::ip::tcp::v4());
  acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
  acceptor.bind(acceptor_endpoint, ec);
  ASSERT_EQ(0, ec.value()) << "Bind acceptor should not be in error: "
                           << ec.message();
  acceptor.listen(100, ec);
  ASSERT_EQ(0, ec.value()) << "Listen acceptor should not be in error: "
                           << ec.message();

  acceptor.async_accept(socket2, accepted);
  socket1.async_connect(remote_endpoint, connected);

  p_bandwidth.reset();

  boost::thread_group threads;
  for (uint16_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    threads.create_thread([&io_service]() { io_service.run(); });
  }
  threads.join_all();
}

template <class StreamProtocol>
void PerfTestStreamProtocolHalfDuplex(
    typename StreamProtocol::resolver::query client_parameters,
    typename StreamProtocol::resolver::query acceptor_parameters,
    uint64_t mbytes) {
  using Buffer = std::vector<uint8_t>;

  boost::asio::io_service io_service;
  boost::system::error_code resolve_ec;

  uint64_t data_size_to_send = mbytes * 1024ULL * 1024ULL;

  auto p_bandwidth1 =
      std::make_shared<ScopedBandWidth>(data_size_to_send * 8ULL);

  uint64_t s_count1 = 0;
  uint64_t count1 = 0;

  Buffer buffer1(1024 * 1024);
  Buffer r_buffer2(1024 * 1024);
  tests::virtual_network_helpers::ResetBuffer(&buffer1, 1);
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
  tests::virtual_network_helpers::ReceiveHandler received_handler2;

  connected = [&, p_bandwidth1](const boost::system::error_code& ec) {
    ASSERT_EQ(0, ec.value())
        << "Connect handler should not be in error: " << ec.message();

    p_bandwidth1->ResetTime();

    if (data_size_to_send - s_count1 >= buffer1.size()) {
      socket1.async_send(boost::asio::buffer(buffer1), sent_handler1);
    } else {
      socket1.async_send(
          boost::asio::buffer(&buffer1[0], static_cast<std::size_t>(
                                               data_size_to_send - s_count1)),
          sent_handler1);
    }
  };

  sent_handler1 = [&, p_bandwidth1](const boost::system::error_code& ec,
                                    std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Send handler should not be in error: " << ec.message();

    s_count1 += length;
    if (s_count1 < data_size_to_send) {
      if (data_size_to_send - s_count1 >= buffer1.size()) {
        socket1.async_send(boost::asio::buffer(buffer1), sent_handler1);
      } else {
        socket1.async_send(
            boost::asio::buffer(&buffer1[0], static_cast<std::size_t>(
                                                 data_size_to_send - s_count1)),
            sent_handler1);
      }
    }
  };

  accepted = [&](const boost::system::error_code& ec) {
    ASSERT_EQ(0, ec.value())
        << "Accept handler should not be in error: " << ec.message();

    socket2.async_receive(boost::asio::buffer(r_buffer2), received_handler2);
  };

  received_handler2 = [&](const boost::system::error_code& ec,
                          std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Receive handler should not be in error: " << ec.message();
    ASSERT_TRUE(tests::virtual_network_helpers::CheckBuffers(buffer1, r_buffer2,
                                                             length));
    tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0, length);

    count1 += length;
    if (count1 < data_size_to_send) {
      socket2.async_receive(boost::asio::buffer(r_buffer2), received_handler2);
    } else {
      ASSERT_EQ(data_size_to_send, count1);
      boost::system::error_code close_ec;
      socket1.close(close_ec);
      socket2.close(close_ec);
      acceptor.close(close_ec);
    }
  };

  boost::system::error_code ec;

  acceptor.open();
  acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
  acceptor.bind(acceptor_endpoint, ec);
  ASSERT_EQ(0, ec.value()) << "Bind acceptor should not be in error: "
                           << ec.message();
  acceptor.listen(100, ec);
  ASSERT_EQ(0, ec.value()) << "Listen acceptor should not be in error: "
                           << ec.message();

  acceptor.async_accept(socket2, accepted);
  socket1.async_connect(remote_endpoint, connected);

  p_bandwidth1.reset();

  boost::thread_group threads;
  for (uint16_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    threads.create_thread([&io_service]() { io_service.run(); });
  }
  threads.join_all();
}

template <class StreamProtocol>
void PerfTestStreamProtocolFullDuplex(
    typename StreamProtocol::resolver::query client_parameters,
    typename StreamProtocol::resolver::query acceptor_parameters,
    uint64_t mbytes) {
  using Buffer = std::vector<uint8_t>;

  std::promise<bool> finished1;
  std::promise<bool> finished2;

  boost::asio::io_service io_service;
  boost::system::error_code resolve_ec;

  uint64_t data_size_to_send = mbytes * 1024ULL * 1024ULL;

  auto p_bandwidth1 =
      std::make_shared<ScopedBandWidth>(data_size_to_send * 8ULL);
  auto p_bandwidth2 =
      std::make_shared<ScopedBandWidth>(data_size_to_send * 8ULL);

  uint64_t s_count1 = 0;
  uint64_t count1 = 0;
  uint64_t s_count2 = 0;
  uint64_t count2 = 0;

  Buffer buffer1(1024 * 1024);
  Buffer buffer2(1024 * 1024);
  Buffer r_buffer1(1024 * 1024);
  Buffer r_buffer2(1024 * 1024);
  tests::virtual_network_helpers::ResetBuffer(&buffer1, 1);
  tests::virtual_network_helpers::ResetBuffer(&buffer2, 1);
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

  connected = [&, p_bandwidth1](const boost::system::error_code& ec) {
    ASSERT_EQ(0, ec.value())
        << "Connect handler should not be in error: " << ec.message();

    socket1.async_receive(boost::asio::buffer(r_buffer1), received_handler1);

    p_bandwidth1->ResetTime();

    if (data_size_to_send >= buffer1.size()) {
      socket1.async_send(boost::asio::buffer(buffer1), sent_handler1);
    } else {
      socket1.async_send(
          boost::asio::buffer(&buffer1[0],
                              static_cast<std::size_t>(data_size_to_send)),
          sent_handler1);
    }
  };

  accepted = [&, p_bandwidth2](const boost::system::error_code& ec) {
    ASSERT_EQ(0, ec.value())
        << "Accept handler should not be in error: " << ec.message();

    socket2.async_receive(boost::asio::buffer(r_buffer2), received_handler2);

    p_bandwidth2->ResetTime();

    if (data_size_to_send >= buffer2.size()) {
      socket2.async_send(boost::asio::buffer(buffer2), sent_handler2);
    } else {
      socket2.async_send(
          boost::asio::buffer(&buffer2[0],
                              static_cast<std::size_t>(data_size_to_send)),
          sent_handler2);
    }
  };

  sent_handler1 = [&, p_bandwidth1](const boost::system::error_code& ec,
                                    std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Send handler should not be in error: " << ec.message();

    s_count1 += length;

    if (s_count1 < data_size_to_send) {
      if (data_size_to_send - s_count1 >= buffer1.size()) {
        socket1.async_send(boost::asio::buffer(buffer1), sent_handler1);
      } else {
        socket1.async_send(
            boost::asio::buffer(&buffer1[0], static_cast<std::size_t>(
                                                 data_size_to_send - s_count1)),
            sent_handler1);
      }
    }
  };

  sent_handler2 = [&, p_bandwidth2](const boost::system::error_code& ec,
                                    std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Send handler should not be in error: " << ec.message();

    s_count2 += length;

    if (s_count2 < data_size_to_send) {
      if (data_size_to_send - s_count2 >= buffer2.size()) {
        socket2.async_send(boost::asio::buffer(buffer2), sent_handler2);
      } else {
        socket2.async_send(
            boost::asio::buffer(&buffer2[0], static_cast<std::size_t>(
                                                 data_size_to_send - s_count2)),
            sent_handler2);
      }
    }
  };

  received_handler1 = [&](const boost::system::error_code& ec,
                          std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Receive handler should not be in error: " << ec.message();
    ASSERT_TRUE(tests::virtual_network_helpers::CheckBuffers(buffer2, r_buffer1,
                                                             length));
    tests::virtual_network_helpers::ResetBuffer(&r_buffer1, 0, length);

    count2 += length;

    if (count2 < data_size_to_send) {
      socket1.async_receive(boost::asio::buffer(r_buffer1), received_handler1);
    } else {
      ASSERT_EQ(data_size_to_send, count2);
      finished2.set_value(true);
    }
  };

  received_handler2 = [&](const boost::system::error_code& ec,
                          std::size_t length) {
    ASSERT_EQ(0, ec.value())
        << "Receive handler should not be in error: " << ec.message();
    ASSERT_TRUE(tests::virtual_network_helpers::CheckBuffers(buffer1, r_buffer2,
                                                             length));
    tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0, length);

    count1 += length;

    if (count1 < data_size_to_send) {
      socket2.async_receive(boost::asio::buffer(r_buffer2), received_handler2);
    } else {
      ASSERT_EQ(data_size_to_send, count1);
      finished1.set_value(true);
    }
  };

  boost::system::error_code ec;

  acceptor.open();
  acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
  acceptor.bind(acceptor_endpoint, ec);
  ASSERT_EQ(0, ec.value()) << "Bind acceptor should not be in error: "
                           << ec.message();
  acceptor.listen(100, ec);
  ASSERT_EQ(0, ec.value()) << "Listen acceptor should not be in error: "
                           << ec.message();

  p_bandwidth1.reset();
  p_bandwidth2.reset();

  acceptor.async_accept(socket2, accepted);
  socket1.async_connect(remote_endpoint, connected);

  boost::thread_group threads;
  for (uint16_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    threads.create_thread([&io_service]() { io_service.run(); });
  }

  finished1.get_future().wait();
  finished2.get_future().wait();

  boost::system::error_code close_ec;
  socket1.close(close_ec);
  socket2.close(close_ec);
  acceptor.close(close_ec);

  threads.join_all();
}

/// Test a stream protocol
/// Bind an acceptor to the endpoint defined by
///   acceptor_parameters
/// Connect a client socket to an endpoint defined
///   by client_parameters
/// Send data in ping pong mode
template <class StreamProtocol>
void TestStreamProtocol(
    typename StreamProtocol::resolver::query client_parameters,
    typename StreamProtocol::resolver::query acceptor_parameters,
    uint64_t max_packets) {
  std::cout << "  * TestStreamProtocol" << std::endl;
  typedef std::array<uint8_t, buffer_size> Buffer;
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

  received_handler1 =
      [&](const boost::system::error_code& ec, std::size_t length) {
        ASSERT_EQ(0, ec.value())
            << "Receive handler should not be in error: " << ec.message();
        ASSERT_TRUE(tests::virtual_network_helpers::CheckBuffers(
            buffer2, r_buffer1, length));

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

          socket1.shutdown(boost::asio::socket_base::shutdown_both, close_ec);
          socket1.close(close_ec);
          ASSERT_EQ(false, socket1.is_open());

          socket2.shutdown(boost::asio::socket_base::shutdown_both, close_ec);
          socket2.close(close_ec);
          ASSERT_EQ(false, socket2.is_open());
        }
      };

  received_handler2 =
      [&](const boost::system::error_code& ec, std::size_t length) {
        ASSERT_EQ(0, ec.value())
            << "Receive handler should not be in error: " << ec.message();
        ASSERT_TRUE(tests::virtual_network_helpers::CheckBuffers(
            buffer1, r_buffer2, length));
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
  acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
  acceptor.bind(acceptor_endpoint, ec);
  ASSERT_EQ(0, ec.value()) << "Bind acceptor should not be in error: "
                           << ec.message();
  acceptor.listen(100, ec);
  ASSERT_EQ(0, ec.value()) << "Listen acceptor should not be in error: "
                           << ec.message();
  acceptor.async_accept(socket2, accepted);

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
    typename StreamProtocol::resolver::query client_parameters,
    typename StreamProtocol::resolver::query acceptor_parameters) {
  std::cout << "  * TestStreamProtocolFuture" << std::endl;
  typedef std::array<uint8_t, buffer_size> Buffer;
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
  acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
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
    } catch (const std::exception& e) {
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
    } catch (const std::exception& e) {
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
    typename StreamProtocol::resolver::query client_parameters,
    typename StreamProtocol::resolver::query acceptor_parameters) {
  std::cout << "  * TestStreamProtocolSpawn" << std::endl;
  typedef std::array<uint8_t, buffer_size> Buffer;
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
  acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
  acceptor.bind(acceptor_endpoint, ec);
  ASSERT_EQ(0, ec.value()) << "Bind acceptor should not be in error: "
                           << ec.message();
  acceptor.listen(100, ec);
  ASSERT_EQ(0, ec.value()) << "Listen acceptor should not be in error: "
                           << ec.message();

  auto lambda2 = [&](boost::asio::yield_context yield) {
    try {
      acceptor.async_accept(socket2, yield);
      boost::asio::async_read(socket2, boost::asio::buffer(r_buffer2), yield);
      boost::asio::async_write(socket2, boost::asio::buffer(buffer2), yield);
    } catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
    }
  };

  auto lambda1 = [&](boost::asio::yield_context yield) {
    try {
      socket1.async_connect(remote_endpoint, yield);
      boost::asio::async_write(socket1, boost::asio::buffer(buffer1), yield);
      boost::asio::async_read(socket1, boost::asio::buffer(r_buffer1), yield);

      socket1.shutdown(boost::asio::socket_base::shutdown_both);
      socket1.close();
      ASSERT_EQ(false, socket1.is_open());

      socket2.shutdown(boost::asio::socket_base::shutdown_both);
      socket2.close();
      ASSERT_EQ(false, socket2.is_open());

      acceptor.close();
      ASSERT_EQ(false, acceptor.is_open());
    } catch (const std::exception& e) {
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
    typename StreamProtocol::resolver::query client_parameters,
    typename StreamProtocol::resolver::query acceptor_parameters) {
  typedef std::array<uint8_t, buffer_size> Buffer;
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
  acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
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
    } catch (const std::exception& e) {
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
    } catch (const std::exception& e) {
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
    typename StreamProtocol::resolver::query client_no_connection_parameters) {
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

  tests::virtual_network_helpers::ConnectHandler connection_error_handler =
      [](const boost::system::error_code& ec) {
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
    typename StreamProtocol::resolver::query wrong_parameters) {
  boost::asio::io_service io_service;
  boost::system::error_code resolve_ec;

  typename StreamProtocol::resolver resolver(io_service);

  auto wrong_resolving_endpoint_it =
      resolver.resolve(wrong_parameters, resolve_ec);
  ASSERT_NE(0, resolve_ec.value()) << "Resolving should be in error";
}

#endif  // SSF_TESTS_STREAM_PROTOCOL_HELPERS_H_
