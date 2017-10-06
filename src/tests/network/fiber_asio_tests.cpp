#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <gtest/gtest.h>

#include <ssf/log/log.h>

#include "common/boost/fiber/basic_endpoint.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"
#include "common/boost/fiber/datagram_fiber.hpp"
#include "common/boost/fiber/stream_fiber.hpp"

#include "tests/tls_config_helper.h"

class FiberTest : public ::testing::Test {
 protected:
  FiberTest()
      : client_threads_(),
        io_service_client_(),
        p_client_worker(std::unique_ptr<boost::asio::io_service::work>(
            new boost::asio::io_service::work(io_service_client_))),
        socket_client_(io_service_client_),
        resolver_client_(io_service_client_),
        query_client_(boost::asio::ip::tcp::v4(), "127.0.0.1", "9011"),
        iterator_client_(resolver_client_.resolve(query_client_)),
        demux_client_(io_service_client_),
        client_ready_(),
        server_threads_(),
        io_service_server_(),
        p_server_worker(std::unique_ptr<boost::asio::io_service::work>(
            new boost::asio::io_service::work(io_service_server_))),
        socket_server_(io_service_server_),
        acceptor_server_(io_service_server_),
        resolver_server_(io_service_server_),
        query_server_(boost::asio::ip::tcp::v4(), "127.0.0.1", "9011"),
        endpoint_server_(*resolver_server_.resolve(query_server_)),
        demux_server_(io_service_server_),
        server_ready_() {}

  ~FiberTest() {}

  typedef boost::asio::ip::tcp::socket socket;
  typedef boost::asio::ip::tcp::resolver::iterator tcp_endpoint_it;
  typedef boost::asio::fiber::stream_fiber<socket> stream_fiber;
  typedef boost::asio::fiber::datagram_fiber<socket> datagram_fiber;
  typedef boost::asio::fiber::basic_fiber_demux<socket> fiber_demux;
  typedef stream_fiber::socket fiber;
  typedef stream_fiber::acceptor fiber_acceptor;
  typedef stream_fiber::endpoint fiber_endpoint;
  typedef datagram_fiber::socket dgr_fiber;
  typedef datagram_fiber::endpoint dgr_fiber_endpoint;
  typedef std::function<void(const boost::system::error_code&)>
      accept_handler_type;
  typedef std::function<void(const boost::system::error_code&, tcp_endpoint_it)>
      connect_handler_type;

  virtual void SetUp() {
    StartServer();
    ConnectClient();
  }

  virtual void TearDown() {
    StopClient();
    StopServer();
  }

  bool Wait() {
    auto server_ready_future = server_ready_.get_future();
    auto client_ready_future = client_ready_.get_future();

    server_ready_future.wait();
    client_ready_future.wait();

    return server_ready_future.get() && client_ready_future.get();
  }

 private:
  void StartServer(accept_handler_type accept_h =
                       [](const boost::system::error_code& ec) {}) {
    auto lambda = [this]() {
      boost::system::error_code ec;
      this->io_service_server_.run(ec);
    };

    for (uint8_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
      server_threads_.emplace_back(lambda);
    }

    auto accepted_lambda = [this,
                            accept_h](const boost::system::error_code& ec) {
      if (!ec) {
        this->demux_server_.fiberize(std::move(this->socket_server_));
      }

      this->server_ready_.set_value(!ec);
      accept_h(ec);
    };

    boost::system::error_code server_ec;
    boost::asio::socket_base::reuse_address option(true);
    acceptor_server_.open(endpoint_server_.protocol(), server_ec);
    acceptor_server_.set_option(option, server_ec);
    acceptor_server_.bind(endpoint_server_, server_ec);
    acceptor_server_.listen(boost::asio::socket_base::max_connections,
                            server_ec);
    acceptor_server_.async_accept(socket_server_, std::move(accepted_lambda));
  }

  void ConnectClient(connect_handler_type connect_h =
                         [](const boost::system::error_code&, tcp_endpoint_it) {
                         }) {
    auto lambda = [this]() {
      boost::system::error_code ec;
      this->io_service_client_.run(ec);
    };

    for (uint8_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
      client_threads_.emplace_back(lambda);
    }

    auto connected_lambda = [this, connect_h](
        const boost::system::error_code& ec, tcp_endpoint_it ep_it) {
      if (!ec) {
        this->demux_client_.fiberize(std::move(this->socket_client_));
      }

      this->client_ready_.set_value(!ec);
      connect_h(ec, ep_it);
    };

    boost::asio::async_connect(socket_client_, iterator_client_,
                               std::move(connected_lambda));
  }

  void StopServer() {
    demux_server_.close();
    socket_server_.close();
    acceptor_server_.close();
    p_server_worker.reset();

    for (auto& thread : server_threads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
    io_service_server_.stop();
  }

  void StopClient() {
    boost::system::error_code ec;
    demux_client_.close();
    socket_client_.close(ec);
    p_client_worker.reset();

    for (auto& thread : client_threads_) {
      if (thread.joinable()) {
        thread.join();
      }
    };
    io_service_client_.stop();
  }

 protected:
  std::vector<std::thread> client_threads_;
  boost::asio::io_service io_service_client_;
  std::unique_ptr<boost::asio::io_service::work> p_client_worker;
  boost::asio::ip::tcp::socket socket_client_;
  boost::asio::ip::tcp::resolver resolver_client_;
  boost::asio::ip::tcp::resolver::query query_client_;
  boost::asio::ip::tcp::resolver::iterator iterator_client_;
  fiber_demux demux_client_;
  std::promise<bool> client_ready_;

  std::vector<std::thread> server_threads_;
  boost::asio::io_service io_service_server_;
  std::unique_ptr<boost::asio::io_service::work> p_server_worker;
  boost::asio::ip::tcp::socket socket_server_;
  boost::asio::ip::tcp::acceptor acceptor_server_;
  boost::asio::ip::tcp::resolver resolver_server_;
  boost::asio::ip::tcp::resolver::query query_server_;
  boost::asio::ip::tcp::endpoint endpoint_server_;
  fiber_demux demux_server_;
  std::promise<bool> server_ready_;
};

//-----------------------------------------------------------------------------
TEST_F(FiberTest, StartStopServer) { Wait(); }

//----------------------------------------------------------------------------
TEST_F(FiberTest, ConnectDisconnectFiberFromServer) {
  Wait();

  std::promise<bool> accept_closed;
  std::promise<bool> connect_closed;

  fiber_acceptor fib_acceptor(io_service_server_);
  fiber fib_server(io_service_server_);
  fiber fib_client(io_service_client_);

  auto accepted_lambda = [this, &fib_server,
                          &accept_closed](const boost::system::error_code& ec) {
    ASSERT_EQ(ec.value(), 0) << "Accept handler should not be in error";

    boost::system::error_code close_fib_server_ec;
    fib_server.close(close_fib_server_ec);

    accept_closed.set_value(!ec);
  };

  auto wait_closing_lambda =
      [&fib_client, &connect_closed](const boost::system::error_code& ec) {
        auto p_int = std::make_shared<int>(0);
        auto& connect_closed_ref = connect_closed;
        auto receive_handler = [p_int, &connect_closed_ref](
            const boost::system::error_code& ec, size_t) {
          connect_closed_ref.set_value(!!ec);
        };
        fib_client.async_receive(boost::asio::buffer(p_int.get(), sizeof(int)),
                                 std::move(receive_handler));
      };

  auto connected_lambda =
      [this, &wait_closing_lambda](const boost::system::error_code& ec) {
        ASSERT_EQ(ec.value(), 0) << "Connect handler should not be in error";

        io_service_client_.post(std::bind(wait_closing_lambda, ec));
      };

  boost::system::error_code acceptor_ec;
  fiber_endpoint fib_server_endpoint(
      boost::asio::fiber::stream_fiber<socket>::v1(), demux_server_, 1);
  fib_acceptor.open(fib_server_endpoint.protocol());
  fib_acceptor.bind(fib_server_endpoint, acceptor_ec);
  fib_acceptor.listen();
  fib_acceptor.async_accept(fib_server, std::move(accepted_lambda));

  fiber_endpoint fib_client_endpoint(
      boost::asio::fiber::stream_fiber<socket>::v1(), demux_client_, 1);
  fib_client.async_connect(fib_client_endpoint, connected_lambda);

  connect_closed.get_future().wait();
  accept_closed.get_future().wait();

  boost::system::error_code close_fib_acceptor_ec;
  fib_acceptor.close(close_fib_acceptor_ec);
}

//----------------------------------------------------------------------------
TEST_F(FiberTest, ConnectDisconnectFiberFromClient) {
  Wait();

  std::promise<bool> accept_closed;
  std::promise<bool> connect_closed;

  fiber_acceptor fib_acceptor(io_service_server_);
  fiber fib_server(io_service_server_);
  fiber fib_client(io_service_client_);

  auto wait_closing_lambda =
      [&fib_server, &accept_closed](const boost::system::error_code& ec) {
        auto p_int = std::make_shared<int>(0);
        auto& accept_closed_ref = accept_closed;
        auto receive_handler = [p_int, &accept_closed_ref](
            const boost::system::error_code& ec, size_t) {
          accept_closed_ref.set_value(!!ec);
        };
        fib_server.async_receive(boost::asio::buffer(p_int.get(), sizeof(int)),
                                 std::move(receive_handler));
      };

  auto accepted_lambda =
      [this, &wait_closing_lambda](const boost::system::error_code& ec) {
        ASSERT_EQ(ec.value(), 0) << "Accept handler should not be in error";

        io_service_client_.post(std::bind(wait_closing_lambda, ec));
      };

  auto connected_lambda = [this, &fib_client, &connect_closed](
      const boost::system::error_code& ec) {
    ASSERT_EQ(ec.value(), 0) << "Connect handler should not be in error";

    if (!ec) {
      boost::system::error_code close_fib_client_ec;
      fib_client.close(close_fib_client_ec);
    }

    connect_closed.set_value(!ec);
  };

  boost::system::error_code acceptor_ec;
  fiber_endpoint fib_server_endpoint(
      boost::asio::fiber::stream_fiber<socket>::v1(), demux_server_, 1);
  fib_acceptor.open(fib_server_endpoint.protocol());
  fib_acceptor.bind(fib_server_endpoint, acceptor_ec);
  fib_acceptor.listen();
  fib_acceptor.async_accept(fib_server, std::move(accepted_lambda));

  fiber_endpoint fib_client_endpoint(
      boost::asio::fiber::stream_fiber<socket>::v1(), demux_client_, 1);
  fib_client.async_connect(fib_client_endpoint, connected_lambda);

  connect_closed.get_future().wait();
  accept_closed.get_future().wait();

  boost::system::error_code close_fib_acceptor_ec;
  fib_acceptor.close(close_fib_acceptor_ec);
}

//-----------------------------------------------------------------------------
TEST_F(FiberTest, MultipleConnectDisconnectFiber) {
  Wait();

  const uint32_t number_of_connections = 1000;
  std::atomic<uint32_t> number_of_connected(0);
  std::atomic<uint32_t> number_of_accepted(0);
  std::promise<bool> accepted_all;
  std::promise<bool> connected_all;

  fiber_acceptor fib_acceptor(io_service_server_);

  std::function<void(std::shared_ptr<fiber> p_fiber,
                     const boost::system::error_code&)>
      async_accept_h1;
  std::function<void()> async_accept_h2;

  async_accept_h2 = [this, &async_accept_h1, &fib_acceptor]() {
    auto p_fiber = std::make_shared<fiber>(io_service_server_);
    fib_acceptor.async_accept(
        *p_fiber, std::bind(async_accept_h1, p_fiber, std::placeholders::_1));
  };

  async_accept_h1 = [this, &async_accept_h2, &number_of_accepted,
                     &number_of_connections, &accepted_all](
      std::shared_ptr<fiber> p_fiber,
      const boost::system::error_code& accept_ec) {
    if (!accept_ec) {
      p_fiber->close();
      ++number_of_accepted;

      if (number_of_accepted == number_of_connections) {
        accepted_all.set_value(true);
        return;
      }

      this->io_service_server_.dispatch(async_accept_h2);
    } else {
      ASSERT_EQ(accept_ec.value(), 0)
          << "Accept handler should not be in error: " << accept_ec.value();
    }
  };

  auto async_connect_h1 = [this, &number_of_connected, &number_of_connections,
                           &connected_all](
      std::shared_ptr<fiber> p_fiber,
      const boost::system::error_code& connect_ec) {
    if (!connect_ec) {
      p_fiber->close();
      ++number_of_connected;

      if (number_of_connected == number_of_connections) {
        connected_all.set_value(true);
      }
    } else {
      ASSERT_EQ(connect_ec.value(), 0)
          << "Connect handler should not be in error";
    }
  };

  boost::system::error_code ec_server;
  fiber_endpoint fib_server_endpoint(
      boost::asio::fiber::stream_fiber<socket>::v1(), demux_server_, 1);
  fib_acceptor.open(fib_server_endpoint.protocol());
  fib_acceptor.bind(fib_server_endpoint, ec_server);
  fib_acceptor.listen();
  async_accept_h2();

  fiber_endpoint fib_client_endpoint(
      boost::asio::fiber::stream_fiber<socket>::v1(), demux_client_, 1);
  for (std::size_t i = 0; i < number_of_connections; ++i) {
    auto p_fiber = std::make_shared<fiber>(io_service_client_);
    p_fiber->async_connect(
        fib_client_endpoint,
        std::bind(async_connect_h1, p_fiber, std::placeholders::_1));
  }

  connected_all.get_future().wait();
  accepted_all.get_future().wait();

  boost::system::error_code close_fib_acceptor_ec;
  fib_acceptor.close(close_fib_acceptor_ec);
}

//-----------------------------------------------------------------------------
TEST_F(FiberTest, ExchangePackets) {
  Wait();

  const int32_t packet_number = 1000;
  std::promise<bool> client_closed;
  std::promise<bool> server_closed;

  fiber_acceptor fib_acceptor(io_service_server_);
  fiber fib_server(io_service_server_);
  fiber fib_client(io_service_client_);

  uint8_t buffer_client[5] = {0};
  uint8_t buffer_server[5] = {0};
  bool result_server = true;
  int server_received = 0;

  std::function<void(const boost::system::error_code& ec, std::size_t)>
      async_receive_h1;
  std::function<void(const boost::system::error_code& ec, std::size_t)>
      async_send_h1;

  async_receive_h1 = [&, this](const boost::system::error_code& ec,
                               std::size_t) {
    ASSERT_EQ(ec.value(), 0);

    result_server &= (buffer_server[0] == 'a');
    result_server &= (buffer_server[1] == 'b');
    result_server &= (buffer_server[2] == 'c');
    result_server &= (buffer_server[3] == 'd');
    result_server &= (buffer_server[4] == 'e');

    buffer_server[0] = '1';
    buffer_server[1] = '2';
    buffer_server[2] = '3';
    buffer_server[3] = '4';
    buffer_server[4] = '5';

    if (++server_received < packet_number) {
      boost::asio::async_write(fib_server, boost::asio::buffer(buffer_server),
                               std::bind(async_send_h1, std::placeholders::_1,
                                         std::placeholders::_2));
    } else {
      fib_server.close();
      server_closed.set_value(true);
    }
  };

  async_send_h1 = [&, this](const boost::system::error_code& ec, std::size_t) {
    ASSERT_EQ(ec.value(), 0);

    boost::asio::async_read(fib_server, boost::asio::buffer(buffer_server),
                            std::bind(async_receive_h1, std::placeholders::_1,
                                      std::placeholders::_2));
  };

  auto async_accept_h = [&, this](const boost::system::error_code& ec) {
    ASSERT_EQ(ec.value(), 0);

    buffer_server[0] = '1';
    buffer_server[1] = '2';
    buffer_server[2] = '3';
    buffer_server[3] = '4';
    buffer_server[4] = '5';

    boost::asio::async_read(fib_server, boost::asio::buffer(buffer_server),
                            std::bind(async_receive_h1, std::placeholders::_1,
                                      std::placeholders::_2));
  };

  std::function<void(const boost::system::error_code& ec, std::size_t)>
      async_receive_h2;
  std::function<void(const boost::system::error_code& ec, std::size_t)>
      async_send_h2;

  async_receive_h2 = [&, this](const boost::system::error_code& ec,
                               std::size_t) {
    if (!ec) {
      buffer_client[0] = 'a';
      buffer_client[1] = 'b';
      buffer_client[2] = 'c';
      buffer_client[3] = 'd';
      buffer_client[4] = 'e';

      boost::asio::async_write(fib_client, boost::asio::buffer(buffer_client),
                               std::bind(async_send_h2, std::placeholders::_1,
                                         std::placeholders::_2));
    } else {
      fib_client.close();
      client_closed.set_value(true);
    }
  };

  async_send_h2 = [&, this](const boost::system::error_code& ec, std::size_t) {
    ASSERT_EQ(ec.value(), 0);
    boost::asio::async_read(fib_client, boost::asio::buffer(buffer_client),
                            std::bind(async_receive_h2, std::placeholders::_1,
                                      std::placeholders::_2));
  };

  auto async_connect_h = [&, this](const boost::system::error_code& ec) {
    ASSERT_EQ(ec.value(), 0);
    buffer_client[0] = 'a';
    buffer_client[1] = 'b';
    buffer_client[2] = 'c';
    buffer_client[3] = 'd';
    buffer_client[4] = 'e';

    boost::asio::async_write(
        fib_client, boost::asio::buffer(buffer_client),
        std::bind(async_send_h2, std::placeholders::_1, std::placeholders::_2));
  };

  boost::system::error_code acceptor_ec;
  fiber_endpoint server_endpoint(boost::asio::fiber::stream_fiber<socket>::v1(),
                                 demux_server_, 1);
  fib_acceptor.open(server_endpoint.protocol(), acceptor_ec);
  fib_acceptor.bind(server_endpoint, acceptor_ec);
  fib_acceptor.listen(boost::asio::socket_base::max_connections, acceptor_ec);
  fib_acceptor.async_accept(fib_server,
                            std::bind(async_accept_h, std::placeholders::_1));

  fiber_endpoint client_endpoint(boost::asio::fiber::stream_fiber<socket>::v1(),
                                 demux_client_, 1);
  fib_client.async_connect(client_endpoint,
                           std::bind(async_connect_h, std::placeholders::_1));

  client_closed.get_future().wait();
  server_closed.get_future().wait();

  ASSERT_EQ(true, result_server);
  ASSERT_EQ(packet_number, server_received);
  boost::system::error_code close_fib_acceptor_ec;
  fib_acceptor.close(close_fib_acceptor_ec);
}

////-----------------------------------------------------------------------------
TEST_F(FiberTest, ExchangePacketsFiveClients) {
  Wait();

  typedef std::array<uint8_t, 5> buffer_type;

  struct TestConnection {
    TestConnection(boost::asio::io_service& io_service_server,
                   boost::asio::io_service& io_service_client)
        : fib_server(io_service_server), fib_client(io_service_client) {}

    fiber fib_server;
    fiber fib_client;
    buffer_type buffer_server;
    buffer_type buffer_client;
    std::promise<bool> server_closed;
    std::promise<bool> client_closed;
  };

  uint32_t connection_number = 5;
  int32_t packet_number = 5 * 100;

  fiber_acceptor fib_acceptor(io_service_server_);

  std::list<TestConnection> test_connections;

  std::recursive_mutex server_received_mutex;
  int32_t server_received = 0;

  std::atomic<bool> finished(false);

  std::function<void(const boost::system::error_code&, std::size_t,
                     std::promise<bool>&, fiber&, buffer_type&)>
      async_receive_h1;
  std::function<void(const boost::system::error_code&, std::size_t,
                     std::promise<bool>&, fiber&, buffer_type&)>
      async_send_h1;

  async_receive_h1 = [&](const boost::system::error_code& ec, std::size_t,
                         std::promise<bool>& closed, fiber& fib,
                         buffer_type& buffer_server) {

    if (ec) {
      std::unique_lock<std::recursive_mutex> lock_server_received(
          server_received_mutex);
      ASSERT_EQ(packet_number, server_received);
      return;
    }

    ASSERT_EQ(buffer_server[0], 'a');
    ASSERT_EQ(buffer_server[1], 'b');
    ASSERT_EQ(buffer_server[2], 'c');
    ASSERT_EQ(buffer_server[3], 'd');
    ASSERT_EQ(buffer_server[4], 'e');

    buffer_server[0] = '1';
    buffer_server[1] = '2';
    buffer_server[2] = '3';
    buffer_server[3] = '4';
    buffer_server[4] = '5';

    std::unique_lock<std::recursive_mutex> lock(server_received_mutex);
    if (++server_received < packet_number - 4) {
      boost::asio::async_write(
          fib, boost::asio::buffer(buffer_server),
          std::bind(async_send_h1, std::placeholders::_1, std::placeholders::_2,
                    std::ref(closed), std::ref(fib), std::ref(buffer_server)));
    } else if (server_received < packet_number) {
      fib.close();
      closed.set_value(true);
    } else {
      finished = true;
      fib.close();
      closed.set_value(true);
    }
  };

  async_send_h1 = [&](const boost::system::error_code& ec, std::size_t,
                      std::promise<bool>& closed, fiber& fib,
                      buffer_type& buffer_server) {
    ASSERT_EQ(ec.value(), 0);
    boost::asio::async_read(fib, boost::asio::buffer(buffer_server),
                            std::bind(async_receive_h1, std::placeholders::_1,
                                      std::placeholders::_2, std::ref(closed),
                                      std::ref(fib), std::ref(buffer_server)));
  };

  auto async_accept_h = [&, this](const boost::system::error_code& ec,
                                  std::promise<bool>& closed, fiber& fib,
                                  buffer_type& buffer_server) {
    EXPECT_EQ(ec.value(), 0);

    buffer_server[0] = '1';
    buffer_server[1] = '2';
    buffer_server[2] = '3';
    buffer_server[3] = '4';
    buffer_server[4] = '5';

    boost::asio::async_read(fib, boost::asio::buffer(buffer_server),
                            std::bind(async_receive_h1, std::placeholders::_1,
                                      std::placeholders::_2, std::ref(closed),
                                      std::ref(fib), std::ref(buffer_server)));
  };

  std::function<void(const boost::system::error_code&, std::size_t,
                     std::promise<bool>&, fiber&, buffer_type&)>
      async_receive_h2;
  std::function<void(const boost::system::error_code&, std::size_t,
                     std::promise<bool>&, fiber&, buffer_type&)>
      async_send_h2;

  async_receive_h2 = [&](const boost::system::error_code& ec, std::size_t,
                         std::promise<bool>& closed, fiber& fib,
                         buffer_type& buffer_client) {
    if (!ec) {
      ASSERT_EQ(buffer_client[0], '1');
      ASSERT_EQ(buffer_client[1], '2');
      ASSERT_EQ(buffer_client[2], '3');
      ASSERT_EQ(buffer_client[3], '4');
      ASSERT_EQ(buffer_client[4], '5');
      buffer_client[0] = 'a';
      buffer_client[1] = 'b';
      buffer_client[2] = 'c';
      buffer_client[3] = 'd';
      buffer_client[4] = 'e';

      boost::asio::async_write(
          fib, boost::asio::buffer(buffer_client),
          std::bind(async_send_h2, std::placeholders::_1, std::placeholders::_2,
                    std::ref(closed), std::ref(fib), std::ref(buffer_client)));
    } else {
      fib.close();
      closed.set_value(true);
    }
  };

  async_send_h2 = [&](const boost::system::error_code& ec, std::size_t,
                      std::promise<bool>& closed, fiber& fib,
                      buffer_type& buffer_client) {
    EXPECT_EQ(ec.value(), 0);
    boost::asio::async_read(fib, boost::asio::buffer(buffer_client),
                            std::bind(async_receive_h2, std::placeholders::_1,
                                      std::placeholders::_2, std::ref(closed),
                                      std::ref(fib), std::ref(buffer_client)));
  };

  auto async_connect_h = [&](std::promise<bool>& closed, fiber& fib,
                             buffer_type& buffer_client,
                             const boost::system::error_code& ec) {
    ASSERT_EQ(ec.value(), 0);

    buffer_client[0] = 'a';
    buffer_client[1] = 'b';
    buffer_client[2] = 'c';
    buffer_client[3] = 'd';
    buffer_client[4] = 'e';

    boost::asio::async_write(
        fib, boost::asio::buffer(buffer_client),
        std::bind(async_send_h2, std::placeholders::_1, std::placeholders::_2,
                  std::ref(closed), std::ref(fib), std::ref(buffer_client)));
  };

  boost::system::error_code acceptor_ec;
  fiber_endpoint server_endpoint(boost::asio::fiber::stream_fiber<socket>::v1(),
                                 demux_server_, 1);
  fib_acceptor.open(server_endpoint.protocol(), acceptor_ec);
  fib_acceptor.bind(server_endpoint, acceptor_ec);
  fib_acceptor.listen(boost::asio::socket_base::max_connections, acceptor_ec);

  fiber_endpoint client_endpoint(boost::asio::fiber::stream_fiber<socket>::v1(),
                                 demux_client_, 1);

  for (std::size_t i = 0; i < connection_number; ++i) {
    test_connections.emplace_front(io_service_server_, io_service_client_);
    auto& test_connection = test_connections.front();
    fib_acceptor.async_accept(
        test_connection.fib_server,
        std::bind(async_accept_h, std::placeholders::_1,
                  std::ref(test_connection.server_closed),
                  std::ref(test_connection.fib_server),
                  std::ref(test_connection.buffer_server)));
    test_connection.fib_client.async_connect(
        client_endpoint,
        std::bind(async_connect_h, std::ref(test_connection.client_closed),
                  std::ref(test_connection.fib_client),
                  std::ref(test_connection.buffer_client),
                  std::placeholders::_1));
  }

  for (auto& test_connection : test_connections) {
    test_connection.client_closed.get_future().wait();
    test_connection.server_closed.get_future().wait();
  }

  ASSERT_EQ(packet_number, server_received);
  boost::system::error_code close_fib_acceptor_ec;
  fib_acceptor.close(close_fib_acceptor_ec);

  ASSERT_EQ(finished, true) << "Test did not finish completely";
}

//-----------------------------------------------------------------------------
TEST_F(FiberTest, TooSmallReceiveBuffer) {
  Wait();

  std::array<uint8_t, 5> buffer_client;
  std::array<uint8_t, 2> buffer_server;

  std::recursive_mutex received_mutex;
  uint8_t count = 0;
  size_t received = 0;
  size_t sent = 0;

  fiber_acceptor fib_acceptor(io_service_server_);
  fiber fib_server(io_service_server_);
  fiber fib_client(io_service_client_);

  std::promise<bool> client_closed;
  std::promise<bool> server_closed;

  auto void_handler_receive = [&](const boost::system::error_code& ec,
                                  size_t s) {
    std::unique_lock<std::recursive_mutex> lock(received_mutex);
    received += s;
    ++count;

    if (count == 4) {
      ASSERT_EQ(received, 5) << "Not received all data";
      ASSERT_EQ(fib_server.is_open(), false) << "Server fiber not closed";
      server_closed.set_value(true);
    }
  };

  auto async_accept_h1 = [&, this](const boost::system::error_code& ec) {
    ASSERT_EQ(ec.value(), 0);
    boost::asio::async_read(
        fib_server, boost::asio::buffer(buffer_server),
        std::bind(void_handler_receive, std::placeholders::_1,
                  std::placeholders::_2));
    boost::asio::async_read(
        fib_server, boost::asio::buffer(buffer_server),
        std::bind(void_handler_receive, std::placeholders::_1,
                  std::placeholders::_2));
    boost::asio::async_read(
        fib_server, boost::asio::buffer(buffer_server),
        std::bind(void_handler_receive, std::placeholders::_1,
                  std::placeholders::_2));
    boost::asio::async_read(
        fib_server, boost::asio::buffer(buffer_server),
        std::bind(void_handler_receive, std::placeholders::_1,
                  std::placeholders::_2));
  };

  auto void_handler_send = [&](const boost::system::error_code& ec, size_t s) {
    EXPECT_EQ(ec.value(), 0);
    if (ec) {
      client_closed.set_value(false);
      return;
    }
    sent += s;

    fib_client.close();
    client_closed.set_value(true);
  };

  auto async_connect_h1 = [&, this](const boost::system::error_code& ec) {
    ASSERT_EQ(ec.value(), 0);
    buffer_client[0] = 'a';
    buffer_client[1] = 'b';
    buffer_client[2] = 'c';
    buffer_client[3] = 'd';
    buffer_client[4] = 'e';

    boost::asio::async_write(fib_client, boost::asio::buffer(buffer_client),
                             std::bind(void_handler_send, std::placeholders::_1,
                                       std::placeholders::_2));
  };

  boost::system::error_code acceptor_ec;
  fiber_endpoint server_endpoint(boost::asio::fiber::stream_fiber<socket>::v1(),
                                 demux_server_, 1);
  fib_acceptor.open(server_endpoint.protocol(), acceptor_ec);
  fib_acceptor.bind(server_endpoint, acceptor_ec);
  fib_acceptor.listen(boost::asio::socket_base::max_connections, acceptor_ec);
  fib_acceptor.async_accept(fib_server,
                            std::bind(async_accept_h1, std::placeholders::_1));

  fiber_endpoint client_endpoint(boost::asio::fiber::stream_fiber<socket>::v1(),
                                 demux_client_, 1);
  fib_client.async_connect(client_endpoint,
                           std::bind(async_connect_h1, std::placeholders::_1));

  client_closed.get_future().wait();
  server_closed.get_future().wait();

  { std::unique_lock<std::recursive_mutex> lock(received_mutex); }

  EXPECT_EQ(received, 5);
  EXPECT_EQ(sent, 5);
  boost::system::error_code close_fib_acceptor_ec;
  fib_acceptor.close(close_fib_acceptor_ec);
}

//-----------------------------------------------------------------------------
TEST_F(FiberTest, UDPfiber) {
  Wait();

  uint32_t rem_p = (1 << 16) + 1;

  dgr_fiber_endpoint endpoint_server_local_port(
      boost::asio::fiber::datagram_fiber<socket>::v1(), demux_server_, rem_p);
  dgr_fiber_endpoint endpoint_client_remote_port(
      boost::asio::fiber::datagram_fiber<socket>::v1(), demux_client_, rem_p);
  dgr_fiber dgr_f_server(io_service_server_);
  dgr_fiber dgr_f_client(io_service_client_);
  std::array<uint8_t, 5> buffer_client;

  std::promise<bool> client_closed;
  std::promise<bool> server_closed;

  boost::system::error_code ec;

  dgr_fiber_endpoint endpoint_server_from(
      boost::asio::fiber::datagram_fiber<socket>::v1(), demux_server_);
  std::array<uint8_t, 5> buffer_server;

  auto sent_server = [&](const boost::system::error_code& sent_ec,
                         size_t length) {
    ASSERT_EQ(sent_ec.value(), 0) << "Sent handler should not be in error";
    dgr_f_server.close();
    server_closed.set_value(true);
  };

  auto received_server = [&](const boost::system::error_code& received_ec,
                             size_t length) {
    ASSERT_EQ(received_ec.value(), 0)
        << "Received handler should not be in error";
    EXPECT_EQ(buffer_client[0], buffer_server[0]);
    EXPECT_EQ(buffer_client[1], buffer_server[1]);
    EXPECT_EQ(buffer_client[2], buffer_server[2]);
    EXPECT_EQ(buffer_client[3], buffer_server[3]);
    EXPECT_EQ(buffer_client[4], buffer_server[4]);

    buffer_server[0] = 10;
    buffer_server[1] = 11;
    buffer_server[2] = 12;
    buffer_server[3] = 13;
    buffer_server[4] = 14;

    dgr_f_server.async_send_to(
        boost::asio::buffer(buffer_server), endpoint_server_from,
        std::bind(sent_server, std::placeholders::_1, std::placeholders::_2));
  };

  dgr_f_server.open(endpoint_server_local_port.protocol(), ec);
  dgr_f_server.bind(endpoint_server_local_port, ec);
  ASSERT_EQ(ec.value(), 0);

  dgr_f_server.async_receive_from(
      boost::asio::buffer(buffer_server), endpoint_server_from,
      std::bind(received_server, std::placeholders::_1, std::placeholders::_2));

  buffer_client[0] = 1;
  buffer_client[1] = 2;
  buffer_client[2] = 3;
  buffer_client[3] = 4;
  buffer_client[4] = 5;

  dgr_fiber_endpoint endpoint_client_new_remote_port(
      boost::asio::fiber::datagram_fiber<socket>::v1(), demux_client_);

  auto received_client = [&](const boost::system::error_code& received_ec,
                             size_t length) {
    ASSERT_EQ(received_ec.value(), 0)
        << "Received handler should not be in error";
    EXPECT_EQ(buffer_client[0], buffer_server[0]);
    EXPECT_EQ(buffer_client[1], buffer_server[1]);
    EXPECT_EQ(buffer_client[2], buffer_server[2]);
    EXPECT_EQ(buffer_client[3], buffer_server[3]);
    EXPECT_EQ(buffer_client[4], buffer_server[4]);

    ASSERT_EQ(endpoint_client_new_remote_port.port(),
              endpoint_client_remote_port.port())
        << "Endpoint ports should be equal";

    dgr_f_client.close();
    client_closed.set_value(true);
  };

  auto sent_client = [&](const boost::system::error_code& sent_ec,
                         size_t length) {
    ASSERT_EQ(sent_ec.value(), 0) << "Sent handler should not be in error";
    dgr_f_client.async_receive_from(
        boost::asio::buffer(buffer_client), endpoint_client_new_remote_port,
        std::bind(received_client, std::placeholders::_1,
                  std::placeholders::_2));
  };

  dgr_f_client.async_send_to(
      boost::asio::buffer(buffer_client), endpoint_client_remote_port,
      std::bind(sent_client, std::placeholders::_1, std::placeholders::_2));

  client_closed.get_future().wait();
  server_closed.get_future().wait();
}

//----------------------------------------------------------------------------
TEST_F(FiberTest, TLSConnectDisconnectFiberFromClient) {
  Wait();

  typedef boost::asio::ssl::stream<fiber> ssl_fiber_t;
  boost::asio::ssl::context ctx_client(boost::asio::ssl::context::tlsv12);
  boost::asio::ssl::context ctx_server(boost::asio::ssl::context::tlsv12);

  std::function<bool(bool, boost::asio::ssl::verify_context&)> verify_callback =
      [](bool preverified, boost::asio::ssl::verify_context& ctx) {
        SSF_LOG(kLogDebug) << "------------------------------";
        X509_STORE_CTX* cts = ctx.native_handle();
        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());

        char subject_name[256];
        auto err = X509_STORE_CTX_get_error(ctx.native_handle());
        auto depth_err = X509_STORE_CTX_get_error_depth(ctx.native_handle());

        SSF_LOG(kLogDebug) << "Error " << X509_verify_cert_error_string(err)
                           << std::endl;
        SSF_LOG(kLogDebug) << "Depth " << depth_err;

        X509* issuer = cts->current_issuer;
        if (issuer) {
          X509_NAME_oneline(X509_get_subject_name(issuer), subject_name, 256);
          SSF_LOG(kLogDebug) << "Issuer " << subject_name;
        }

        X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
        SSF_LOG(kLogDebug) << "Verifying " << subject_name;

        SSF_LOG(kLogDebug) << "------------------------------";

        return preverified;
      };

  ctx_client.set_verify_depth(100);
  ctx_server.set_verify_depth(100);

  // Set the mutual authentication
  ctx_client.set_verify_mode(boost::asio::ssl::verify_peer |
                             boost::asio::ssl::verify_fail_if_no_peer_cert);
  ctx_server.set_verify_mode(boost::asio::ssl::verify_peer |
                             boost::asio::ssl::verify_fail_if_no_peer_cert);

  // Set the callback to verify the cetificate chains of the peer
  ctx_client.set_verify_callback(
      std::bind(verify_callback, std::placeholders::_1, std::placeholders::_2));
  ctx_server.set_verify_callback(
      std::bind(verify_callback, std::placeholders::_1, std::placeholders::_2));

  // Load the file containing the trusted certificate authorities
  ctx_client.add_certificate_authority(
      boost::asio::buffer(ssf::tests::GetCaCert()));
  ctx_server.add_certificate_authority(
      boost::asio::buffer(ssf::tests::GetCaCert()));

  // The certificate used by the local peer
  ctx_client.use_certificate_chain(
      boost::asio::buffer(ssf::tests::GetClientCert()));
  ctx_server.use_certificate_chain(
      boost::asio::buffer(ssf::tests::GetServerCert()));

  // The private key used by the local peer
  ctx_client.use_private_key(boost::asio::buffer(ssf::tests::GetClientKey()),
                             boost::asio::ssl::context::pem);
  ctx_server.use_private_key(boost::asio::buffer(ssf::tests::GetServerKey()),
                             boost::asio::ssl::context::pem);

  // The Diffie-Hellman parameter file
  ctx_server.use_tmp_dh(boost::asio::buffer(ssf::tests::GetServerDhParam()));

  // Force a specific cipher suite
  SSL_CTX_set_cipher_list(ctx_client.native_handle(),
                          "DHE-RSA-AES256-GCM-SHA384");
  SSL_CTX_set_cipher_list(ctx_server.native_handle(),
                          "DHE-RSA-AES256-GCM-SHA384");

  fiber_acceptor fib_acceptor(io_service_server_);

  ssl_fiber_t ssl_fiber_client(io_service_client_, ctx_client);
  ssl_fiber_t ssl_fiber_server(io_service_server_, ctx_server);

  std::promise<bool> client_closed;
  std::promise<bool> server_closed;

  uint32_t buffer_s = 1;
  uint32_t buffer_c = 0;

  auto sent = [this, &server_closed, &ssl_fiber_server, &buffer_s](
      const boost::system::error_code& ec, size_t length) {
    EXPECT_EQ(ec.value(), 0) << "Sent handler should not be in error";
    SSF_LOG(kLogDebug) << "Server sent: " << buffer_s << std::endl;
    server_closed.set_value(true);
  };

  auto async_handshaked_s = [this, &ssl_fiber_server, &buffer_s,
                             &sent](const boost::system::error_code& ec) {
    EXPECT_EQ(ec.value(), 0) << "Handshaked handler should not be in error";
    buffer_s = 10;
    boost::asio::async_write(
        ssl_fiber_server, boost::asio::buffer(&buffer_s, sizeof(buffer_s)),
        std::bind(sent, std::placeholders::_1, std::placeholders::_2));
  };

  auto async_accept_s = [this, &ssl_fiber_server, &async_handshaked_s](
      const boost::system::error_code& ec) {
    EXPECT_EQ(ec.value(), 0) << "Accept handler should not be in error";
    ssl_fiber_server.async_handshake(
        boost::asio::ssl::stream_base::server,
        std::bind(async_handshaked_s, std::placeholders::_1));
  };

  auto received = [this, &client_closed, &ssl_fiber_client, &buffer_c,
                   &buffer_s](const boost::system::error_code& ec,
                              size_t length) {
    EXPECT_EQ(ec.value(), 0) << "Received handler should not be in error "
                             << ec.message();
    EXPECT_EQ(buffer_s, buffer_c);
    SSF_LOG(kLogDebug) << "client received: " << buffer_c << std::endl;
    ssl_fiber_client.next_layer().close();
    client_closed.set_value(true);
  };

  auto async_handshaked_c = [this, &ssl_fiber_client, &buffer_c,
                             &received](const boost::system::error_code& ec) {
    EXPECT_EQ(ec.value(), 0) << "Handshaked handler should not be in error";
    boost::asio::async_read(
        ssl_fiber_client, boost::asio::buffer(&buffer_c, sizeof(buffer_c)),
        std::bind(received, std::placeholders::_1, std::placeholders::_2));
  };

  auto async_connect_c = [this, &ssl_fiber_client, &async_handshaked_c](
      const boost::system::error_code& ec) {
    EXPECT_EQ(ec.value(), 0) << "Connect handler should not be in error";
    ssl_fiber_client.async_handshake(
        boost::asio::ssl::stream_base::client,
        std::bind(async_handshaked_c, std::placeholders::_1));
  };

  boost::system::error_code acceptor_ec;
  fiber_endpoint server_endpoint(boost::asio::fiber::stream_fiber<socket>::v1(),
                                 demux_server_, 1);
  fib_acceptor.open(server_endpoint.protocol(), acceptor_ec);
  fib_acceptor.bind(server_endpoint, acceptor_ec);
  fib_acceptor.listen(boost::asio::socket_base::max_connections, acceptor_ec);
  fib_acceptor.async_accept(ssl_fiber_server.next_layer(),
                            std::bind(async_accept_s, std::placeholders::_1));

  fiber_endpoint client_endpoint(boost::asio::fiber::stream_fiber<socket>::v1(),
                                 demux_client_, 1);
  ssl_fiber_client.next_layer().async_connect(
      client_endpoint, std::bind(async_connect_c, std::placeholders::_1));

  client_closed.get_future().wait();
  server_closed.get_future().wait();

  boost::system::error_code ec;
  ssl_fiber_server.next_layer().close(ec);
  fib_acceptor.close(ec);
}
