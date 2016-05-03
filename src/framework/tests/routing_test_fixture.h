#ifndef SSF_TESTS_ROUTING_TEST_FIXTURE_H_
#define SSF_TESTS_ROUTING_TEST_FIXTURE_H_

#include <gtest/gtest.h>

#include <memory>

#include <boost/asio/io_service.hpp>

#include <boost/thread.hpp>

#include "ssf/layer/parameters.h"

#include "ssf/layer/network/basic_network_protocol.h"
#include "ssf/layer/routing/basic_router.h"
#include "ssf/layer/routing/basic_routed_protocol.h"

#include "tests/interface_test_fixture.h"

class RoutingTestFixture : public InterfaceTestFixture {
 protected:
  using NetworkProtocol =
      ssf::layer::network::basic_NetworkProtocol<InterfaceProtocol>;
  using RoutedProtocol =
      ssf::layer::routing::basic_RoutedProtocol<NetworkProtocol>;

 protected:
  RoutingTestFixture()
      : InterfaceTestFixture(),
        io_service_(),
        resolver_(io_service_),
        threads_(),
        p_work_(new boost::asio::io_service::work(io_service_)),
        p_router1_(nullptr),
        p_router2_(nullptr) {}

  virtual ~RoutingTestFixture() {
    boost::system::error_code ec;
    RoutedProtocol::StopAllRouters();
  }

  virtual void SetUp() {
    RoutedProtocol::StartRouter("router1", io_service_);
    RoutedProtocol::StartRouter("router2", io_service_);

    p_router1_ = RoutedProtocol::get_router("router1");
    p_router2_ = RoutedProtocol::get_router("router2");

    InterfaceTestFixture::SetUp();

    boost::system::error_code ec;

    {
      ssf::layer::LayerParameters socket1_network_parameters;
      socket1_network_parameters["network_id"] = "1";
      ssf::layer::LayerParameters socket1_interface_parameters;
      socket1_interface_parameters["interface_id"] = "simple_lo1";
      ssf::layer::ParameterStack parameters1;
      parameters1.push_back(socket1_network_parameters);
      parameters1.push_back(socket1_interface_parameters);
      auto socket1_endpoint_it = resolver_.resolve(parameters1, ec);
      NetworkProtocol::endpoint socket1_local_endpoint(*socket1_endpoint_it);
      uint32_t prefix1 = 1;
      p_router1_->add_network(prefix1, socket1_local_endpoint, ec);
    }
    {
      ssf::layer::LayerParameters socket1_network_parameters;
      socket1_network_parameters["network_id"] = "2";
      ssf::layer::LayerParameters socket1_interface_parameters;
      socket1_interface_parameters["interface_id"] = "simple_lo2";
      ssf::layer::ParameterStack parameters1;
      parameters1.push_back(socket1_network_parameters);
      parameters1.push_back(socket1_interface_parameters);
      auto socket1_endpoint_it = resolver_.resolve(parameters1, ec);
      NetworkProtocol::endpoint socket1_local_endpoint(*socket1_endpoint_it);
      uint32_t prefix1 = 2;
      p_router2_->add_network(prefix1, socket1_local_endpoint, ec);
    }
    {
      ssf::layer::LayerParameters socket1_network_parameters;
      socket1_network_parameters["network_id"] = "3";
      ssf::layer::LayerParameters socket1_interface_parameters;
      socket1_interface_parameters["interface_id"] = "simple_tls_lo1";
      ssf::layer::ParameterStack parameters1;
      parameters1.push_back(socket1_network_parameters);
      parameters1.push_back(socket1_interface_parameters);
      auto socket1_endpoint_it = resolver_.resolve(parameters1, ec);
      NetworkProtocol::endpoint socket1_local_endpoint(*socket1_endpoint_it);
      uint32_t prefix1 = 3;
      p_router1_->add_network(prefix1, socket1_local_endpoint, ec);
    }
    {
      ssf::layer::LayerParameters socket1_network_parameters;
      socket1_network_parameters["network_id"] = "4";
      ssf::layer::LayerParameters socket1_interface_parameters;
      socket1_interface_parameters["interface_id"] = "simple_tls_lo2";
      ssf::layer::ParameterStack parameters1;
      parameters1.push_back(socket1_network_parameters);
      parameters1.push_back(socket1_interface_parameters);
      auto socket1_endpoint_it = resolver_.resolve(parameters1, ec);
      NetworkProtocol::endpoint socket1_local_endpoint(*socket1_endpoint_it);
      uint32_t prefix1 = 4;
      p_router2_->add_network(prefix1, socket1_local_endpoint, ec);
    }
    {
      ssf::layer::LayerParameters socket1_network_parameters;
      socket1_network_parameters["network_id"] = "5";
      ssf::layer::LayerParameters socket1_interface_parameters;
      socket1_interface_parameters["interface_id"] = "circuit_lo1";
      ssf::layer::ParameterStack parameters1;
      parameters1.push_back(socket1_network_parameters);
      parameters1.push_back(socket1_interface_parameters);
      auto socket1_endpoint_it = resolver_.resolve(parameters1, ec);
      NetworkProtocol::endpoint socket1_local_endpoint(*socket1_endpoint_it);
      uint32_t prefix1 = 5;
      p_router1_->add_network(prefix1, socket1_local_endpoint, ec);
    }
    {
      ssf::layer::LayerParameters socket1_network_parameters;
      socket1_network_parameters["network_id"] = "6";
      ssf::layer::LayerParameters socket1_interface_parameters;
      socket1_interface_parameters["interface_id"] = "circuit_lo2";
      ssf::layer::ParameterStack parameters1;
      parameters1.push_back(socket1_network_parameters);
      parameters1.push_back(socket1_interface_parameters);
      auto socket1_endpoint_it = resolver_.resolve(parameters1, ec);
      NetworkProtocol::endpoint socket1_local_endpoint(*socket1_endpoint_it);
      uint32_t prefix1 = 6;
      p_router2_->add_network(prefix1, socket1_local_endpoint, ec);
    }
    {
      ssf::layer::LayerParameters socket1_network_parameters;
      socket1_network_parameters["network_id"] = "7";
      ssf::layer::LayerParameters socket1_interface_parameters;
      socket1_interface_parameters["interface_id"] = "circuit_tls_lo1";
      ssf::layer::ParameterStack parameters1;
      parameters1.push_back(socket1_network_parameters);
      parameters1.push_back(socket1_interface_parameters);
      auto socket1_endpoint_it = resolver_.resolve(parameters1, ec);
      NetworkProtocol::endpoint socket1_local_endpoint(*socket1_endpoint_it);
      uint32_t prefix1 = 7;
      p_router1_->add_network(prefix1, socket1_local_endpoint, ec);
    }
    {
      ssf::layer::LayerParameters socket1_network_parameters;
      socket1_network_parameters["network_id"] = "8";
      ssf::layer::LayerParameters socket1_interface_parameters;
      socket1_interface_parameters["interface_id"] = "circuit_tls_lo2";
      ssf::layer::ParameterStack parameters1;
      parameters1.push_back(socket1_network_parameters);
      parameters1.push_back(socket1_interface_parameters);
      auto socket1_endpoint_it = resolver_.resolve(parameters1, ec);
      NetworkProtocol::endpoint socket1_local_endpoint(*socket1_endpoint_it);
      uint32_t prefix1 = 8;
      p_router2_->add_network(prefix1, socket1_local_endpoint, ec);
    }

    //------------------------- Set routing table --------------------------//
    {
      boost::system::error_code ec;
      p_router1_->add_route(2, 7, ec);
      p_router1_->add_route(4, 1, ec);
      p_router1_->add_route(6, 3, ec);
      p_router1_->add_route(8, 5, ec);

      p_router2_->add_route(1, 4, ec);
      p_router2_->add_route(3, 6, ec);
      p_router2_->add_route(5, 8, ec);
      p_router2_->add_route(7, 2, ec);
    }

    //------------------------- run io_service --------------------------//
    for (uint16_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
      threads_.create_thread([this]() { this->io_service_.run(); });
    }

    Wait();
  }

  virtual void TearDown() {
    boost::system::error_code ec;
    p_router1_->remove_network(1, ec);
    p_router1_->remove_network(3, ec);
    p_router1_->remove_network(5, ec);
    p_router1_->remove_network(7, ec);
    p_router2_->remove_network(2, ec);
    p_router2_->remove_network(4, ec);
    p_router2_->remove_network(6, ec);
    p_router2_->remove_network(8, ec);

    p_router1_->remove_route(2, ec);
    p_router1_->remove_route(4, ec);
    p_router1_->remove_route(6, ec);
    p_router1_->remove_route(8, ec);
    p_router2_->remove_route(1, ec);
    p_router2_->remove_route(3, ec);
    p_router2_->remove_route(5, ec);
    p_router2_->remove_route(7, ec);

    p_router1_->close(ec);
    p_router2_->close(ec);

    p_work_.reset();
    threads_.join_all();

    InterfaceTestFixture::TearDown();
  }

 private:
  bool Wait() { return true; }

 protected:
  boost::asio::io_service io_service_;

  NetworkProtocol::resolver resolver_;

  boost::thread_group threads_;
  std::unique_ptr<boost::asio::io_service::work> p_work_;

  std::shared_ptr<typename RoutedProtocol::router_type> p_router1_;
  std::shared_ptr<typename RoutedProtocol::router_type> p_router2_;
};

#endif  // SSF_TESTS_ROUTING_TEST_FIXTURE_H_
