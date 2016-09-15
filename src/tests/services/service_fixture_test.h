#ifndef TESTS_SERVICES_SERVICE_FIXTURE_TEST_H_
#define TESTS_SERVICES_SERVICE_FIXTURE_TEST_H_

#include <random>
#include <utility>

#include <gtest/gtest.h>

#include <ssf/log/log.h>

#include "services/base_service.h"
#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "core/network_protocol.h"
#include "core/client/client.h"
#include "core/server/server.h"

template <template <typename> class TServiceTested>
class ServiceFixtureTest : public ::testing::Test {
 public:
  using NetworkProtocol = ssf::network::NetworkProtocol;
  using Client =
      ssf::SSFClient<NetworkProtocol::Protocol, ssf::TransportProtocolPolicy>;
  using Server =
      ssf::SSFServer<NetworkProtocol::Protocol, ssf::TransportProtocolPolicy>;
  using demux = Client::Demux;
  using BaseUserServicePtr =
      ssf::services::BaseUserService<demux>::BaseUserServicePtr;
  using ServiceTested = TServiceTested<demux>;

 public:
  virtual ~ServiceFixtureTest() {}

  void SetUp() override {
    auto cleanup = [this]() {
      network_set_.set_value(false);
      service_set_.set_value(false);
      transport_set_.set_value(false);
    };

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribution(30000, 65000);

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
    p_ssf_client_->Stop();
    p_ssf_server_->Stop();
  }

  virtual void SetServerConfig(ssf::config::Config& config) {}

  virtual void SetClientConfig(ssf::config::Config& config) {}

  bool StartServer(const std::string& host_addr, const std::string& host_port) {
    ssf::config::Config ssf_config;
    ssf_config.Init();

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
    std::vector<BaseUserServicePtr> client_options;
    boost::system::error_code ec;

    BaseUserServicePtr p_service = ServiceCreateServiceOptions(ec);

    client_options.push_back(p_service);

    ssf::config::Config ssf_config;
    ssf_config.Init();

    SetClientConfig(ssf_config);

    auto endpoint_query = NetworkProtocol::GenerateClientQuery(
        target_addr, target_host, ssf_config, {});

    p_ssf_client_.reset(new Client(
        client_options, ssf_config.services(),
        boost::bind(&ServiceFixtureTest::SSFClientCallback, this, _1, _2, _3)));
    boost::system::error_code run_ec;
    p_ssf_client_->Run(endpoint_query, run_ec);
    if (run_ec) {
      SSF_LOG(kLogError) << "Could not run client";
    }
    return run_ec.value() == 0;
  }

  virtual std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
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

  void SSFClientCallback(ssf::services::initialisation::type type,
                         BaseUserServicePtr p_user_service,
                         const boost::system::error_code& ec) {
    if (type == ssf::services::initialisation::NETWORK) {
      network_set_.set_value(!ec);
      if (ec) {
        SSF_LOG(kLogCritical) << "Network initialization failed";
        service_set_.set_value(false);
        transport_set_.set_value(false);
      }

      return;
    }

    if (type == ssf::services::initialisation::TRANSPORT) {
      transport_set_.set_value(!ec);
      if (ec) {
        SSF_LOG(kLogCritical) << "Transport initialization failed";
      }

      return;
    }

    if (type == ssf::services::initialisation::SERVICE) {
      if (ec) {
        SSF_LOG(kLogCritical) << "user_service[" << p_user_service->GetName()
                              << "]: initialization failed";
      }
      if (p_user_service->GetName() == ServiceTested::GetParseName()) {
        service_set_.set_value(!ec);
      }

      return;
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
