#include <condition_variable>
#include <mutex>
#include <vector>

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <gtest/gtest.h>

#include "common/config/config.h"

#include "core/network_protocol.h"
#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"
#include "core/client/client.h"

using NetworkProtocol = ssf::network::NetworkProtocol;
using Client =
    ssf::SSFClient<NetworkProtocol::Protocol, ssf::TransportProtocolPolicy>;
using Demux = Client::Demux;
using BaseUserServicePtr =
    ssf::services::BaseUserService<Demux>::BaseUserServicePtr;

void InitTCPServer(boost::asio::ip::tcp::acceptor& server, int server_port);

TEST(SSFClientTest, CloseWhileConnecting) {
  ssf::AsyncEngine async_engine;

  std::condition_variable wait_stop_cv;
  std::mutex mutex;
  bool stopped = false;

  // End test callback
  auto end_test = [&wait_stop_cv, &mutex, &stopped](bool status) {
    {
      boost::lock_guard<std::mutex> lock(mutex);
      stopped = status;
    }
    wait_stop_cv.notify_all();
  };

  // Init server
  int server_port = 15000;
  boost::asio::ip::tcp::acceptor server(async_engine.get_io_service());
  InitTCPServer(server, 15000);

  // Init timer (if client hangs)
  boost::system::error_code timer_ec;
  boost::asio::steady_timer timer(async_engine.get_io_service());
  timer.expires_from_now(std::chrono::seconds(5), timer_ec);
  ASSERT_EQ(0, timer_ec.value());
  timer.async_wait([&end_test](const boost::system::error_code& ec) {
    EXPECT_NE(0, ec.value()) << "Timer should be canceled. Client is hanging";
    if (!ec) {
      end_test(false);
    }
  });

  // Init client
  std::vector<BaseUserServicePtr> client_options;
  ssf::config::Config ssf_config;
  ssf_config.Init();

  auto endpoint_query = NetworkProtocol::GenerateClientQuery(
      "127.0.0.1", std::to_string(server_port), ssf_config, {});

  auto callback = [&end_test](ssf::services::initialisation::type type,
                              BaseUserServicePtr p_user_service,
                              const boost::system::error_code& ec) {
    EXPECT_EQ(ssf::services::initialisation::NETWORK, type);
    EXPECT_NE(0, ec.value());
    end_test(ec.value() != 0);
  };
  Client client(client_options, ssf_config.services(), callback);

  boost::system::error_code run_ec;

  // Connect client to server
  client.Run(endpoint_query, run_ec);
  ASSERT_EQ(0, run_ec.value());

  // Wait new server connection
  async_engine.Start();
  ASSERT_TRUE(async_engine.IsStarted());
  boost::asio::ip::tcp::socket socket(async_engine.get_io_service());
  server.async_accept(socket, [&client](const boost::system::error_code& ec) {
    EXPECT_EQ(0, ec.value()) << "Accept connection in error";
    // Stop client while connecting
    client.Stop();
  });

  // Wait client action
  std::unique_lock<std::mutex> lock(mutex);
  wait_stop_cv.wait(lock);
  lock.unlock();

  EXPECT_TRUE(stopped) << "Stop failed";

  timer.cancel(timer_ec);
  boost::system::error_code close_ec;
  socket.close(close_ec);
  server.close(close_ec);

  async_engine.Stop();
}

void InitTCPServer(boost::asio::ip::tcp::acceptor& server, int server_port) {
  boost::system::error_code server_ec;
  boost::asio::ip::tcp::endpoint server_ep(boost::asio::ip::tcp::v4(),
                                           server_port);
  server.open(boost::asio::ip::tcp::v4(), server_ec);
  ASSERT_EQ(0, server_ec.value()) << "Could not open server acceptor";
  server.bind(server_ep, server_ec);
  ASSERT_EQ(0, server_ec.value()) << "Could not bind server acceptor";
  server.listen(boost::asio::socket_base::max_connections, server_ec);
  ASSERT_EQ(0, server_ec.value()) << "Server acceptor could not listen";
}
