#include <condition_variable>
#include <mutex>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "tests/network/ssf_fixture_test.h"

void InitTCPServer(boost::asio::ip::tcp::acceptor& server, int server_port);

TEST_F(SSFFixtureTest, ConnectToUnknownHost) {
  // Init timer (if client hangs)
  boost::system::error_code timer_ec;
  auto timer_callback = [this](const boost::system::error_code& ec) {
    EXPECT_NE(0, ec.value()) << "Timer should be canceled. Client is hanging";
    if (!ec) {
      SendNotification(false);
    }
  };
  StartTimer(std::chrono::seconds(5), timer_callback, timer_ec);

  auto client_callback = [this](ssf::Status status) {
    switch(status) {
      case ssf::Status::kEndpointNotResolvable:
      case ssf::Status::kServerUnreachable:
        SSF_LOG(kLogCritical) << "Network initialization failed";
        SendNotification(true);
        break;
      case ssf::Status::kServerNotSupported:
        SSF_LOG(kLogCritical) << "Transport initialization failed";
        SendNotification(false);
        break;
      case ssf::Status::kConnected:
        SendNotification(false);
        break;
      case ssf::Status::kDisconnected:
        SSF_LOG(kLogInfo) << "client: disconnected";
        break;
      case ssf::Status::kRunning:
        SendNotification(false);
        break;
      default:
        break;
    }
  };

  boost::system::error_code run_ec;
  StartClient("16000", client_callback, run_ec);
  ASSERT_EQ(0, run_ec.value()) << "Could not start client";

  // Wait client action
  WaitNotification();

  EXPECT_TRUE(IsNotificationSuccess()) << "Stop failed";

  StopClient();
  StopTimer();
}

TEST_F(SSFFixtureTest, CloseWhileConnecting) {
  // Init server
  int server_port = 15000;
  boost::asio::ip::tcp::acceptor server(get_io_service());
  InitTCPServer(server, server_port);

  // Init timer (if client hangs)
  boost::system::error_code timer_ec;
  auto timer_callback = [this](const boost::system::error_code& ec) {
    EXPECT_NE(0, ec.value()) << "Timer should be canceled. Client is hanging";
    if (!ec) {
      SendNotification(false);
    }
  };
  StartTimer(std::chrono::seconds(20), timer_callback, timer_ec);
  ASSERT_EQ(0, timer_ec.value()) << "Could not start timer";

  // Init client
  auto client_callback = [this](ssf::Status status) {
    switch(status) {
      case ssf::Status::kEndpointNotResolvable:
      case ssf::Status::kServerUnreachable:
        SSF_LOG(kLogCritical) << "Network initialization failed";
        SendNotification(true);
        break;
      case ssf::Status::kServerNotSupported:
        SSF_LOG(kLogCritical) << "Transport initialization failed";
        SendNotification(true);
        break;
      case ssf::Status::kConnected:
        SendNotification(true);
        break;
      case ssf::Status::kDisconnected:
        SSF_LOG(kLogInfo) << "client: disconnected";
        break;
      case ssf::Status::kRunning:
        SendNotification(false);
        break;
      default:
        break;
    }
  };
  boost::system::error_code run_ec;
  StartClient(std::to_string(server_port), client_callback, run_ec);
  ASSERT_EQ(0, run_ec.value());

  // Wait new server connection
  boost::asio::ip::tcp::socket socket(get_io_service());
  server.async_accept(socket, [this](const boost::system::error_code& ec) {
    EXPECT_EQ(0, ec.value()) << "Accept connection in error";
    // Stop client while connecting
    StopClient();
  });

  // Wait client action
  WaitNotification();

  EXPECT_TRUE(IsNotificationSuccess()) << "Stop failed";

  StopTimer();
  boost::system::error_code close_ec;
  socket.close(close_ec);
  server.close(close_ec);
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
