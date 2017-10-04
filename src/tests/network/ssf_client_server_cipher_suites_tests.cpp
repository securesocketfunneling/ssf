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

class SSFClientServerCipherSuitesTest : public ::testing::Test {
 public:
  using Client = ssf::Client;
  using Server = ssf::SSFServer<NetworkProtocol::FullTLSProtocol,
                                ssf::TransportProtocolPolicy>;
  using Demux = Client::Demux;
  using UserServicePtr = Client::UserServicePtr;

 public:
  SSFClientServerCipherSuitesTest()
      : p_ssf_client_(nullptr), p_ssf_server_(nullptr) {}

  ~SSFClientServerCipherSuitesTest() {}

  virtual void TearDown() {
    boost::system::error_code ec;
    p_ssf_client_->Stop(ec);
    p_ssf_server_->Stop();
  }

  void StartServer(const std::string& server_port,
                   const ssf::config::Config& config) {
    auto endpoint_query =
        NetworkProtocol::GenerateServerTLSQuery("", server_port, config);

    p_ssf_server_.reset(new Server(config.services()));

    boost::system::error_code run_ec;
    p_ssf_server_->Run(endpoint_query, run_ec);
  }

  void StartClient(const std::string& server_port,
                   const ssf::config::Config& config,
                   const Client::OnStatusCb& on_status) {
    boost::system::error_code ec;
    auto endpoint_query = NetworkProtocol::GenerateClientTLSQuery(
        "127.0.0.1", server_port, config, {});
    auto on_user_service_status = [](UserServicePtr p_user_service,
                                     const boost::system::error_code& ec) {};
    p_ssf_client_.reset(new Client());
    p_ssf_client_->Init(endpoint_query, 1, 0, false, {}, config.services(),
                        on_status, on_user_service_status, ec);
    if (ec) {
      return;
    }

    p_ssf_client_->Run(ec);
    if (ec) {
      SSF_LOG(kLogError) << "Could not run client";
      return;
    }
  }

 protected:
  std::unique_ptr<Client> p_ssf_client_;
  std::unique_ptr<Server> p_ssf_server_;
};

TEST_F(SSFClientServerCipherSuitesTest, connectDisconnectDifferentSuite) {
  std::promise<bool> network_set;
  std::promise<bool> transport_set;

  auto network_set_future = network_set.get_future();
  auto transport_set_future = transport_set.get_future();
  auto callback = [&network_set, &transport_set](ssf::Status status) {
    switch (status) {
      case ssf::Status::kEndpointNotResolvable:
      case ssf::Status::kServerUnreachable:
        SSF_LOG(kLogCritical) << "Network initialization failed";
        network_set.set_value(false);
        transport_set.set_value(false);
        break;
      case ssf::Status::kServerNotSupported:
        SSF_LOG(kLogCritical) << "Transport initialization failed";
        transport_set.set_value(false);
        break;
      case ssf::Status::kConnected:
        network_set.set_value(true);
        break;
      case ssf::Status::kDisconnected:
        SSF_LOG(kLogInfo) << "client: disconnected";
        break;
      case ssf::Status::kRunning:
        transport_set.set_value(true);
        break;
      default:
        break;
    }
  };

  ssf::config::Config client_config;
  client_config.Init();
  ssf::config::Config server_config;
  server_config.Init();
  boost::system::error_code ec;

  const char* new_config = R"RAWSTRING(
{
    "ssf": {
        "tls" : {
            "cipher_alg": "DHE-RSA-AES256-SHA256"
        }
    }
}
)RAWSTRING";

  server_config.UpdateFromString(new_config, ec);
  ASSERT_EQ(ec.value(), 0) << "Could not update server config from string "
                           << new_config;

  std::string server_port("8600");
  StartServer(server_port, server_config);
  StartClient(server_port, client_config, callback);

  network_set_future.wait();
  transport_set_future.wait();
}

TEST_F(SSFClientServerCipherSuitesTest, connectDisconnectTwoSuites) {
  std::promise<bool> network_set;
  std::promise<bool> transport_set;

  auto network_set_future = network_set.get_future();
  auto transport_set_future = transport_set.get_future();

  auto callback = [&network_set, &transport_set](ssf::Status status) {
    switch (status) {
      case ssf::Status::kEndpointNotResolvable:
      case ssf::Status::kServerUnreachable:
        SSF_LOG(kLogCritical) << "Network initialization failed";
        network_set.set_value(false);
        transport_set.set_value(false);
        break;
      case ssf::Status::kServerNotSupported:
        SSF_LOG(kLogCritical) << "Transport initialization failed";
        transport_set.set_value(false);
        break;
      case ssf::Status::kConnected:
        network_set.set_value(true);
        break;
      case ssf::Status::kDisconnected:
        SSF_LOG(kLogInfo) << "client: disconnected";
        break;
      case ssf::Status::kRunning:
        transport_set.set_value(true);
        break;
      default:
        break;
    }
  };

  ssf::config::Config client_config;
  client_config.Init();
  ssf::config::Config server_config;
  server_config.Init();
  boost::system::error_code ec;
  const char* new_client_config = R"RAWSTRING(
{
    "ssf": {
        "tls" : {
            "cipher_alg": "DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-SHA256"
        }
    }
}
)RAWSTRING";

  const char* new_server_config = R"RAWSTRING(
{
    "ssf": {
        "tls" : {
            "cipher_alg": "ECDH-ECDSA-AES128-SHA256:DHE-RSA-AES128-SHA256"
        }
    }
}
)RAWSTRING";

  client_config.UpdateFromString(new_client_config, ec);
  ASSERT_EQ(ec.value(), 0) << "Could not update client config from string "
                           << new_client_config;
  server_config.UpdateFromString(new_server_config, ec);
  ASSERT_EQ(ec.value(), 0) << "Could not update server config from string "
                           << new_server_config;

  std::string server_port("8700");
  StartServer(server_port, server_config);
  StartClient(server_port, client_config, callback);

  network_set_future.wait();
  transport_set_future.wait();
}
