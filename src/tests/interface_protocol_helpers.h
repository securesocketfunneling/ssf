#ifndef SSF_TESTS_INTERFACE_PROTOCOL_HELPERS_H_
#define SSF_TESTS_INTERFACE_PROTOCOL_HELPERS_H_

#include <cstdint>

#include <future>
#include <vector>

#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include "core/virtual_network/parameters.h"

#include "core/virtual_network/interface_layer/basic_interface_protocol.h"
#include "core/virtual_network/interface_layer/basic_interface.h"

#include "tests/tools.h"
#include "tests/virtual_network_helpers.h"

namespace tests {
namespace interface_protocol_helpers {

using InterfaceProtocol =
    virtual_network::interface_layer::basic_InterfaceProtocol;

template <class NextLayer>
using Interface =
    virtual_network::interface_layer::basic_Interface<InterfaceProtocol,
                                                      NextLayer>;

template <class InterfaceProtocol>
void TestInterfaceHalfDuplex(const std::string& interface_1,
                             const std::string& interface_2,
                             uint64_t size_in_mtu_multiples) {
  std::cout << "  * TestInterfaceHalfDuplux" << std::endl;
  using Buffer = std::vector<uint8_t>;

  std::promise<bool> finished1;

  uint64_t data_size_to_send = InterfaceProtocol::mtu * size_in_mtu_multiples;

  auto p_bandwidth1 =
      std::make_shared<ScopedBandWidth>(data_size_to_send * 8ULL);

  uint64_t s_count1 = 0;
  uint64_t count1 = 0;

  auto p_socket1 =
      InterfaceProtocol::get_interface_manager().available_sockets[interface_1];
  auto p_socket2 =
      InterfaceProtocol::get_interface_manager().available_sockets[interface_2];

  Buffer buffer1(InterfaceProtocol::mtu);
  Buffer r_buffer2(InterfaceProtocol::mtu);
  tests::virtual_network_helpers::ResetBuffer(&buffer1, 1);
  tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0);

  tests::virtual_network_helpers::SendHandler sent_handler1;
  tests::virtual_network_helpers::ReceiveHandler received_handler2;

  received_handler2 =
      [&](const boost::system::error_code& ec, std::size_t length) {
        ASSERT_EQ(0, ec.value()) << "Error: " << ec.message();
        ASSERT_TRUE(tests::virtual_network_helpers::CheckBuffers(
            buffer1, r_buffer2, length));
        tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0, length);

        count1 += length;

        if (count1 < data_size_to_send) {
          p_socket2->async_receive(boost::asio::buffer(r_buffer2),
                                   received_handler2);
        } else {
          ASSERT_EQ(data_size_to_send, count1);
          finished1.set_value(true);
        }
      };

  sent_handler1 = [&, p_bandwidth1](const boost::system::error_code& ec,
                                    std::size_t length) {
    ASSERT_EQ(0, ec.value()) << "Error: " << ec.message();

    s_count1 += length;

    if (s_count1 < data_size_to_send) {
      if (data_size_to_send - s_count1 >= buffer1.size()) {
        p_socket1->async_send(boost::asio::buffer(buffer1), sent_handler1);
      } else {
        p_socket1->async_send(
            boost::asio::buffer(&buffer1[0], static_cast<std::size_t>(
                                                 data_size_to_send - s_count1)),
            sent_handler1);
      }
    }
  };

  boost::system::error_code ec;

  p_socket2->async_receive(boost::asio::buffer(r_buffer2), received_handler2);

  p_bandwidth1->ResetTime();
  if (data_size_to_send >= buffer1.size()) {
    p_socket1->async_send(boost::asio::buffer(buffer1), sent_handler1);
  } else {
    p_socket1->async_send(
        boost::asio::buffer(&buffer1[0],
                            static_cast<std::size_t>(data_size_to_send)),
        sent_handler1);
  }
  p_bandwidth1.reset();

  finished1.get_future().wait();
}

template <class InterfaceProtocol>
void TestInterfaceFullDuplex(const std::string& interface_1,
                             const std::string& interface_2,
                             uint64_t size_in_mtu_multiples) {
  std::cout << "  * TestInterfaceFullDuplex" << std::endl;
  using Buffer = std::vector<uint8_t>;

  std::promise<bool> finished1;
  std::promise<bool> finished2;

  uint64_t data_size_to_send = InterfaceProtocol::mtu * size_in_mtu_multiples;

  auto p_bandwidth1 =
      std::make_shared<ScopedBandWidth>(data_size_to_send * 8ULL);
  auto p_bandwidth2 =
      std::make_shared<ScopedBandWidth>(data_size_to_send * 8ULL);

  uint64_t s_count1 = 0;
  uint64_t count1 = 0;
  uint64_t s_count2 = 0;
  uint64_t count2 = 0;

  auto p_socket1 =
      InterfaceProtocol::get_interface_manager().available_sockets[interface_1];
  auto p_socket2 =
      InterfaceProtocol::get_interface_manager().available_sockets[interface_2];

  Buffer buffer1(InterfaceProtocol::mtu);
  Buffer buffer2(InterfaceProtocol::mtu);
  Buffer r_buffer1(InterfaceProtocol::mtu);
  Buffer r_buffer2(InterfaceProtocol::mtu);
  tests::virtual_network_helpers::ResetBuffer(&buffer1, 1);
  tests::virtual_network_helpers::ResetBuffer(&buffer2, 1);
  tests::virtual_network_helpers::ResetBuffer(&r_buffer1, 0);
  tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0);

  tests::virtual_network_helpers::SendHandler sent_handler1;
  tests::virtual_network_helpers::SendHandler sent_handler2;
  tests::virtual_network_helpers::ReceiveHandler received_handler1;
  tests::virtual_network_helpers::ReceiveHandler received_handler2;

  received_handler1 =
      [&](const boost::system::error_code& ec, std::size_t length) {
        ASSERT_EQ(0, ec.value()) << "Error: " << ec.message();
        ASSERT_TRUE(tests::virtual_network_helpers::CheckBuffers(
            buffer2, r_buffer1, length));
        tests::virtual_network_helpers::ResetBuffer(&r_buffer1, 0);

        count2 += length;

        if (count2 < data_size_to_send) {
          p_socket1->async_receive(boost::asio::buffer(r_buffer1),
                                   received_handler1);
        } else {
          ASSERT_EQ(data_size_to_send, count2);
          finished2.set_value(true);
        }
      };

  received_handler2 =
      [&](const boost::system::error_code& ec, std::size_t length) {
        ASSERT_EQ(0, ec.value()) << "Error: " << ec.message();
        ASSERT_TRUE(tests::virtual_network_helpers::CheckBuffers(
            buffer1, r_buffer2, length));
        tests::virtual_network_helpers::ResetBuffer(&r_buffer2, 0, length);

        count1 += length;

        if (count1 < data_size_to_send) {
          p_socket2->async_receive(boost::asio::buffer(r_buffer2),
                                   received_handler2);
        } else {
          ASSERT_EQ(data_size_to_send, count1);
          finished1.set_value(true);
        }
      };

  sent_handler1 = [&, p_bandwidth1](const boost::system::error_code& ec,
                                    std::size_t length) {
    ASSERT_EQ(0, ec.value()) << "Error: " << ec.message();

    s_count1 += length;

    if (s_count1 < data_size_to_send) {
      if (data_size_to_send - s_count1 >= buffer1.size()) {
        p_socket1->async_send(boost::asio::buffer(buffer1), sent_handler1);
      } else {
        p_socket1->async_send(
            boost::asio::buffer(&buffer1[0], static_cast<std::size_t>(
                                                 data_size_to_send - s_count1)),
            sent_handler1);
      }
    }
  };

  sent_handler2 = [&, p_bandwidth2](const boost::system::error_code& ec,
                                    std::size_t length) {
    ASSERT_EQ(0, ec.value()) << "Error: " << ec.message();

    s_count2 += length;

    if (s_count2 < data_size_to_send) {
      if (data_size_to_send - s_count2 >= buffer2.size()) {
        p_socket2->async_send(boost::asio::buffer(buffer2), sent_handler2);
      } else {
        p_socket2->async_send(
            boost::asio::buffer(&buffer2[0], static_cast<std::size_t>(
                                                 data_size_to_send - s_count2)),
            sent_handler2);
      }
    }
  };

  boost::system::error_code ec;

  p_socket1->async_receive(boost::asio::buffer(r_buffer1), received_handler1);
  p_socket2->async_receive(boost::asio::buffer(r_buffer2), received_handler2);

  p_bandwidth1->ResetTime();
  if (data_size_to_send >= buffer1.size()) {
    p_socket1->async_send(boost::asio::buffer(buffer1), sent_handler1);
  } else {
    p_socket1->async_send(
        boost::asio::buffer(&buffer1[0],
                            static_cast<std::size_t>(data_size_to_send)),
        sent_handler1);
  }
  p_bandwidth1.reset();

  p_bandwidth2->ResetTime();
  if (data_size_to_send >= buffer2.size()) {
    p_socket2->async_send(boost::asio::buffer(buffer2), sent_handler2);
  } else {
    p_socket2->async_send(
        boost::asio::buffer(&buffer2[0],
                            static_cast<std::size_t>(data_size_to_send)),
        sent_handler2);
  }
  p_bandwidth2.reset();

  finished1.get_future().wait();
  finished2.get_future().wait();
}

template <class NextLayer>
std::pair<Interface<NextLayer>, Interface<NextLayer>> InterfaceSetup(
    boost::asio::io_service& io_service,
    const virtual_network::ParameterStack& parameters1,
    const std::string& interface1_name,
    const virtual_network::ParameterStack& parameters2,
    const std::string& interface2_name) {
  using SimpleInterface = Interface<NextLayer>;

  typename NextLayer::resolver simple_link_resolver(io_service);
  boost::system::error_code ec;

  auto simple2_local_endpoint_it =
      simple_link_resolver.resolve(parameters2, ec);
  typename NextLayer::endpoint simple2_local_endpoint(
      *simple2_local_endpoint_it);

  auto simple1_local_endpoint_it =
      simple_link_resolver.resolve(parameters1, ec);
  typename NextLayer::endpoint simple1_local_endpoint(
      *simple1_local_endpoint_it);

  SimpleInterface interface1(io_service);
  SimpleInterface interface2(io_service);

  std::promise<boost::system::error_code> connect_done;
  std::promise<boost::system::error_code> accept_done;

  auto connected =
      [&](const boost::system::error_code& ec) { connect_done.set_value(ec); };

  auto accepted =
      [&](const boost::system::error_code& ec) { accept_done.set_value(ec); };

  interface1.async_accept("int1", simple2_local_endpoint, accepted);
  interface2.async_connect("int2", simple1_local_endpoint, connected);

  auto connect_ec = connect_done.get_future().get();
  auto accept_ec = accept_done.get_future().get();

  EXPECT_EQ(true, !connect_ec && !accept_ec);

  return std::make_pair(std::move(interface1), std::move(interface2));
}

}  // interface_protocol_helpers
}  // tests

#endif  // SSF_TESTS_INTERFACE_PROTOCOL_HELPERS_H_
