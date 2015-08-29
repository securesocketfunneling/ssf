#include <gtest/gtest.h>

#include <cstdint>

#include <future>
#include <vector>

#include "core/system/specific_interfaces_collection.h"
#include "core/system/system_interfaces.h"

#include "core/virtual_network/physical_layer/tcp.h"
#include "core/virtual_network/physical_layer/tlsotcp.h"
#include "core/virtual_network/data_link_layer/basic_circuit_protocol.h"
#include "core/virtual_network/data_link_layer/simple_circuit_policy.h"

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class SystemTestFixture : public ::testing::Test {
 protected:
  SystemTestFixture()
      : tcp_accept_filename_("system/tcp_accept_config.json"),
        tcp_connect_filename_("system/tcp_connect_config.json"),
        fail_tcp_accept_filename_("system/fail_tcp_accept_config.json"),
        fail_tcp_connect_filename_("system/fail_tcp_connect_config.json"),
        tlsotcp_accept_filename_("system/tlsotcp_accept_config.json"),
        tlsotcp_connect_filename_("system/tlsotcp_connect_config.json"),
        link_tcp_accept_filename_("system/link_tcp_accept_config.json"),
        link_tcp_connect_filename_("system/link_tcp_connect_config.json"),
        link_tlsotcp_accept_filename_("system/link_tlsotcp_accept_config.json"),
        link_tlsotcp_connect_filename_("system/link_tlsotcp_connect_config.json"),
        circuit_tlsotcp_accept1_filename_(
            "system/circuit_tlsotcp_accept1_config.json"),
        circuit_tlsotcp_accept2_filename_(
            "system/circuit_tlsotcp_accept2_config.json"),
        circuit_tlsotcp_accept3_filename_(
            "system/circuit_tlsotcp_accept3_config.json"),
        circuit_tlsotcp_connect_filename_(
            "system/circuit_tlsotcp_connect_config.json"),
        system_multiple_config_filename_("system/system_multiple_config.json") {}

  ~SystemTestFixture() {}

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
        BOOST_LOG_TRIVIAL(trace) << "accept interface name : " << interface_name
                                 << " " << ec.message();
        accept_mounted.set_value(ec.value() == 0);
      });

  interfaces_collection.AsyncMount(
      io_service, connect_pt,
      [&connect_mounted](const boost::system::error_code& ec,
                         const std::string& interface_name) {
        BOOST_LOG_TRIVIAL(trace)
            << "connect interface name : " << interface_name << " "
            << ec.message();
        connect_mounted.set_value(ec.value() == 0);
      });

  boost::thread_group threads;
  for (uint16_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    threads.create_thread([&io_service]() { io_service.run(); });
  }

  bool connect_mounted_val = connect_mounted.get_future().get();
  bool accept_mounted_val = accept_mounted.get_future().get();
  if (should_fail) {
    ASSERT_FALSE(connect_mounted_val) << "Mount connect interface not failed";
    ASSERT_FALSE(accept_mounted_val) << "Mount accept interface not failed";
  } else {
    ASSERT_TRUE(connect_mounted_val) << "Fail mount connect interface";
    ASSERT_TRUE(accept_mounted_val) << "Fail mount accept interface";
  }

  interfaces_collection.UmountAll();

  threads.join_all();
}

TEST_F(SystemTestFixture, SimpleTCPSpecificInterfaces) {
  using TCPProtocol = virtual_network::physical_layer::TCPPhysicalLayer;

  TestInterface<TCPProtocol>(false, tcp_connect_filename_, tcp_accept_filename_,
                             std::vector<std::string>());
}

TEST_F(SystemTestFixture, FailSimpleTCPSpecificInterfaces) {
  using TCPProtocol = virtual_network::physical_layer::TCPPhysicalLayer;

  TestInterface<TCPProtocol>(true, fail_tcp_connect_filename_,
                             fail_tcp_accept_filename_,
                             std::vector<std::string>());
}

TEST_F(SystemTestFixture, TLSoTCPSpecificInterfaces) {
  using TLSoTCPProtocol =
      virtual_network::physical_layer::TLSboTCPPhysicalLayer;

  TestInterface<TLSoTCPProtocol>(false, tlsotcp_connect_filename_,
                                 tlsotcp_accept_filename_,
                                 std::vector<std::string>());
}

TEST_F(SystemTestFixture, LinkTCPSpecificInterfaces) {
  using TCPProtocol = virtual_network::physical_layer::TCPPhysicalLayer;
  using CircuitLinkProtocol =
      virtual_network::data_link_layer::basic_CircuitProtocol<
          TCPProtocol, virtual_network::data_link_layer::CircuitPolicy>;

  TestInterface<CircuitLinkProtocol>(false, link_tcp_connect_filename_,
                                     link_tcp_accept_filename_,
                                     std::vector<std::string>());
}

TEST_F(SystemTestFixture, LinkTLSoTCPSpecificInterfaces) {
  using TLSoTCPProtocol =
      virtual_network::physical_layer::TLSboTCPPhysicalLayer;
  using CircuitLinkProtocol =
      virtual_network::data_link_layer::basic_CircuitProtocol<
          TLSoTCPProtocol, virtual_network::data_link_layer::CircuitPolicy>;

  TestInterface<CircuitLinkProtocol>(false, link_tlsotcp_connect_filename_,
                                     link_tlsotcp_accept_filename_,
                                     std::vector<std::string>());
}

TEST_F(SystemTestFixture, CircuitTLSoTCPSpecificInterfaces) {
  using SimpleTLSLinkProtocol =
      virtual_network::physical_layer::TLSboTCPPhysicalLayer;
  using CircuitLinkProtocol =
      virtual_network::data_link_layer::basic_CircuitProtocol<
          SimpleTLSLinkProtocol,
          virtual_network::data_link_layer::CircuitPolicy>;
  std::vector<std::string> circuit;
  circuit.push_back(circuit_tlsotcp_accept1_filename_);
  circuit.push_back(circuit_tlsotcp_accept2_filename_);
  TestInterface<CircuitLinkProtocol>(false, circuit_tlsotcp_connect_filename_,
                                     circuit_tlsotcp_accept3_filename_,
                                     circuit);
}

TEST_F(SystemTestFixture, ImportHeterogeneousInterfaces) {
  using TCPProtocol = virtual_network::physical_layer::TCPPhysicalLayer;
  using TLSoTCPProtocol =
      virtual_network::physical_layer::TLSboTCPPhysicalLayer;
  using CircuitTCPProtocol =
      virtual_network::data_link_layer::basic_CircuitProtocol<
          TCPProtocol, virtual_network::data_link_layer::CircuitPolicy>;
  using CircuitTLSoTCPProtocol =
      virtual_network::data_link_layer::basic_CircuitProtocol<
          TLSoTCPProtocol, virtual_network::data_link_layer::CircuitPolicy>;

  ssf::system::SystemInterfaces system_interfaces;

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

  auto interface_up_handler = [&nb_interface_ups, &finished](
      const boost::system::error_code& ec, const std::string& interface_name) {
    BOOST_LOG_TRIVIAL(trace) << "interface " << interface_name
                             << " up : " << ec.message();
    if (!ec) {
      ++nb_interface_ups;
      if (nb_interface_ups.load() == 8) {
        finished.set_value(true);
      }
    }
  };

  system_interfaces.AsyncMount(system_multiple_config_filename_,
                               interface_up_handler);

  finished.get_future().get();
  system_interfaces.Stop();
  system_interfaces.UnregisterAllInterfacesCollection();
}
