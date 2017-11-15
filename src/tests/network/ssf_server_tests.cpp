#include <condition_variable>
#include <mutex>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "tests/network/ssf_fixture_test.h"

TEST_F(SSFFixtureTest, failListeningWrongInterface) {
  boost::system::error_code server_ec;
  StartServer("1.1.1.1", "8200", server_ec);

  ASSERT_NE(0, server_ec.value());

  StopServer();
}

TEST_F(SSFFixtureTest, listeningAllInterfaces) {
  boost::system::error_code server_ec;
  StartServer("", "8300", server_ec);

  ASSERT_EQ(0, server_ec.value());

  StopServer();
}

TEST_F(SSFFixtureTest, listeningLocalhostInterface) {
  boost::system::error_code server_ec;
  StartServer("127.0.0.1", "8300", server_ec);

  ASSERT_EQ(0, server_ec.value());

  StopServer();
}

TEST_F(SSFFixtureTest, buggyConnectionTest) {
  int server_port = 8400;
  boost::system::error_code server_ec;
  StartServer("127.0.0.1", std::to_string(server_port), server_ec);
  ASSERT_EQ(0, server_ec.value());

  boost::system::error_code timer_ec;
  auto timer_callback = [this](const boost::system::error_code& ec) {
    EXPECT_NE(0, ec.value()) << "Timer should be canceled. Client is hanging";
    if (!ec) {
      SendNotification(false);
    }
  };
  StartTimer(std::chrono::seconds(10), timer_callback, timer_ec);
  ASSERT_EQ(0, timer_ec.value()) << "Could not start timer";

  auto client_callback = [this](ssf::Status status) {
    switch (status) {
      case ssf::Status::kEndpointNotResolvable:
      case ssf::Status::kServerUnreachable:
        SSF_LOG("test", critical, "Network initialization failed");
        SendNotification(false);
        break;
      case ssf::Status::kServerNotSupported:
        SSF_LOG("test", critical, "Transport initialization failed");
        SendNotification(false);
        break;
      case ssf::Status::kConnected:
        break;
      case ssf::Status::kDisconnected:
        SSF_LOG("test", info, "client: disconnected");
        SendNotification(true);
        break;
      case ssf::Status::kRunning:
        SendNotification(true);
        break;
      default:
        SendNotification(false);
        break;
    }
  };

  boost::asio::ip::tcp::socket buggy_client(get_io_service());
  boost::asio::ip::tcp::endpoint server_ep(
      boost::asio::ip::address::from_string("127.0.0.1"), server_port);

  auto connect_callback = [client_callback, &buggy_client, &server_port, this](
      const boost::system::error_code& ec) {
    boost::system::error_code run_ec;
    StartClient(std::to_string(server_port), client_callback, run_ec);
    EXPECT_EQ(0, run_ec.value());
    buggy_client.close(run_ec);
  };

  buggy_client.async_connect(server_ep, connect_callback);

  // Wait client action
  WaitNotification();

  EXPECT_TRUE(IsNotificationSuccess()) << "Client connection failed";

  StopTimer();
  StopClient();
  StopServer();
}
