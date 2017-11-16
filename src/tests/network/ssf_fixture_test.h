#ifndef TESTS_NETWORK_SSF_FIXTURE_TEST_H_
#define TESTS_NETWORK_SSF_FIXTURE_TEST_H_

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <gtest/gtest.h>

#include "common/config/config.h"

#include "core/network_protocol.h"
#include "core/client/client.h"
#include "core/server/server.h"

#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

using NetworkProtocol = ssf::network::NetworkProtocol;

class SSFFixtureTest : public ::testing::Test {
 public:
  using Client = ssf::Client;
  using Server =
      ssf::SSFServer<NetworkProtocol::Protocol, ssf::TransportProtocolPolicy>;
  using ClientCallback = Client::OnStatusCb;
  using TimerCallback = std::function<void(const boost::system::error_code&)>;
  using UserServicePtr = Client::UserServicePtr;

 public:
  void SetUp() override;
  void TearDown() override;

  boost::asio::io_service& get_io_service();

  void StartAsyncEngine();
  void StopAsyncEngine();

  void StartClient(const std::string& server_port, ClientCallback callback,
                   boost::system::error_code& ec);
  void StopClient();

  void StartServer(const std::string& addr, const std::string& server_port,
                   boost::system::error_code& ec);
  void StopServer();

  void StartTimer(const std::chrono::seconds& wait_duration,
                  TimerCallback callback, boost::system::error_code&);

  void StopTimer();

  void WaitNotification();

  void SendNotification(bool success);

  bool IsNotificationSuccess();

 protected:
  SSFFixtureTest();

 protected:
  std::unique_ptr<Client> p_ssf_client_;
  std::unique_ptr<Server> p_ssf_server_;
  std::unique_ptr<boost::asio::steady_timer> p_timer_;

 private:
  ssf::AsyncEngine async_engine_;
  std::condition_variable wait_stop_cv_;
  std::mutex mutex_;
  bool success_;
  bool stopped_;
};

#endif  // TESTS_NETWORK_SSF_FIXTURE_TEST_H_