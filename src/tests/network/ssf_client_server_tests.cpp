#include <vector>
#include <functional>
#include <array>
#include <future>
#include <list>

#include <gtest/gtest.h>
#include <boost/asio.hpp>

#include "common/config/config.h"

#include "core/network_protocol.h"
#include "core/client/client.h"
#include "core/server/server.h"

#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/user_services/udp_port_forwarding.h"

using NetworkProtocol = ssf::network::NetworkProtocol;

class SSFClientServerTest : public ::testing::Test {
 public:
  using Client = ssf::Client;
  using Server =
      ssf::SSFServer<NetworkProtocol::Protocol, ssf::TransportProtocolPolicy>;
  using Demux = Client::Demux;
  using UserServicePtr = Client::UserServicePtr;

 public:
  SSFClientServerTest() : p_ssf_client_(nullptr), p_ssf_server_(nullptr) {}

  ~SSFClientServerTest() {}

  virtual void SetUp() {
    std::string server_port("8100");
    StartServer(server_port);
    StartClient(server_port);
  }

  virtual void TearDown() {
    boost::system::error_code ec;
    p_ssf_client_->Stop(ec);
    p_ssf_server_->Stop();
  }

  void StartServer(const std::string& server_port) {
    ssf::config::Config ssf_config;

    ssf_config.Init();

    auto endpoint_query =
        NetworkProtocol::GenerateServerQuery("", server_port, ssf_config);
    p_ssf_server_.reset(new Server(ssf_config.services()));

    boost::system::error_code run_ec;
    p_ssf_server_->Run(endpoint_query, run_ec);
  }

  void StartClient(const std::string& server_port) {
    boost::system::error_code ec;

    ssf::config::Config ssf_config;
    ssf_config.Init();

    auto endpoint_query = NetworkProtocol::GenerateClientTLSQuery(
        "127.0.0.1", server_port, ssf_config, {});
    auto on_user_service_status = [](UserServicePtr p_user_service,
                                     const boost::system::error_code& ec) {};
    p_ssf_client_.reset(new Client());
    p_ssf_client_->Init(endpoint_query, 1, 0, false, {}, ssf_config.services(),
                        std::bind(&SSFClientServerTest::OnClientStatus, this,
                                  std::placeholders::_1),
                        on_user_service_status, ec);
    if (ec) {
      return;
    }

    p_ssf_client_->Run(ec);
    if (ec) {
      SSF_LOG(kLogError) << "Could not run client";
      return;
    }
  }

  bool Wait() {
    auto network_set_future = network_set_.get_future();
    auto transport_set_future = transport_set_.get_future();

    network_set_future.wait();
    transport_set_future.wait();

    return network_set_future.get() && transport_set_future.get();
  }

  void OnClientStatus(ssf::Status status) {
    switch (status) {
      case ssf::Status::kEndpointNotResolvable:
      case ssf::Status::kServerUnreachable:
        SSF_LOG(kLogCritical) << "Network initialization failed";
        network_set_.set_value(false);
        transport_set_.set_value(false);
        break;
      case ssf::Status::kServerNotSupported:
        SSF_LOG(kLogCritical) << "Transport initialization failed";
        transport_set_.set_value(false);
        break;
      case ssf::Status::kConnected:
        network_set_.set_value(true);
        break;
      case ssf::Status::kDisconnected:
        SSF_LOG(kLogInfo) << "client: disconnected";
        break;
      case ssf::Status::kRunning:
        transport_set_.set_value(true);
        break;
      default:
        break;
    }
  }

 protected:
  std::unique_ptr<Client> p_ssf_client_;
  std::unique_ptr<Server> p_ssf_server_;

  std::promise<bool> network_set_;
  std::promise<bool> transport_set_;
};

TEST_F(SSFClientServerTest, connectDisconnect) { ASSERT_TRUE(Wait()); }
