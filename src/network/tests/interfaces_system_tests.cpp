#include <gtest/gtest.h>

#include <cstdint>

#include <future>
#include <vector>

#include <boost/asio/io_service.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "ssf/system/specific_interfaces_collection.h"
#include "ssf/system/system_interfaces.h"

#include "ssf/layer/physical/tcp.h"
#include "ssf/layer/physical/tlsotcp.h"
#include "ssf/layer/data_link/basic_circuit_protocol.h"
#include "ssf/layer/data_link/simple_circuit_policy.h"

#include "ssf/log/log.h"

class SystemTestFixture : public ::testing::Test {
 protected:
  SystemTestFixture()
      : ::testing::Test(),
        tcp_accept_filename_("./system/tcp_accept_config.json"),
        tcp_connect_filename_("./system/tcp_connect_config.json"),
        fail_tcp_accept_filename_("./system/fail_tcp_accept_config.json"),
        fail_tcp_connect_filename_("./system/fail_tcp_connect_config.json"),
        tlsotcp_accept_filename_("./system/tlsotcp_accept_config.json"),
        tlsotcp_connect_filename_("./system/tlsotcp_connect_config.json"),
        link_tcp_accept_filename_("./system/link_tcp_accept_config.json"),
        link_tcp_connect_filename_("./system/link_tcp_connect_config.json"),
        link_tlsotcp_accept_filename_(
            "./system/link_tlsotcp_accept_config.json"),
        link_tlsotcp_connect_filename_(
            "./system/link_tlsotcp_connect_config.json"),
        circuit_tlsotcp_accept1_filename_(
            "./system/circuit_tlsotcp_accept1_config.json"),
        circuit_tlsotcp_accept2_filename_(
            "./system/circuit_tlsotcp_accept2_config.json"),
        circuit_tlsotcp_accept3_filename_(
            "./system/circuit_tlsotcp_accept3_config.json"),
        circuit_tlsotcp_connect_filename_(
            "./system/circuit_tlsotcp_connect_config.json"),
        system_multiple_config_filename_(
            "./system/system_multiple_config.json"),
        system_reconnect_config_filename_(
            "./system/system_reconnect_config.json") {}

  virtual ~SystemTestFixture() {}

  virtual void SetUp() {}

  virtual void TearDown() {}

  std::string tcp_accept_filename_;
  std::string tcp_connect_filename_;
  std::string fail_tcp_accept_filename_;
  std::string fail_tcp_connect_filename_;
  std::string tlsotcp_accept_filename_;
  std::string tlsotcp_connect_filename_;
  std::string link_tcp_accept_filename_;
  std::string link_tcp_connect_filename_;
  std::string link_tlsotcp_accept_filename_;
  std::string link_tlsotcp_connect_filename_;
  std::string circuit_tlsotcp_accept1_filename_;
  std::string circuit_tlsotcp_accept2_filename_;
  std::string circuit_tlsotcp_accept3_filename_;
  std::string circuit_tlsotcp_connect_filename_;
  std::string system_multiple_config_filename_;
  std::string system_reconnect_config_filename_;
};

template <class Protocol>
void TestInterface(bool should_fail, const std::string& connect_filename,
                   const std::string& accept_filename,
                   const std::vector<std::string>& other_accepts_filenames) {
  boost::asio::io_service io_service;

  ssf::system::SpecificInterfacesCollection<Protocol> interfaces_collection;

  boost::property_tree::ptree accept_pt;
  boost::property_tree::ptree connect_pt;

  try {
    boost::property_tree::read_json(accept_filename, accept_pt);
    boost::property_tree::read_json(connect_filename, connect_pt);
  } catch (const std::exception&) {
    ASSERT_TRUE(false) << "Config file parsing failed";
    return;
  }

  for (auto& other_accept_filename : other_accepts_filenames) {
    boost::property_tree::ptree other_accept_pt;
    boost::property_tree::read_json(other_accept_filename, other_accept_pt);
    interfaces_collection.AsyncMount(io_service, other_accept_pt,
                                     [](const boost::system::error_code&,
                                        const std::string& interface_name) {});
  }

  std::promise<bool> accept_mounted;
  std::promise<bool> connect_mounted;

  std::string connect_interface_name = connect_pt.get_child("interface").data();
  std::string accept_interface_name = accept_pt.get_child("interface").data();

  interfaces_collection.AsyncMount(
      io_service, accept_pt,
      [&accept_mounted](const boost::system::error_code& ec,
                        const std::string& interface_name) {
        SSF_LOG("test", trace, " * Accept interface name {} ({})",
                interface_name, ec.message());
        accept_mounted.set_value(ec.value() == 0);
      });

  interfaces_collection.AsyncMount(
      io_service, connect_pt,
      [&connect_mounted](const boost::system::error_code& ec,
                         const std::string& interface_name) {
        SSF_LOG("test", trace, " * Connect interface name {} ({})",
                interface_name, ec.message());
        connect_mounted.set_value(ec.value() == 0);
      });

  std::vector<std::thread> threads;
  for (uint16_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    threads.emplace_back([&io_service]() { io_service.run(); });
  }

  bool connect_mounted_val = connect_mounted.get_future().get();
  bool accept_mounted_val = accept_mounted.get_future().get();
  if (should_fail) {
    EXPECT_FALSE(connect_mounted_val) << "Mount connect interface not failed";
    EXPECT_FALSE(accept_mounted_val) << "Mount accept interface not failed";
  } else {
    EXPECT_TRUE(connect_mounted_val) << "Fail mount connect interface";
    EXPECT_TRUE(accept_mounted_val) << "Fail mount accept interface";
  }

  interfaces_collection.UmountAll();
  
  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

TEST_F(SystemTestFixture, SimpleTCPSpecificInterfaces) {
  using TCPProtocol = ssf::layer::physical::TCPPhysicalLayer;

  TestInterface<TCPProtocol>(false, tcp_connect_filename_, tcp_accept_filename_,
                             std::vector<std::string>());
}

TEST_F(SystemTestFixture, FailSimpleTCPSpecificInterfaces) {
  using TCPProtocol = ssf::layer::physical::TCPPhysicalLayer;

  TestInterface<TCPProtocol>(true, fail_tcp_connect_filename_,
                             fail_tcp_accept_filename_,
                             std::vector<std::string>());
}

TEST_F(SystemTestFixture, TLSoTCPSpecificInterfaces) {
  using TLSoTCPProtocol =
      ssf::layer::physical::TLSboTCPPhysicalLayer;

  TestInterface<TLSoTCPProtocol>(false, tlsotcp_connect_filename_,
                                 tlsotcp_accept_filename_,
                                 std::vector<std::string>());
}

TEST_F(SystemTestFixture, LinkTCPSpecificInterfaces) {
  using TCPProtocol = ssf::layer::physical::TCPPhysicalLayer;
  using CircuitLinkProtocol =
      ssf::layer::data_link::basic_CircuitProtocol<
          TCPProtocol, ssf::layer::data_link::CircuitPolicy>;

  TestInterface<CircuitLinkProtocol>(false, link_tcp_connect_filename_,
                                     link_tcp_accept_filename_,
                                     std::vector<std::string>());
}

TEST_F(SystemTestFixture, LinkTLSoTCPSpecificInterfaces) {
  using TLSoTCPProtocol =
      ssf::layer::physical::TLSboTCPPhysicalLayer;
  using CircuitLinkProtocol =
      ssf::layer::data_link::basic_CircuitProtocol<
          TLSoTCPProtocol, ssf::layer::data_link::CircuitPolicy>;

  TestInterface<CircuitLinkProtocol>(false, link_tlsotcp_connect_filename_,
                                     link_tlsotcp_accept_filename_,
                                     std::vector<std::string>());
}

TEST_F(SystemTestFixture, CircuitTLSoTCPSpecificInterfaces) {
  using SimpleTLSLinkProtocol =
      ssf::layer::physical::TLSboTCPPhysicalLayer;
  using CircuitLinkProtocol =
      ssf::layer::data_link::basic_CircuitProtocol<
          SimpleTLSLinkProtocol,
          ssf::layer::data_link::CircuitPolicy>;
  std::vector<std::string> circuit;
  circuit.push_back(circuit_tlsotcp_accept1_filename_);
  circuit.push_back(circuit_tlsotcp_accept2_filename_);
  TestInterface<CircuitLinkProtocol>(false, circuit_tlsotcp_connect_filename_,
                                     circuit_tlsotcp_accept3_filename_,
                                     circuit);
}

TEST_F(SystemTestFixture, ImportHeterogeneousInterfaces) {
  using TCPProtocol = ssf::layer::physical::TCPPhysicalLayer;
  using TLSoTCPProtocol =
      ssf::layer::physical::TLSboTCPPhysicalLayer;
  using CircuitTCPProtocol =
      ssf::layer::data_link::basic_CircuitProtocol<
          TCPProtocol, ssf::layer::data_link::CircuitPolicy>;
  using CircuitTLSoTCPProtocol =
      ssf::layer::data_link::basic_CircuitProtocol<
          TLSoTCPProtocol, ssf::layer::data_link::CircuitPolicy>;

  boost::asio::io_service io_service;
  ssf::system::SystemInterfaces system_interfaces(io_service);

  system_interfaces.Start();

  ASSERT_TRUE(system_interfaces.RegisterInterfacesCollection<TCPProtocol>());
  ASSERT_TRUE(
      system_interfaces.RegisterInterfacesCollection<TLSoTCPProtocol>());
  ASSERT_TRUE(
      system_interfaces.RegisterInterfacesCollection<CircuitTCPProtocol>());
  ASSERT_TRUE(
      system_interfaces.RegisterInterfacesCollection<CircuitTLSoTCPProtocol>());

  ASSERT_FALSE(system_interfaces.RegisterInterfacesCollection<TCPProtocol>());
  ASSERT_FALSE(
      system_interfaces.RegisterInterfacesCollection<TLSoTCPProtocol>());
  ASSERT_FALSE(
      system_interfaces.RegisterInterfacesCollection<CircuitTCPProtocol>());
  ASSERT_FALSE(
      system_interfaces.RegisterInterfacesCollection<CircuitTLSoTCPProtocol>());

  std::promise<bool> finished;
  std::atomic<uint32_t> nb_interface_ups(0);
  bool ok = true;

  auto interface_up_handler = [&nb_interface_ups, &finished, &ok](
      const boost::system::error_code& ec, const std::string& interface_name) {
    SSF_LOG("test", trace, "interface {} up ({})", interface_name, ec.message());
    ok = ok && !ec;
    ++nb_interface_ups;
    if (nb_interface_ups.load() == 8) {
      finished.set_value(ok);
    }
  };

  system_interfaces.AsyncMount(system_multiple_config_filename_,
                               interface_up_handler);

  std::vector<std::thread> threads;
  for (uint16_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    threads.emplace_back([&io_service]() { io_service.run(); });
  }

  EXPECT_TRUE(finished.get_future().get()) << "All interfaces not mounted";
  system_interfaces.Stop();

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  system_interfaces.UnregisterAllInterfacesCollection();
}

/*TEST_F(SystemTestFixture, ReconnectHeterogeneousInterfaces) {
  using TCPProtocol = ssf::layer::physical::TCPPhysicalLayer;
  using InterfaceProtocol =
      ssf::layer::interface_layer::basic_InterfaceProtocol;

  boost::asio::io_service io_service;
  ssf::system::SystemInterfaces system_interfaces(io_service);

  system_interfaces.Start(1);

  ASSERT_TRUE(system_interfaces.RegisterInterfacesCollection<TCPProtocol>());

  std::promise<bool> finished;
  std::atomic<uint32_t> nb_interface_ups(0);

  boost::system::error_code ec;
  boost::asio::ip::tcp::acceptor acceptor(io_service);
  boost::asio::ip::tcp::socket socket(io_service);

  boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), 8000);
  boost::asio::ip::tcp::endpoint remote_ep;
  acceptor.open(ep.protocol(), ec);
  acceptor.bind(ep, ec);

  auto interface_up_handler = [](const boost::system::error_code& ec,
                                 const std::string& interface_name) {
    SSF_LOG("test", trace, "interface {} up ({})", interface_name, ec.message());
    if (!ec) {
      auto socket_optional =
          InterfaceProtocol::get_interface_manager().Find(interface_name);

      if (socket_optional) {
        int dummy_buf;
        (*socket_optional)
            ->async_receive(
                boost::asio::buffer(&dummy_buf, sizeof(dummy_buf)),
                [](const boost::system::error_code& ec, std::size_t length) {
                  ASSERT_NE(ec.value(), 0) << "Socket should be closed"
                                           << ec.message();
                });
      }
    }
  };

  std::function<void(const boost::system::error_code&)> accept_handler;

  accept_handler =
      [&socket, &remote_ep, &nb_interface_ups, &acceptor, &finished,
       &accept_handler](const boost::system::error_code& accept_ec) {
        if (!accept_ec) {
          ++nb_interface_ups;
          if (nb_interface_ups.load() < 5) {
            socket.close();

            acceptor.async_accept(socket, remote_ep, accept_handler);
          }
        }
        if (nb_interface_ups.load() == 5) {
          finished.set_value(true);
        }
      };

  acceptor.listen();

  acceptor.async_accept(socket, remote_ep, accept_handler);
  system_interfaces.AsyncMount(system_reconnect_config_filename_,
                               interface_up_handler);

  std::vector<std::thread> threads;
  for (uint16_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    threads.emplace_back([&io_service]() { io_service.run(); });
  }

  ASSERT_TRUE(finished.get_future().get()) << "All interfaces not mounted";

  system_interfaces.Stop();

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  system_interfaces.UnregisterAllInterfacesCollection();
}*/