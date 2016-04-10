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

#include <ssf/layer/data_link/circuit_helpers.h>
#include <ssf/layer/data_link/basic_circuit_protocol.h>
#include <ssf/layer/data_link/simple_circuit_policy.h>
#include <ssf/layer/parameters.h>
#include <ssf/layer/physical/tcp.h>
#include <ssf/layer/physical/tlsotcp.h>

#include "common/config/config.h"

#include "core/client/client.h"
#include "core/client/query_factory.h"
#include "core/server/server.h"
#include "core/server/query_factory.h"

#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/initialisation.h"
#include "services/user_services/udp_port_forwarding.h"

class SSFClientServerCipherSuitesTest : public ::testing::Test
{
 public:
  using TLSPhysicalProtocol = ssf::layer::physical::TLSboTCPPhysicalLayer;
  using TLSCircuitProtocol = ssf::layer::data_link::basic_CircuitProtocol<
      TLSPhysicalProtocol, ssf::layer::data_link::CircuitPolicy>;
  using Client =
      ssf::SSFClient<TLSCircuitProtocol, ssf::TransportProtocolPolicy>;
  using Server =
      ssf::SSFServer<TLSCircuitProtocol, ssf::TransportProtocolPolicy>;
  using demux = Client::demux;
  using BaseUserServicePtr =
      ssf::services::BaseUserService<demux>::BaseUserServicePtr;
  typedef boost::function<void(ssf::services::initialisation::type,
                               BaseUserServicePtr,
                               const boost::system::error_code&)>
     ClientCallback;

 public:
   SSFClientServerCipherSuitesTest()
      : client_io_service_(),
        p_client_worker_(new boost::asio::io_service::work(client_io_service_)),
        server_io_service_(),
        p_server_worker_(new boost::asio::io_service::work(server_io_service_)),
        p_ssf_client_(nullptr),
        p_ssf_server_(nullptr) {}

   ~SSFClientServerCipherSuitesTest() {}

  virtual void TearDown() {
    StopClientThreads();
    StopServerThreads();
  }

  void StartServer(const ssf::Config& config) {
    uint16_t port = 8000;
    auto endpoint_query =
        ssf::GenerateServerTLSNetworkQuery(std::to_string(port), config);

    p_ssf_server_.reset(new Server(server_io_service_, config, 8000));

    StartServerThreads();
    p_ssf_server_->Run(endpoint_query);
  }

  void StartClient(const ssf::Config& config, const ClientCallback& callback) {
    std::vector<BaseUserServicePtr>
        client_options;

    auto endpoint_query =
        ssf::GenerateTLSNetworkQuery("127.0.0.1", "8000", config, {});

    p_ssf_client_.reset(
        new Client(client_io_service_, client_options, callback));
    StartClientThreads();
    p_ssf_client_->Run(endpoint_query);
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

 protected:
  boost::asio::io_service client_io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_client_worker_;
  boost::thread_group client_threads_;

  boost::asio::io_service server_io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_server_worker_;
  boost::thread_group server_threads_;
  std::unique_ptr<Client> p_ssf_client_;
  std::unique_ptr<Server> p_ssf_server_;
};

//-----------------------------------------------------------------------------
TEST_F(SSFClientServerCipherSuitesTest, connectDisconnectDifferentSuite) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::debug);


  std::promise<bool> network_set;
  std::promise<bool> transport_set;

  auto network_set_future = network_set.get_future();
  auto transport_set_future = transport_set.get_future();
  auto callback = [&network_set, &transport_set](
      ssf::services::initialisation::type type,
      SSFClientServerCipherSuitesTest::BaseUserServicePtr p_user_service,
      const boost::system::error_code& ec) {
    if (type == ssf::services::initialisation::NETWORK) {
      EXPECT_TRUE(!!ec);
      network_set.set_value(!ec);
      transport_set.set_value(false);

      return;
    }
  };
  ssf::Config client_config;
  ssf::Config server_config;
  server_config.tls.cipher_alg = "DHE-RSA-AES256-GCM-SHA256";
  StartServer(server_config);
  StartClient(client_config, callback);

  network_set_future.wait();
  transport_set_future.wait();
}


//-----------------------------------------------------------------------------
TEST_F(SSFClientServerCipherSuitesTest, connectDisconnectTwoSuites) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::debug);


  std::promise<bool> network_set;
  std::promise<bool> transport_set;

  auto network_set_future = network_set.get_future();
  auto transport_set_future = transport_set.get_future();
  
  auto callback = [&network_set,
    &transport_set](ssf::services::initialisation::type type,
      SSFClientServerCipherSuitesTest::BaseUserServicePtr p_user_service,
      const boost::system::error_code& ec) {
    if (type == ssf::services::initialisation::NETWORK) {
      EXPECT_TRUE(!ec);
      network_set.set_value(!ec);

      return;
    } else if (type == ssf::services::initialisation::TRANSPORT) {
      EXPECT_TRUE(!ec);
      transport_set.set_value(!ec);

      return;
    }
  };
  ssf::Config client_config;
  ssf::Config server_config;
  client_config.tls.cipher_alg = "DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-SHA256";
  server_config.tls.cipher_alg = "ECDH-ECDSA-AES128-SHA256:DHE-RSA-AES128-SHA256";
  StartServer(server_config);
  StartClient(client_config, callback);

  network_set_future.wait();
  transport_set_future.wait();
}
