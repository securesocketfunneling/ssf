#ifndef TESTS_SERVICES_SERVICE_FIXTURE_TEST_H_
#define TESTS_SERVICES_SERVICE_FIXTURE_TEST_H_

#include <functional>
#include <random>
#include <utility>

#include <gtest/gtest.h>

#include <ssf/log/log.h>

#include "services/base_service.h"
#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "core/network_protocol.h"
#include "core/client/client.h"
#include "core/client/status.h"
#include "core/server/server.h"
#include "services/user_services/parameters.h"

#include "tests/tls_config_helper.h"

template <template <typename> class TServiceTested>
class ServiceFixtureTest : public ::testing::Test {
 public:
  using NetworkProtocol = ssf::network::NetworkProtocol;
  using Client = ssf::Client;
  using Server =
      ssf::SSFServer<NetworkProtocol::Protocol, ssf::TransportProtocolPolicy>;
  using demux = Client::Demux;
  using BaseUserServicePtr =
      ssf::services::BaseUserService<demux>::BaseUserServicePtr;
  using ServiceTested = TServiceTested<demux>;

 public:
  virtual ~ServiceFixtureTest() {}

  void SetUp() override {
    ssf::log::Log::SetSeverityLevel(ssf::log::LogLevel::kLogDebug);
    auto cleanup = [this]() {
      network_set_.set_value(false);
      service_set_.set_value(false);
      transport_set_.set_value(false);
    };

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribution(10000, 32767);

    auto port = std::to_string(distribution(gen));

    SSF_LOG(kLogInfo) << "Service fixture: server will listen on port " << port;

    if (!StartServer("127.0.0.1", port)) {
      cleanup();
      FAIL() << "Could not start server";
      return;
    }
    if (!StartClient("127.0.0.1", port)) {
      cleanup();
      FAIL() << "Could not start client";
      return;
    }
  }

  void TearDown() override {
    boost::system::error_code stop_ec;
    if (p_ssf_client_) {
      p_ssf_client_->Stop(stop_ec);
      p_ssf_client_->Deinit();
    }
    if (p_ssf_server_) {
      p_ssf_server_->Stop();
    }
  }

  virtual void SetServerConfig(ssf::config::Config& config) {}

  virtual void SetClientConfig(ssf::config::Config& config) {}

  bool StartServer(const std::string& host_addr, const std::string& host_port) {
    ssf::config::Config ssf_config;
    ssf_config.Init();
    ssf::tests::SetServerTlsConfig(&ssf_config);

    SetServerConfig(ssf_config);
    auto endpoint_query =
        NetworkProtocol::GenerateServerQuery(host_addr, host_port, ssf_config);

    p_ssf_server_.reset(new Server(ssf_config.services()));

    boost::system::error_code run_ec;
    p_ssf_server_->Run(endpoint_query, run_ec);
    if (run_ec) {
      SSF_LOG(kLogError) << "Could not run server";
    }
    return run_ec.value() == 0;
  }

  bool StartClient(const std::string& target_addr,
                   const std::string target_host) {
    boost::system::error_code ec;

    auto user_service_params = CreateUserServiceParameters(ec);

    ssf::config::Config ssf_config;
    ssf_config.Init();
    ssf::tests::SetClientTlsConfig(&ssf_config);

    SetClientConfig(ssf_config);

    auto endpoint_query = NetworkProtocol::GenerateClientQuery(
        target_addr, target_host, ssf_config, {});

    // Init client
    p_ssf_client_.reset(new Client());
    p_ssf_client_->Register<TServiceTested<demux>>();
    p_ssf_client_->Init(
        endpoint_query, 1, 0, false, user_service_params, ssf_config.services(),
        std::bind(&ServiceFixtureTest::OnClientStatus, this,
                  std::placeholders::_1),
        std::bind(&ServiceFixtureTest::OnClientUserServiceStatus, this,
                  std::placeholders::_1, std::placeholders::_2),
        ec);
    if (ec) {
      return false;
    }

    p_ssf_client_->Run(ec);
    if (ec) {
      SSF_LOG(kLogError) << "Could not run client";
    }
    return ec.value() == 0;
  }

  virtual ssf::UserServiceParameters CreateUserServiceParameters(
      boost::system::error_code& ec) = 0;

  bool Wait() {
    auto network_set_future = network_set_.get_future();
    auto service_set_future = service_set_.get_future();
    auto transport_set_future = transport_set_.get_future();

    network_set_future.wait();
    service_set_future.wait();
    transport_set_future.wait();

    return network_set_future.get() && service_set_future.get() &&
           transport_set_future.get();
  }

  void OnClientStatus(ssf::Status status) {
    switch (status) {
      case ssf::Status::kEndpointNotResolvable:
      case ssf::Status::kServerUnreachable:
        SSF_LOG(kLogCritical) << "Network initialization failed";
        network_set_.set_value(false);
        transport_set_.set_value(false);
        service_set_.set_value(false);
        break;
      case ssf::Status::kServerNotSupported:
        SSF_LOG(kLogCritical) << "Transport initialization failed";
        transport_set_.set_value(false);
        service_set_.set_value(false);
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

  void OnClientUserServiceStatus(BaseUserServicePtr p_user_service,
                                 const boost::system::error_code& ec) {
    if (ec) {
      SSF_LOG(kLogCritical) << "user_service[" << p_user_service->GetName()
                            << "]: initialization failed";
    }
    if (p_user_service->GetName() == ServiceTested::GetParseName()) {
      service_set_.set_value(!ec);
    }
  }

 protected:
  ServiceFixtureTest() : p_ssf_client_(nullptr), p_ssf_server_(nullptr) {}

 protected:
  std::unique_ptr<Client> p_ssf_client_;
  std::unique_ptr<Server> p_ssf_server_;

  std::promise<bool> network_set_;
  std::promise<bool> transport_set_;
  std::promise<bool> service_set_;
};

#endif  // TESTS_SERVICES_SERVICE_FIXTURE_TEST_H_
