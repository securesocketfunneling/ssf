#include <vector>
#include <functional>
#include <array>
#include <future>
#include <list>

#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "common/config/config.h"

#include "core/network_protocol.h"
#include "core/client/client.h"
#include "core/server/server.h"

#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/initialisation.h"
#include "services/user_services/udp_port_forwarding.h"

class SSFClientServerTest : public ::testing::Test {
 public:
  using Client =
      ssf::SSFClient<ssf::network::Protocol, ssf::TransportProtocolPolicy>;
  using Server =
      ssf::SSFServer<ssf::network::Protocol, ssf::TransportProtocolPolicy>;

  using demux = Client::demux;

  using BaseUserServicePtr =
      ssf::services::BaseUserService<demux>::BaseUserServicePtr;

 public:
  SSFClientServerTest()
      : client_io_service_(),
        p_client_worker_(new boost::asio::io_service::work(client_io_service_)),
        server_io_service_(),
        p_server_worker_(new boost::asio::io_service::work(server_io_service_)),
        p_ssf_client_(nullptr),
        p_ssf_server_(nullptr) {}

  ~SSFClientServerTest() {}

  virtual void SetUp() {
    StartServer();
    StartClient();
  }

  virtual void TearDown() {
    StopClientThreads();
    StopServerThreads();
  }

  void StartServer() {
    ssf::Config ssf_config;

    uint16_t port = 8000;
    auto endpoint_query =
        ssf::network::GenerateServerQuery(std::to_string(port), ssf_config);
    p_ssf_server_.reset(new Server(server_io_service_, ssf_config, port));

    StartServerThreads();
    boost::system::error_code run_ec;
    p_ssf_server_->Run(endpoint_query, run_ec);
  }

  void StartClient() {

    std::vector<BaseUserServicePtr> client_options;

    ssf::Config ssf_config;

    auto endpoint_query =
        ssf::network::GenerateClientQuery("127.0.0.1", "8000", ssf_config, {});

    p_ssf_client_.reset(
        new Client(client_io_service_, client_options,
                   boost::bind(&SSFClientServerTest::SSFClientCallback, this,
                               _1, _2, _3)));
    StartClientThreads();
    boost::system::error_code run_ec;
    p_ssf_client_->Run(endpoint_query, run_ec);
  }

  bool Wait() {
    auto network_set_future = network_set_.get_future();
    auto transport_set_future = transport_set_.get_future();

    network_set_future.wait();
    transport_set_future.wait();

    return network_set_future.get() && transport_set_future.get();
  }

  void StartServerThreads() {
    for (uint8_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
      server_threads_.create_thread([&]() { server_io_service_.run(); });
    }
  }

  void StartClientThreads() {
    for (uint8_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
      client_threads_.create_thread([&]() { client_io_service_.run(); });
    }
  }

  void StopServerThreads() {
    p_ssf_server_->Stop();
    p_server_worker_.reset();
    server_threads_.join_all();
    server_io_service_.stop();
  }

  void StopClientThreads() {
    p_ssf_client_->Stop();
    p_client_worker_.reset();
    client_threads_.join_all();
    client_io_service_.stop();
  }

  void SSFClientCallback(ssf::services::initialisation::type type,
                         BaseUserServicePtr p_user_service,
                         const boost::system::error_code& ec) {
    if (type == ssf::services::initialisation::NETWORK) {
      network_set_.set_value(!ec);
      if (ec) {
        transport_set_.set_value(false);
      }

      return;
    }

    if (type == ssf::services::initialisation::TRANSPORT) {
      transport_set_.set_value(!ec);

      return;
    }
  }

 protected:
  boost::asio::io_service client_io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_client_worker_;
  boost::thread_group client_threads_;

  boost::asio::io_service server_io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_server_worker_;
  boost::thread_group server_threads_;
  std::unique_ptr<Client> p_ssf_client_;
  std::unique_ptr<Server> p_ssf_server_;

  std::promise<bool> network_set_;
  std::promise<bool> transport_set_;
};

//-----------------------------------------------------------------------------
TEST_F(SSFClientServerTest, connectDisconnect) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::debug);

  ASSERT_TRUE(Wait());
}
