#ifndef SSF_TESTS_DATAGRAM_PROTOCOL_HELPERS_H_
#define SSF_TESTS_DATAGRAM_PROTOCOL_HELPERS_H_

#include <cstdint>

#include <array>
#include <future>

#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include "ssf/layer/parameters.h"

#include "ssf/log/log.h"

#include "tests/virtual_network_helpers.h"
#include "tests/tools.h"

template <class DatagramProtocol>
void TestDatagramProtocolPerfHalfDuplex(
    ssf::layer::ParameterStack socket1_parameters,
    ssf::layer::ParameterStack socket2_parameters,
    uint64_t size_in_mtu_multiples) {
  using Buffer = std::vector<uint8_t>;

  SSF_LOG(kLogTrace) << " * TestDatagramProtocolPerfHalfDuplex";
  SSF_LOG(kLogTrace) << "   MTU : " << DatagramProtocol::mtu;

  std::promise<bool> s_finished1;
  std::promise<bool> finished1;

  boost::asio::io_service io_service;
  boost::system::error_code resolve_ec;

  uint64_t data_size_to_send = DatagramProtocol::mtu * size_in_mtu_multiples;

  auto p_bandwidth1 =
      std::make_shared<ScopedBandWidth>(data_size_to_send * 8ULL);

  uint64_t s_count1 = 0;
  uint64_t count1 = 0;

  Buffer buffer1(DatagramProtocol::mtu);
  Buffer r_buffer2(DatagramProtocol::mtu);
  tests::virtual_network_helpers::ResetBuffer(&buffer1, 1);
  tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0);

  typename DatagramProtocol::socket socket1(io_service);
  typename DatagramProtocol::socket socket2(io_service);
  typename DatagramProtocol::resolver resolver(io_service);

  auto socket1_endpoint_it = resolver.resolve(socket1_parameters, resolve_ec);
  ASSERT_EQ(0, resolve_ec.value())
      << "Resolving should not be in error: " << resolve_ec.message();
  typename DatagramProtocol::endpoint socket1_endpoint(*socket1_endpoint_it);

  auto socket2_endpoint_it = resolver.resolve(socket2_parameters, resolve_ec);
  ASSERT_EQ(0, resolve_ec.value())
      << "Resolving should not be in error: " << resolve_ec.message();
  typename DatagramProtocol::endpoint socket2_endpoint(*socket2_endpoint_it);

  typename DatagramProtocol::endpoint socket2_r_endpoint;

  tests::virtual_network_helpers::ReceiveHandler received_handler2;
  tests::virtual_network_helpers::SendHandler sent_handler1;

  received_handler2 = [&, p_bandwidth1](const boost::system::error_code& ec,
                                        std::size_t length) {
    ASSERT_EQ(0, ec.value()) << "Error: " << ec.message();
    ASSERT_TRUE(tests::virtual_network_helpers::CheckBuffers(buffer1, r_buffer2,
                                                             length));
    tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0, length);

    count1 += length;
    if (count1 < data_size_to_send) {
      socket2.async_receive_from(boost::asio::buffer(r_buffer2),
                                 socket2_r_endpoint, received_handler2);
    } else {
      ASSERT_EQ(data_size_to_send, count1);
      finished1.set_value(true);
    }
  };

  sent_handler1 = [&](const boost::system::error_code& ec, std::size_t length) {
    ASSERT_EQ(0, ec.value()) << "Error: " << ec.message();

    s_count1 += length;

    if (count1 < data_size_to_send) {
      socket1.async_send_to(boost::asio::buffer(buffer1), socket2_endpoint,
                            sent_handler1);
    } else {
      SSF_LOG(kLogTrace) << "   Packet loss1: "
                         << static_cast<double>(s_count1 - count1) /
                                DatagramProtocol::mtu;
      s_finished1.set_value(true);
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

  boost::system::error_code ec;

  socket2.async_receive_from(boost::asio::buffer(r_buffer2), socket2_r_endpoint,
                             received_handler2);

  p_bandwidth1->ResetTime();
  socket1.async_send_to(boost::asio::buffer(buffer1), socket2_endpoint,
                        sent_handler1);
  p_bandwidth1.reset();

  std::vector<std::thread> threads;
  for (uint16_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    threads.emplace_back([&io_service]() { io_service.run(); });
  }

  s_finished1.get_future().wait();
  finished1.get_future().wait();

  socket1.close(ec);
  socket2.close(ec);
  
  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

/// Bind two sockets to the endpoints resolved by given parameters
/// Send data in ping pong mode
template <class DatagramProtocol>
void TestDatagramProtocolPerfFullDuplex(
    ssf::layer::ParameterStack socket1_parameters,
    ssf::layer::ParameterStack socket2_parameters,
    uint64_t size_in_mtu_multiples) {
  using Buffer = std::vector<uint8_t>;

  SSF_LOG(kLogTrace) << " * TestDatagramProtocolPerfFullDuplex";
  SSF_LOG(kLogTrace) << "   MTU : " << DatagramProtocol::mtu;

  std::promise<bool> s_finished1;
  std::promise<bool> finished1;
  std::promise<bool> s_finished2;
  std::promise<bool> finished2;

  boost::asio::io_service io_service;
  boost::system::error_code resolve_ec;

  uint64_t data_size_to_send = DatagramProtocol::mtu * size_in_mtu_multiples;

  auto p_bandwidth1 =
      std::make_shared<ScopedBandWidth>(data_size_to_send * 8ULL);
  auto p_bandwidth2 =
      std::make_shared<ScopedBandWidth>(data_size_to_send * 8ULL);

  uint64_t s_count1 = 0;
  uint64_t count1 = 0;
  uint64_t s_count2 = 0;
  uint64_t count2 = 0;

  Buffer buffer1(DatagramProtocol::mtu);
  Buffer buffer2(DatagramProtocol::mtu);
  Buffer r_buffer1(DatagramProtocol::mtu);
  Buffer r_buffer2(DatagramProtocol::mtu);
  tests::virtual_network_helpers::ResetBuffer(&buffer1, 1);
  tests::virtual_network_helpers::ResetBuffer(&buffer2, 1);
  tests::virtual_network_helpers::ResetBuffer(&r_buffer1, 0);
  tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0);

  typename DatagramProtocol::socket socket1(io_service);
  typename DatagramProtocol::socket socket2(io_service);
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

  tests::virtual_network_helpers::ReceiveHandler received_handler1;
  tests::virtual_network_helpers::ReceiveHandler received_handler2;
  tests::virtual_network_helpers::SendHandler sent_handler1;
  tests::virtual_network_helpers::SendHandler sent_handler2;

  received_handler1 = [&, p_bandwidth2](const boost::system::error_code& ec,
                                        std::size_t length) {
    ASSERT_EQ(0, ec.value()) << "Error: " << ec.message();
    ASSERT_TRUE(tests::virtual_network_helpers::CheckBuffers(buffer2, r_buffer1,
                                                             length));
    tests::virtual_network_helpers::ResetBuffer(&r_buffer1, 0);

    count2 += length;

    if (count2 < data_size_to_send) {
      socket1.async_receive_from(boost::asio::buffer(r_buffer1),
                                 socket1_r_endpoint, received_handler1);
    } else {
      SSF_LOG(kLogTrace) << "   Received2 finished";
      ASSERT_EQ(data_size_to_send, count2);
      finished2.set_value(true);
    }
  };

  received_handler2 = [&, p_bandwidth1](const boost::system::error_code& ec,
                                        std::size_t length) {
    ASSERT_EQ(0, ec.value()) << "Error: " << ec.message();
    ASSERT_TRUE(tests::virtual_network_helpers::CheckBuffers(buffer1, r_buffer2,
                                                             length));
    tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0, length);

    count1 += length;

    if (count1 < data_size_to_send) {
      socket2.async_receive_from(boost::asio::buffer(r_buffer2),
                                 socket2_r_endpoint, received_handler2);
    } else {
      SSF_LOG(kLogTrace) << "   Received1 finished";
      ASSERT_EQ(data_size_to_send, count1);
      finished1.set_value(true);
    }
  };

  sent_handler1 = [&](const boost::system::error_code& ec, std::size_t length) {
    ASSERT_EQ(0, ec.value()) << "Error: " << ec.message();

    s_count1 += length;

    if (count1 < data_size_to_send) {
      socket1.async_send_to(boost::asio::buffer(buffer1), socket2_endpoint,
                            sent_handler1);
    } else {
      SSF_LOG(kLogTrace) << "   Packet loss1: "
                         << static_cast<double>(s_count1 - count1) /
                                DatagramProtocol::mtu;
      s_finished1.set_value(true);
    }
  };

  sent_handler2 = [&](const boost::system::error_code& ec, std::size_t length) {
    ASSERT_EQ(0, ec.value()) << "Error: " << ec.message();

    s_count2 += length;

    if (count2 < data_size_to_send) {
      socket2.async_send_to(boost::asio::buffer(buffer2), socket1_endpoint,
                            sent_handler2);
    } else {
      SSF_LOG(kLogTrace) << "   Packet loss2: "
                         << static_cast<double>(s_count2 - count2) /
                                DatagramProtocol::mtu;
      s_finished2.set_value(true);
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

  boost::system::error_code ec;

  socket1.async_receive_from(boost::asio::buffer(r_buffer1), socket1_r_endpoint,
                             received_handler1);
  socket2.async_receive_from(boost::asio::buffer(r_buffer2), socket2_r_endpoint,
                             received_handler2);

  p_bandwidth1->ResetTime();
  socket1.async_send_to(boost::asio::buffer(buffer1), socket2_endpoint,
                        sent_handler1);
  p_bandwidth1.reset();

  p_bandwidth2->ResetTime();
  socket2.async_send_to(boost::asio::buffer(buffer2), socket1_endpoint,
                        sent_handler2);
  p_bandwidth2.reset();

  std::vector<std::thread> threads;
  for (uint16_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    threads.emplace_back([&io_service]() { io_service.run(); });
  }

  s_finished1.get_future().wait();
  finished1.get_future().wait();
  s_finished2.get_future().wait();
  finished2.get_future().wait();

  socket1.close(ec);
  socket2.close(ec);

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

/// Connect socket1 to the endpoint defined by socket1_d_parameters
/// Bind socket2 to the endpoint defined by socket2_parameters
/// async_send, async_receive on socket1
/// async_send_to, async_receive_from on socket2
template <class DatagramProtocol>
void TestConnectionDatagramProtocol(
    ssf::layer::ParameterStack socket1_d_parameters,
    ssf::layer::ParameterStack socket2_parameters, uint64_t max_packets) {
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

  received_handler1 =
      [&](const boost::system::error_code& ec, std::size_t length) {
        ASSERT_EQ(0, ec.value())
            << "Receive handler should not be in error: " << ec.message();
        ASSERT_TRUE(tests::virtual_network_helpers::CheckBuffers(
            buffer2, r_buffer1, length));

        ++count1;
        if (count1 < max_packets) {
          socket1.async_send(boost::asio::buffer(buffer1), sent_handler1);
        } else {
          ASSERT_EQ(max_packets, count1);
          socket1.close();
        }
      };

  received_handler2 =
      [&](const boost::system::error_code& ec, std::size_t length) {
        ASSERT_EQ(0, ec.value())
            << "Receive handler should not be in error: " << ec.message();
        ASSERT_TRUE(tests::virtual_network_helpers::CheckBuffers(
            buffer1, r_buffer2, length));
        ASSERT_TRUE(socket1_endpoint == socket2_r_endpoint)
            << "Received endpoint from socket2 is not socket1 endpoint";
        ++count2;
        if (count2 < max_packets) {
          socket2.async_send_to(boost::asio::buffer(buffer2),
                                socket2_r_endpoint, sent_handler2);
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

  std::vector<std::thread> threads;
  for (uint16_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    threads.emplace_back([&io_service]() { io_service.run(); });
  }

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

template <class DatagramProtocol>
void TestBindSendLocalDatagramProtocol(
    ssf::layer::ParameterStack socket1_parameters,
    ssf::layer::ParameterStack socket2_parameters, uint64_t max_packets) {
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

  received_handler1 =
      [&](const boost::system::error_code& ec, std::size_t length) {
        ASSERT_EQ(0, ec.value())
            << "Receive handler should not be in error: " << ec.message();
        ASSERT_TRUE(tests::virtual_network_helpers::CheckBuffers(
            buffer2, r_buffer1, length));
        ++count1;
        if (count1 < max_packets) {
          socket1.async_send(boost::asio::buffer(buffer1), sent_handler1);
        } else {
          ASSERT_EQ(max_packets, count1);
          socket1.close();
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

  std::vector<std::thread> threads;
  for (uint16_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    threads.emplace_back([&io_service]() { io_service.run(); });
  }

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

#endif  // SSF_TESTS_DATAGRAM_PROTOCOL_HELPERS_H_
