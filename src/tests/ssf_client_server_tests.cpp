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
  SSFClientServerTest() : p_ssf_client_(nullptr), p_ssf_server_(nullptr) {}

  ~SSFClientServerTest() {}

  virtual void SetUp() {
    StartServer();
    StartClient();
  }

  virtual void TearDown() {
    p_ssf_client_->Stop();
    p_ssf_server_->Stop();
  }

  void StartServer() {
    ssf::Config ssf_config;

    auto endpoint_query =
        ssf::network::GenerateServerQuery("", "8000", ssf_config);
    p_ssf_server_.reset(new Server());

    boost::system::error_code run_ec;
    p_ssf_server_->Run(endpoint_query, run_ec);
  }

  void StartClient() {
    std::vector<BaseUserServicePtr> client_options;

    ssf::Config ssf_config;

    auto endpoint_query =
        ssf::network::GenerateClientQuery("127.0.0.1", "8000", ssf_config, {});

    p_ssf_client_.reset(new Client(
        client_options, boost::bind(&SSFClientServerTest::SSFClientCallback,
                                    this, _1, _2, _3)));

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
