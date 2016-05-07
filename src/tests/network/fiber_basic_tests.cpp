#include <vector>
#include <functional>
#include <atomic>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/thread.hpp>

#include <gtest/gtest.h>

#include "common/boost/basic_fiber.h"
#include "common/boost/basic_fiber_datagram.h"
#include "common/boost/basic_fiber_demux.h"
#include "common/boost/basic_fiber_acceptor.h"

class FiberTest : public ::testing::Test {
protected:
 FiberTest()
     : io_service_client(),
       p_client_worker(new boost::asio::io_service::work(io_service_client)),
       socket_client(io_service_client),
       resolver_client(io_service_client),
       query(boost::asio::ip::tcp::v4(), "127.0.0.1", "8011"),
       iterator_client(resolver_client.resolve(query)),
       demux_client(io_service_client),
       io_service_server(),
       socket_server(io_service_server),
       acceptor_server(io_service_server),
       resolver(io_service_server),
       query_server("127.0.0.1", "8011"),
       endpoint(*resolver.resolve(query_server)),
       demux_server(io_service_server),
       fib_acceptor_serv(demux_server),
       fib_server1(demux_server, 0),
       fib_client1(demux_client, 1) {}

  ~FiberTest() {
  
  }

  typedef boost::asio::ip::tcp::socket socket;
  typedef boost::asio::basic_fiber_demux<socket> fiber_demux;
  typedef boost::asio::basic_fiber_acceptor<socket> fiber_acceptor;
  typedef boost::asio::basic_fiber<socket> fiber;
  typedef boost::asio::basic_datagram_fiber<socket> dgr_fiber;
  typedef std::function<void(const boost::system::error_code&)> accept_handler_type;

  void startServer(accept_handler_type accept_h) {
    boost::asio::socket_base::reuse_address option(true);
    
    acceptor_server.open(endpoint.protocol());
    acceptor_server.set_option(option);
    acceptor_server.bind(endpoint);
    acceptor_server.listen();
    acceptor_server.async_accept(socket_server, accept_h);

    for (uint8_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
      auto lambda = [&]() {
        boost::system::error_code ec;
        try {
          io_service_server.run(ec);
        } catch (std::exception e) {
          std::cout << "Exception: " << e.what() << std::endl;
        }
      };

      serverThreads_.create_thread(lambda);
    }
  }

  void serverStop() {
    p_server_worker.reset();
    demux_server.close();
    socket_server.close();
    acceptor_server.close();
    io_service_server.stop();
  }

  void clientStop() {
    p_client_worker.reset();
    demux_client.close();
    socket_client.close();
    io_service_client.stop();
  }

  void connectClient() {
    boost::asio::connect(socket_client, iterator_client);
  }

  void fiberizeServer() {
    demux_server.fiberize(std::move(socket_server));
  }

  void fiberizeClient() {
    demux_client.fiberize(std::move(socket_client));
  }

  virtual void SetUp() {}

  virtual void TearDown() {}

public:

protected:
  boost::thread clientThreads_;
  boost::asio::io_service io_service_client;
  boost::asio::ip::tcp::socket socket_client;
  boost::asio::ip::tcp::resolver resolver_client;
  boost::asio::ip::tcp::resolver::query query;
  boost::asio::ip::tcp::resolver::iterator iterator_client;
  fiber_demux demux_client;
  std::shared_ptr<boost::asio::io_service::work> p_client_worker;

  std::shared_ptr<boost::asio::io_service::work> p_server_worker;
  boost::thread_group serverThreads_;
  boost::asio::io_service io_service_server;
  boost::asio::ip::tcp::socket socket_server;
  boost::asio::ip::tcp::acceptor acceptor_server;
  boost::asio::ip::tcp::resolver resolver;
  boost::asio::ip::tcp::resolver::query query_server;
  boost::asio::ip::tcp::endpoint endpoint;
  fiber_demux demux_server;
  fiber_acceptor fib_acceptor_serv;

  fiber fib_server1;
  fiber fib_client1;
};

//-----------------------------------------------------------------------------
TEST_F(FiberTest, startStopServerBeforeSocket) {
  auto handler = [](const boost::system::error_code&) {};
  startServer(handler);
  serverStop();
  serverThreads_.join_all();
}

//-----------------------------------------------------------------------------
TEST_F(FiberTest, startStopServerBeforeFiberizeFromServer) {
  auto handler = [&](const boost::system::error_code&) {
    serverStop();
  };

  startServer(handler);
  connectClient();
  serverThreads_.join_all();
}

//-----------------------------------------------------------------------------
TEST_F(FiberTest, startStopServerBeforeFiberizeFromClient) {
  auto handler = [&](const boost::system::error_code&) {};

  startServer(handler);
  connectClient();
  socket_client.close();
  serverThreads_.join_all();
  serverStop();
}

//-----------------------------------------------------------------------------
TEST_F(FiberTest, startStopServerAfterFiberizeFromServer) {
  auto handler = [this](const boost::system::error_code&) {
    fiberizeServer();
    serverStop();
  };

  startServer(handler);
  connectClient();
  fiberizeClient();
  serverThreads_.join_all();
}

//-----------------------------------------------------------------------------
TEST_F(FiberTest, startStopServerAfterFiberizeFromClient) {
  auto handler = [&](const boost::system::error_code&) {
    fiberizeServer();
  };

  startServer(handler);
  connectClient();
  fiberizeClient();
  clientStop();
  serverStop();
  serverThreads_.join_all();
}


//-----------------------------------------------------------------------------
TEST_F(FiberTest, connectDisconnectFiberFromServer) {
  const uint32_t number_of_connections = 1000;
  std::atomic<uint32_t> number_of_connected = 0;

  std::function<void(std::shared_ptr<fiber> p_fiber, const boost::system::error_code&)> async_accept_h1;
  std::function<void()> async_accept_h2;
  std::function<void(const boost::system::error_code&)> handler;

  async_accept_h2 = [this, &async_accept_h1]() {
    auto p_fiber = std::make_shared<fiber>(demux_server, 0);
    fib_acceptor_serv.async_accept(*p_fiber, boost::bind<void>(boost::ref(async_accept_h1), p_fiber, _1));
  };

  async_accept_h1 = [this, &async_accept_h2](std::shared_ptr<fiber> p_fiber, const boost::system::error_code&) {
    p_fiber->close();
    this->io_service_server.dispatch(async_accept_h2);
  };

  handler = [this, &async_accept_h2](const boost::system::error_code&) {
    boost::system::error_code ec_server;

    fiberizeServer();
    fib_acceptor_serv.bind(1, ec_server);
    fib_acceptor_serv.listen(1, ec_server);

    async_accept_h2();
  };

  boost::system::error_code ec_client;
  auto async_connect_h1 = [this, &number_of_connected, &number_of_connections](
      std::shared_ptr<fiber> p_fiber, const boost::system::error_code& ec) {
    p_fiber->close();
    ++number_of_connected;

    if (number_of_connected.load() == number_of_connections) {
      serverStop();
      clientStop();
    }
  };

  startServer(handler);
  connectClient();
  fiberizeClient();

  boost::thread_group clientThreads;
  for (uint8_t i = 0; i < boost::thread::hardware_concurrency(); ++i) {
    auto lambda = [&]() {
      boost::system::error_code ec;
      try {
        io_service_client.run(ec);
      } catch (std::exception e) {
        std::cout << "Exception: " << e.what() << std::endl;
      }
    };
    clientThreads.create_thread(lambda);
  }


  for (size_t i = 0; i < number_of_connections; ++i) {
    auto p_fiber = std::make_shared<fiber>(demux_client, 1);
    p_fiber->async_connect(boost::bind<void>(async_connect_h1, p_fiber, _1));
  }

  serverThreads_.join_all();
  clientThreads.join_all();
}

//-----------------------------------------------------------------------------
TEST_F(FiberTest, connectDisconnectFiberFromClient) {
  auto handler = [this](const boost::system::error_code&) {
    boost::system::error_code ec_server;

    auto async_accept_h1 = [this](const boost::system::error_code&) {};

    fiberizeServer();
    fib_acceptor_serv.bind(1, ec_server);
    fib_acceptor_serv.listen(1, ec_server);
    fib_acceptor_serv.async_accept(fib_server1, async_accept_h1);
  };

  boost::system::error_code ec_client;
  auto async_connect_h1 = [this](const boost::system::error_code&) {
    fib_client1.close();
    clientStop();
    serverStop();
  };

  startServer(handler);
  connectClient();
  fiberizeClient();
  fib_client1.async_connect(async_connect_h1);
  io_service_client.run();
  serverThreads_.join_all();
}

//-----------------------------------------------------------------------------
TEST_F(FiberTest, MTU) {
  uint8_t buffer_server[100 * 1024] = { 0 };
  uint8_t buffer_client[100 * 1024] = { 0 };
  bool result_server = true;
  boost::system::error_code ec_client;

  std::function<void(const boost::system::error_code& ec, size_t)> async_receive_h1;

  async_receive_h1 = [&, this](const boost::system::error_code& ec, size_t length) {
    ASSERT_EQ(!ec, true);
    for (size_t i = 0; i < 50 * 1024; ++i) {
      result_server &= (buffer_server[i] == (i % 256));
    }

    ASSERT_EQ(result_server, true);

    for (size_t i = 50 * 1024; i < 100 * 1024; ++i) {
      result_server &= (buffer_server[i] == (i % 256));
    }

    fib_server1.close([this](const boost::system::error_code&) {
      clientStop();
      serverStop();
    });
  };

  auto async_accept_h = [&, this](const boost::system::error_code& ec) {
    ASSERT_EQ(!ec, true);
    for (size_t i = 0; i < 1024; ++i) {
      buffer_server[i] = (i+11) % 256;
    }

    boost::asio::async_read(fib_server1, boost::asio::buffer(buffer_server),
                            async_receive_h1);
  };

  auto handler_server = [&, this](const boost::system::error_code& ec) {
    ASSERT_EQ(!ec, true);
    boost::system::error_code ec_server;

    fiberizeServer();
    fib_acceptor_serv.bind(1, ec_server);
    fib_acceptor_serv.listen(1, ec_server);
    fib_acceptor_serv.async_accept(fib_server1, async_accept_h);

  };

  std::function<void(const boost::system::error_code& ec, size_t)> async_send_h2;

  async_send_h2 = [&, this](const boost::system::error_code& ec, size_t) {
    ASSERT_EQ(!ec, true);
  };

  auto async_connect_h = [&, this](const boost::system::error_code& ec) {
    ASSERT_EQ(!ec, true);
    for (size_t i = 0; i < 100 * 1024; ++i) {
      buffer_client[i] = i % 256;
    }

    boost::asio::async_write(fib_client1, boost::asio::buffer(buffer_client),
                             async_send_h2);
  };

  startServer(handler_server);
  connectClient();
  fiberizeClient();
  fib_client1.async_connect(async_connect_h);
  boost::system::error_code ec;
  io_service_client.run(ec);
  SSF_LOG(kLogDebug) << ec.message() << " " << ec.value();
  serverThreads_.join_all();

  ASSERT_EQ(true, result_server);
}

//-----------------------------------------------------------------------------
TEST_F(FiberTest, exchangePackets) {
  const int32_t packet_number = 1000;

  uint8_t buffer_client[5] = { 0 };
  uint8_t buffer_server[5] = { 0 };
  bool result_server = true;
  boost::system::error_code ec_client;
  int server_received = 0;

  std::function<void(const boost::system::error_code& ec, size_t)> async_receive_h1;
  std::function<void(const boost::system::error_code& ec, size_t)> async_send_h1;

  async_receive_h1 = [&, this](const boost::system::error_code& ec, size_t) {
    ASSERT_EQ(!ec, true);
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
      boost::asio::async_write(fib_server1, boost::asio::buffer(buffer_server),
                               async_send_h1);
    }
    else {
      fib_server1.close([this](const boost::system::error_code&) {
        clientStop();
        serverStop();
      });
    }
  };

  async_send_h1 = [&, this](const boost::system::error_code& ec, size_t) {
    ASSERT_EQ(!ec, true);
    boost::asio::async_read(fib_server1, boost::asio::buffer(buffer_server),
                            async_receive_h1);
  };

  auto async_accept_h = [&, this](const boost::system::error_code& ec) {
    ASSERT_EQ(!ec, true);
    buffer_server[0] = '1';
    buffer_server[1] = '2';
    buffer_server[2] = '3';
    buffer_server[3] = '4';
    buffer_server[4] = '5';

    boost::asio::async_read(fib_server1, boost::asio::buffer(buffer_server),
                            async_receive_h1);
  };

  auto handler_server = [&, this](const boost::system::error_code& ec) {
    ASSERT_EQ(!ec, true);
    boost::system::error_code ec_server;

    fiberizeServer();
    fib_acceptor_serv.bind(1, ec_server);
    fib_acceptor_serv.listen(1, ec_server);
    fib_acceptor_serv.async_accept(fib_server1, async_accept_h);

  };

  std::function<void(const boost::system::error_code& ec, size_t)> async_receive_h2;
  std::function<void(const boost::system::error_code& ec, size_t)> async_send_h2;

  async_receive_h2 = [&, this](const boost::system::error_code& ec, size_t) {
    if (!ec) {
      buffer_client[0] = 'a';
      buffer_client[1] = 'b';
      buffer_client[2] = 'c';
      buffer_client[3] = 'd';
      buffer_client[4] = 'e';

      boost::asio::async_write(fib_client1, boost::asio::buffer(buffer_client),
                               async_send_h2);
    } else {
      fib_client1.close();
    }
  };

  async_send_h2 = [&, this](const boost::system::error_code& ec, size_t) {
    ASSERT_EQ(!ec, true);
    boost::asio::async_read(fib_client1, boost::asio::buffer(buffer_client),
                            async_receive_h2);
  };

  auto async_connect_h = [&, this](const boost::system::error_code& ec) {
    ASSERT_EQ(!ec, true);
    buffer_client[0] = 'a';
    buffer_client[1] = 'b';
    buffer_client[2] = 'c';
    buffer_client[3] = 'd';
    buffer_client[4] = 'e';

    boost::asio::async_write(fib_client1, boost::asio::buffer(buffer_client),
                             async_send_h2);
  };

  startServer(handler_server);
  connectClient();
  fiberizeClient();
  fib_client1.async_connect(async_connect_h);
  boost::system::error_code ec;
  io_service_client.run(ec);
  SSF_LOG(kLogDebug) << ec.message() << " " << ec.value() << std::endl;
  serverThreads_.join_all();

  ASSERT_EQ(true, result_server);
}

//-----------------------------------------------------------------------------
TEST_F(FiberTest, exchangePacketsFiveClients) {
  typedef std::array<uint8_t, 5> buffer_type;

  buffer_type buffer_client1;
  buffer_type buffer_client2;
  buffer_type buffer_client3;
  buffer_type buffer_client4;
  buffer_type buffer_client5;

  buffer_type buffer_server1;
  buffer_type buffer_server2;
  buffer_type buffer_server3;
  buffer_type buffer_server4;
  buffer_type buffer_server5;

  boost::system::error_code ec_client;
  boost::recursive_mutex server_received_mutex;
  int32_t server_received = 0;
  int32_t number_of_packets = 5 * 100;

  fiber fib_server2(demux_server, 0);
  fiber fib_server3(demux_server, 0);
  fiber fib_server4(demux_server, 0);
  fiber fib_server5(demux_server, 0);

  fiber fib_client2(demux_client, 1);
  fiber fib_client3(demux_client, 1);
  fiber fib_client4(demux_client, 1);
  fiber fib_client5(demux_client, 1);

  std::function<void(const boost::system::error_code & ec, size_t, fiber & fib,
                     buffer_type&)> async_receive_h1;
  std::function<void(const boost::system::error_code & ec, size_t, fiber & fib,
                     buffer_type&)> async_send_h1;

  async_receive_h1 = [&](const boost::system::error_code& ec, size_t,
                         fiber& fib, buffer_type& buffer_server) {
    

    if (ec) {
      boost::recursive_mutex::scoped_lock lock(server_received_mutex);
      ASSERT_EQ(number_of_packets, server_received);
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

    boost::recursive_mutex::scoped_lock lock(server_received_mutex);
    if (++server_received < number_of_packets - 4) {
      boost::asio::async_write(
          fib, boost::asio::buffer(buffer_server),
          boost::bind(async_send_h1, _1, _2, boost::ref(fib),
                      boost::ref(buffer_server)));
    } else if (server_received < number_of_packets) {
      fib.close([this](const boost::system::error_code&) {});
    } else {
      fib.close([this](const boost::system::error_code&) {
        clientStop();
        serverStop();
      });
    }
  };

  async_send_h1 = [&](const boost::system::error_code& ec, size_t, fiber& fib,
                      buffer_type& buffer_server) {
    ASSERT_EQ(!ec, true);
    boost::asio::async_read(
        fib, boost::asio::buffer(buffer_server),
        boost::bind(async_receive_h1, _1, _2, boost::ref(fib),
                    boost::ref(buffer_server)));
  };

  std::function<void(const boost::system::error_code&, fiber&, buffer_type&)>
      async_accept_h = [&](const boost::system::error_code& ec, fiber& fib,
                           buffer_type& buffer_server) {
    EXPECT_EQ(!ec, true);

    buffer_server[0] = '1';
    buffer_server[1] = '2';
    buffer_server[2] = '3';
    buffer_server[3] = '4';
    buffer_server[4] = '5';

    boost::asio::async_read(
        fib, boost::asio::buffer(buffer_server),
        boost::bind(async_receive_h1, _1, _2, boost::ref(fib),
                    boost::ref(buffer_server)));
  };

  auto handler_server = [&](const boost::system::error_code& ec, fiber& fib1,
                            fiber& fib2, fiber& fib3, fiber& fib4,
                            fiber& fib5) {
    EXPECT_EQ(!ec, true);
    boost::system::error_code ec_server;

    fiberizeServer();
    fib_acceptor_serv.bind(1, ec_server);
    fib_acceptor_serv.listen(1, ec_server);
    fib_acceptor_serv.async_accept(
        fib1, boost::bind(async_accept_h, _1, boost::ref(fib1),
                          boost::ref(buffer_server1)));
    fib_acceptor_serv.async_accept(
        fib2, boost::bind(async_accept_h, _1, boost::ref(fib2),
                          boost::ref(buffer_server2)));
    fib_acceptor_serv.async_accept(
        fib3, boost::bind(async_accept_h, _1, boost::ref(fib3),
                          boost::ref(buffer_server3)));
    fib_acceptor_serv.async_accept(
        fib4, boost::bind(async_accept_h, _1, boost::ref(fib4),
                          boost::ref(buffer_server4)));
    fib_acceptor_serv.async_accept(
        fib5, boost::bind(async_accept_h, _1, boost::ref(fib5),
                          boost::ref(buffer_server5)));
  };

  std::function<void(const boost::system::error_code & ec, size_t, fiber & fib,
                     buffer_type&)> async_receive_h2;
  std::function<void(const boost::system::error_code & ec, size_t, fiber & fib,
                     buffer_type&)> async_send_h2;

  async_receive_h2 = [&](const boost::system::error_code& ec, size_t,
                         fiber& fib, buffer_type& buffer_client) {
    if (!ec) {
      buffer_client[0] = 'a';
      buffer_client[1] = 'b';
      buffer_client[2] = 'c';
      buffer_client[3] = 'd';
      buffer_client[4] = 'e';

      boost::asio::async_write(
          fib, boost::asio::buffer(buffer_client),
          boost::bind(async_send_h2, _1, _2, boost::ref(fib),
                      boost::ref(buffer_client)));
    } else {
      fib.close();
    }
  };

  async_send_h2 = [&](const boost::system::error_code& ec, size_t, fiber& fib,
                      buffer_type& buffer_client) {
    boost::asio::async_read(
        fib, boost::asio::buffer(buffer_client),
        boost::bind(async_receive_h2, _1, _2, boost::ref(fib),
                    boost::ref(buffer_client)));
  };

  std::function<void(fiber&, buffer_type&)> async_connect_h = [&](
      fiber& fib, buffer_type& buffer_client) {
    buffer_client[0] = 'a';
    buffer_client[1] = 'b';
    buffer_client[2] = 'c';
    buffer_client[3] = 'd';
    buffer_client[4] = 'e';

    boost::asio::async_write(fib, boost::asio::buffer(buffer_client),
                             boost::bind(async_send_h2, _1, _2, boost::ref(fib),
                                         boost::ref(buffer_client)));
  };

  startServer(
      boost::bind<void>(handler_server, _1, boost::ref(fib_server1),
                        boost::ref(fib_server2), boost::ref(fib_server3),
                        boost::ref(fib_server4), boost::ref(fib_server5)));
  connectClient();
  fiberizeClient();
  fib_client1.async_connect(boost::bind(
      async_connect_h, boost::ref(fib_client1), boost::ref(buffer_client1)));
  fib_client2.async_connect(boost::bind(
      async_connect_h, boost::ref(fib_client2), boost::ref(buffer_client2)));
  fib_client3.async_connect(boost::bind(
      async_connect_h, boost::ref(fib_client3), boost::ref(buffer_client3)));
  fib_client4.async_connect(boost::bind(
      async_connect_h, boost::ref(fib_client4), boost::ref(buffer_client4)));
  fib_client5.async_connect(boost::bind(
      async_connect_h, boost::ref(fib_client5), boost::ref(buffer_client5)));

  boost::thread t1([&]() { io_service_client.run(); });
  boost::thread t2([&]() { io_service_client.run(); });
  boost::thread t3([&]() { io_service_client.run(); });
  boost::thread t4([&]() { io_service_client.run(); });
  boost::thread t5([&]() { io_service_client.run(); });
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();
  serverThreads_.join_all();
}

//-----------------------------------------------------------------------------
TEST_F(FiberTest, tooSmallReceiveBuffer) {
  std::array<uint8_t, 5> buffer_client;
  std::array<uint8_t, 2> buffer_server;

  boost::recursive_mutex received_mutex;
  uint8_t count = 0;
  size_t received = 0;
  size_t sent = 0;

  auto void_handler_receive = [&](const boost::system::error_code& ec, size_t s) {
    boost::recursive_mutex::scoped_lock lock(received_mutex);
    received += s;
    ++count;

    if (count == 4) {
      serverStop();
    }
  };

  auto async_accept_h1 = [&, this](const boost::system::error_code& ec) {
    ASSERT_EQ(!ec, true);
    boost::asio::async_read(fib_server1, boost::asio::buffer(buffer_server),
                            void_handler_receive);
    boost::asio::async_read(fib_server1, boost::asio::buffer(buffer_server),
                            void_handler_receive);
    boost::asio::async_read(fib_server1, boost::asio::buffer(buffer_server),
                            void_handler_receive);
    boost::asio::async_read(fib_server1, boost::asio::buffer(buffer_server),
                            void_handler_receive);
  };

  auto handler = [&, this](const boost::system::error_code& ec) {
    ASSERT_EQ(!ec, true);
    boost::system::error_code ec_server;

    //check how to get a reliable synchronization
    fiberizeServer();
    fib_acceptor_serv.bind(1, ec_server);
    fib_acceptor_serv.listen(1, ec_server);
    fib_acceptor_serv.async_accept(fib_server1, async_accept_h1);
  };

  auto close_handler = [&](const boost::system::error_code& ec) {
    clientStop();
  };

  auto void_handler_send = [&](const boost::system::error_code& ec, size_t s) {
    ASSERT_EQ(!ec, true);
    sent += s;

    fib_client1.close(close_handler);
  };

  auto async_connect_h1 = [&, this](const boost::system::error_code& ec) {
    ASSERT_EQ(!ec, true);

    buffer_client[0] = 'a';
    buffer_client[1] = 'b';
    buffer_client[2] = 'c';
    buffer_client[3] = 'd';
    buffer_client[4] = 'e';

    boost::asio::async_write(fib_client1, boost::asio::buffer(buffer_client),
                             void_handler_send);
  };

  startServer(handler);
  connectClient();
  fiberizeClient();
  fib_client1.async_connect(async_connect_h1);
  io_service_client.run();
  serverThreads_.join_all();

  EXPECT_EQ(received, 5);
  EXPECT_EQ(sent, 5);
}

//-----------------------------------------------------------------------------
TEST_F(FiberTest, UDPfiber) {
  auto handler = [this](const boost::system::error_code&) {
    fiberizeServer();
  };

  startServer(handler);
  connectClient();
  fiberizeClient();

  uint32_t rem_p = (1 << 16) + 1;

  dgr_fiber dgr_f_l(demux_server, 0);
  dgr_fiber dgr_f(demux_client, rem_p);
  std::array<uint8_t, 5> buffer_client;
  boost::system::error_code ec;
  uint32_t from_port;
  std::array<uint8_t, 5> buffer_server;

  std::function<void(const boost::system::error_code&, size_t)>
      sent_server = [&](const boost::system::error_code& ec,
                            size_t length) {
    dgr_f_l.close();
  };

  std::function<void(const boost::system::error_code&, size_t)>
      received_server = [&](const boost::system::error_code& ec,
                            size_t length) {
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

    dgr_f_l.async_send_to(boost::asio::buffer(buffer_server), from_port,
                          sent_server);
  };

  dgr_f_l.bind(rem_p, ec);
  dgr_f_l.async_receive_from(boost::asio::buffer(buffer_server), from_port,
                             received_server);

  buffer_client[0] = 1;
  buffer_client[1] = 2;
  buffer_client[2] = 3;
  buffer_client[3] = 4;
  buffer_client[4] = 5;

  uint32_t new_rem_p = 0;

  std::function<void(const boost::system::error_code&, size_t)>
    received_client = [&](const boost::system::error_code& ec, size_t length) {
    EXPECT_EQ(buffer_client[0], buffer_server[0]);
    EXPECT_EQ(buffer_client[1], buffer_server[1]);
    EXPECT_EQ(buffer_client[2], buffer_server[2]);
    EXPECT_EQ(buffer_client[3], buffer_server[3]);
    EXPECT_EQ(buffer_client[4], buffer_server[4]);


    EXPECT_EQ(new_rem_p, rem_p);

    dgr_f.close();
    clientStop();
    serverStop();
  };

  std::function<void(const boost::system::error_code&, size_t)>
      sent_client = [&](const boost::system::error_code& ec, size_t length) {
    dgr_f.async_receive_from(boost::asio::buffer(buffer_client), new_rem_p,
                             received_client);
  };

  dgr_f.async_send_to(boost::asio::buffer(buffer_client), rem_p, sent_client);
  io_service_client.run();

  serverThreads_.join_all();
}

//-----------------------------------------------------------------------------
//TEST_F(FiberTest, SSLconnectDisconnectFiberFromClient) {
//  typedef boost::asio::ssl::stream<fiber> ssl_fiber_t;
//  boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv12);
//
//  std::function<bool(bool, boost::asio::ssl::verify_context&)> verify_callback =
//      [](bool preverified,
//         boost::asio::ssl::verify_context& ctx) { 
//    std::cout << "------------------------------" << std::endl;
//    X509_STORE_CTX* cts = ctx.native_handle();
//    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
//
//    char subject_name[256];
//    auto err = X509_STORE_CTX_get_error(ctx.native_handle());
//    auto depth_err = X509_STORE_CTX_get_error_depth(ctx.native_handle());
//
//    std::cout << "Error " << X509_verify_cert_error_string(err) << std::endl;
//    std::cout << "Depth " << depth_err << std::endl;
//
//    X509* issuer = cts->current_issuer;
//    if (issuer) {
//      X509_NAME_oneline(X509_get_subject_name(issuer), subject_name, 256);
//      std::cout << "Issuer " << subject_name << "\n";
//    }
//
//    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
//    std::cout << "Verifying " << subject_name << "\n";
//
//    std::cout << "------------------------------" << std::endl;
//
//    return preverified;
//  };
//
//  ctx.set_verify_depth(100);
//
//  // Set the mutual authentication
//  ctx.set_verify_mode(boost::asio::ssl::verify_peer |
//                      boost::asio::ssl::verify_fail_if_no_peer_cert);
//
//  // Set the callback to verify the cetificate chains of the peer
//  ctx.set_verify_callback(
//    boost::bind(verify_callback, _1, _2));
//
//  // Load the file containing the trusted certificate authorities
//  SSL_CTX_load_verify_locations(ctx.native_handle(),
//    "./certs/allowed/allowed.crt", NULL);
//
//  // The certificate used by the local peer
//  ctx.use_certificate_chain_file("./certs/cert.pem");
//
//  // The private key used by the local peer
//  ctx.use_private_key_file("./certs/key.pem", boost::asio::ssl::context::pem);
//
//  // The Diffie-Hellman parameter file
//  ctx.use_tmp_dh_file("./certs/dh4096.pem");
//
//  // Force a specific cipher suite
//  SSL_CTX_set_cipher_list(ctx.native_handle(), "DHE-RSA-AES256-GCM-SHA384");
//
//  ssl_fiber_t ssl_fiber_client(io_service_client, ctx);
//  ssl_fiber_t ssl_fiber_server(io_service_server, ctx);
//
//  ssl_fiber_client.next_layer().open(demux_client, 1);
//  ssl_fiber_server.next_layer().open(demux_server, 0);
//
//  uint32_t buffer_s = 0;
//  uint32_t buffer_c = 0;
//
//  auto sent = [this, &ssl_fiber_client, &buffer_s](
//      const boost::system::error_code& ec, size_t length) {
//    std::cout << "Server sent: " << buffer_s << std::endl;
//  };
//
//  auto async_handshaked_s = [this, &ssl_fiber_server, &buffer_s, sent](
//      const boost::system::error_code& ec) {
//    buffer_s = 10;
//    boost::asio::async_write(ssl_fiber_server,
//                            boost::asio::buffer(&buffer_s, sizeof(buffer_s)),
//                            sent);
//  };
//
//  auto async_accept_s = [this, &ssl_fiber_server, async_handshaked_s](
//      const boost::system::error_code& ec) {
//    ssl_fiber_server.async_handshake(boost::asio::ssl::stream_base::server,
//      async_handshaked_s);
//  };
//
//  auto handler_s = [this, async_accept_s, &ssl_fiber_server](
//      const boost::system::error_code& ec) {
//    boost::system::error_code ec_server;
//    fiberizeServer();
//    fib_acceptor_serv.bind(1, ec_server);
//    fib_acceptor_serv.listen(1, ec_server);
//    fib_acceptor_serv.async_accept(ssl_fiber_server.next_layer(), async_accept_s);
//  };
//
//  auto received = [this, &ssl_fiber_client, &buffer_c, &buffer_s](
//      const boost::system::error_code& ec, size_t length) {
//    ASSERT_EQ(buffer_s, buffer_c);
//    std::cout << "client received: " << buffer_c << std::endl;
//    ssl_fiber_client.next_layer().close();
//    demux_client.close();
//    socket_client.close();
//    serverStop();
//  };
//
//  auto async_handshaked_c = [this, &ssl_fiber_client, &buffer_c, received](
//      const boost::system::error_code& ec) {
//    boost::asio::async_read(ssl_fiber_client,
//                            boost::asio::buffer(&buffer_c, sizeof(buffer_c)),
//                            received);
//  };
//
//  auto async_connect_c = [this, &ssl_fiber_client, async_handshaked_c](
//      const boost::system::error_code& ec) {
//    ssl_fiber_client.async_handshake(boost::asio::ssl::stream_base::client,
//                                     async_handshaked_c);
//  };
//
//  startServer(handler_s);
//
//  connectClient();
//  fiberizeClient();
//  ssl_fiber_client.next_layer().async_connect(async_connect_c);
//  io_service_client.run();
//  serverThread_.join();
//}