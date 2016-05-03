#include <gtest/gtest.h>

#include "tests/interface_protocol_helpers.h"
#include "tests/interface_test_fixture.h"

#include "ssf/layer/parameters.h"

ssf::layer::LayerParameters tcp_acceptor_parameters = {{"port", "9000"}};

ssf::layer::LayerParameters tcp_client_parameters = {{"addr", "127.0.0.1"},
                                                          {"port", "9000"}};

TEST(InterfaceLayerTest, InterfaceSetup) {
  using SimpleLinkProtocol = ssf::layer::physical::TCPPhysicalLayer;

  boost::asio::io_service io_service;
  std::unique_ptr<boost::asio::io_service::work> p_work(
      new boost::asio::io_service::work(io_service));

  boost::thread_group threads;
  for (uint16_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    threads.create_thread([&io_service]() { io_service.run(); });
  }

  ssf::layer::ParameterStack simple2_parameters;
  simple2_parameters.push_back(tcp_acceptor_parameters);

  ssf::layer::ParameterStack simple1_parameters;
  simple1_parameters.push_back(tcp_client_parameters);

  tests::interface_protocol_helpers::InterfaceSetup<SimpleLinkProtocol>(
      io_service, {tcp_client_parameters}, "int1", {tcp_acceptor_parameters},
      "int2");

  p_work.reset();
  threads.join_all();
}

TEST_F(InterfaceTestFixture, Empty) {
  auto& interface_manager = tests::interface_protocol_helpers::
      InterfaceProtocol::get_interface_manager();

  ASSERT_EQ(interface_manager.Count("simple_lo1"), 1)
      << "simple_lo1 not found in interface manager";
  ASSERT_EQ(interface_manager.Count("simple_lo2"), 1)
      << "simple_lo2 not found in interface manager";
  ASSERT_EQ(interface_manager.Count("simple_tls_lo1"), 1)
      << "simple_tls_lo1 not found in interface manager";
  ASSERT_EQ(interface_manager.Count("simple_tls_lo2"), 1)
      << "simple_tls_lo2 not found in interface manager";
  ASSERT_EQ(interface_manager.Count("circuit_lo1"), 1)
      << "circuit_lo1 not found in interface manager";
  ASSERT_EQ(interface_manager.Count("circuit_lo2"), 1)
      << "circuit_lo2 not found in interface manager";
  ASSERT_EQ(interface_manager.Count("circuit_tls_lo1"), 1)
      << "circuit_tls_lo1 not found in interface manager";
  ASSERT_EQ(interface_manager.Count("circuit_tls_lo2"), 1)
      << "circuit_tls_lo2 not found in interface manager";
}

TEST_F(InterfaceTestFixture, SimpleLocalInterfaceTest) {
  tests::interface_protocol_helpers::TestInterfaceHalfDuplex<
      tests::interface_protocol_helpers::InterfaceProtocol>("simple_lo1",
                                                            "simple_lo2", 200);
  tests::interface_protocol_helpers::TestInterfaceFullDuplex<
      tests::interface_protocol_helpers::InterfaceProtocol>("simple_lo1",
                                                            "simple_lo2", 200);
}

TEST_F(InterfaceTestFixture, SimpleTLSLocalInterfaceTest) {
  tests::interface_protocol_helpers::TestInterfaceHalfDuplex<
      tests::interface_protocol_helpers::InterfaceProtocol>(
      "simple_tls_lo1", "simple_tls_lo2", 200);
  tests::interface_protocol_helpers::TestInterfaceFullDuplex<
      tests::interface_protocol_helpers::InterfaceProtocol>(
      "simple_tls_lo1", "simple_tls_lo2", 200);
}

TEST_F(InterfaceTestFixture, CircuitLocalInterfaceTest) {
  tests::interface_protocol_helpers::TestInterfaceHalfDuplex<
      tests::interface_protocol_helpers::InterfaceProtocol>("circuit_lo1",
                                                            "circuit_lo2", 200);
  tests::interface_protocol_helpers::TestInterfaceFullDuplex<
      tests::interface_protocol_helpers::InterfaceProtocol>("circuit_lo1",
                                                            "circuit_lo2", 200);
}

TEST_F(InterfaceTestFixture, CircuitTLSLocalInterfaceTest) {
  tests::interface_protocol_helpers::TestInterfaceHalfDuplex<
      tests::interface_protocol_helpers::InterfaceProtocol>(
      "simple_tls_lo1", "simple_tls_lo2", 200);
  tests::interface_protocol_helpers::TestInterfaceFullDuplex<
      tests::interface_protocol_helpers::InterfaceProtocol>(
      "simple_tls_lo1", "simple_tls_lo2", 200);
}
