#include <array>
#include <functional>
#include <future>
#include <memory>
#include <vector>

#include <gtest/gtest.h>
#include <boost/asio.hpp>

#include <ssf/log/log.h>

#include "services/initialisation.h"
#include "services/user_services/remote_port_forwarding.h"

#include "tests/service_test_fixture.h"

class RemoteStreamForwardTest
    : public ServiceFixtureTest<ssf::services::RemotePortForwading> {
 public:
  RemoteStreamForwardTest() {}

  ~RemoteStreamForwardTest() {}

  virtual void SetUp() {
    StartServer("127.0.0.1", "8000");
    StartClient("127.0.0.1", "8000");
  }

  std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) {
    return ServiceTested::CreateServiceOptions("5454:127.0.0.1:5354", ec);
  }
};

class DummyClient {
 public:
  DummyClient(size_t size)
      : io_service_(),
        p_worker_(new boost::asio::io_service::work(io_service_)),
        socket_(io_service_),
        size_(size) {}

  bool Run() {
    t_ = boost::thread([&]() { io_service_.run(); });

    boost::asio::ip::tcp::resolver r(io_service_);
    boost::asio::ip::tcp::resolver::query q("127.0.0.1", "5454");
    boost::system::error_code ec;
    boost::asio::connect(socket_, r.resolve(q), ec);

    if (ec) {
      SSF_LOG(kLogError) << "dummy client: fail to connect " << ec.value();
      return false;
    }

    boost::asio::write(socket_, boost::asio::buffer(&size_, sizeof(size_t)),
                       ec);

    if (ec) {
      SSF_LOG(kLogError) << "dummy client: fail to write " << ec.value();
      return false;
    }

    size_t received(0);
    size_t n(0);
    while (received < size_) {
      boost::system::error_code ec_read;
      n = socket_.read_some(boost::asio::buffer(one_buffer_), ec_read);

      if (n == 0) {
        return false;
      }

      if (ec_read) {
        return false;
      }

      if (!CheckOneBuffer(n)) {
        return false;
      }
      received += n;
    }

    return received == size_;
  }

  void Stop() {
    boost::system::error_code ec;
    socket_.close(ec);

    p_worker_.reset();

    t_.join();
    io_service_.stop();
  }

 private:
  bool CheckOneBuffer(size_t n) {
    for (size_t i = 0; i < n; ++i) {
      if (one_buffer_[i] != 1) {
        return false;
      }
    }

    return true;
  }

  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  boost::asio::ip::tcp::socket socket_;
  boost::thread t_;
  size_t size_;
  std::array<uint8_t, 10240> one_buffer_;
};

class DummyServer {
 public:
  DummyServer()
      : io_service_(),
        p_worker_(new boost::asio::io_service::work(io_service_)),
        acceptor_(io_service_),
        one_buffer_size_(10240),
        one_buffer_(one_buffer_size_) {
    for (size_t i = 0; i < one_buffer_size_; ++i) {
      one_buffer_[i] = 1;
    }
  }

  void Run() {
    for (uint8_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
      threads_.create_thread([&]() { io_service_.run(); });
    }

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), 5354);
    boost::asio::socket_base::reuse_address option(true);

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(option);
    acceptor_.bind(endpoint);
    acceptor_.listen();

    do_accept();
  }

  void Stop() {
    boost::system::error_code ec;
    acceptor_.close(ec);

    p_worker_.reset();

    threads_.join_all();
    io_service_.stop();
  }

 private:
  void do_accept() {
    auto p_socket = std::make_shared<boost::asio::ip::tcp::socket>(io_service_);
    acceptor_.async_accept(*p_socket, boost::bind(&DummyServer::handle_accept,
                                                  this, p_socket, _1));
  }

  void handle_accept(std::shared_ptr<boost::asio::ip::tcp::socket> p_socket,
                     const boost::system::error_code& ec) {
    if (!ec) {
      auto p_size = std::make_shared<size_t>(0);
      boost::asio::async_read(*p_socket,
                              boost::asio::buffer(p_size.get(), sizeof(size_t)),
                              boost::bind(&DummyServer::handle_send, this,
                                          p_socket, p_size, true, _1, _2));
      do_accept();
    } else {
      p_socket->close();
    }
  }

  void handle_send(std::shared_ptr<boost::asio::ip::tcp::socket> p_socket,
                   std::shared_ptr<size_t> p_size, bool first,
                   const boost::system::error_code& ec,
                   size_t tranferred_bytes) {
    if (!ec) {
      if (!first) {
        (*p_size) -= tranferred_bytes;
      }

      if ((*p_size)) {
        if ((*p_size) > one_buffer_size_) {
          boost::asio::async_write(
              *p_socket, boost::asio::buffer(one_buffer_, one_buffer_size_),
              boost::bind(&DummyServer::handle_send, this, p_socket, p_size,
                          false, _1, _2));
        } else {
          boost::asio::async_write(
              *p_socket, boost::asio::buffer(one_buffer_, (*p_size)),
              boost::bind(&DummyServer::handle_send, this, p_socket, p_size,
                          false, _1, _2));
        }
      } else {
        boost::asio::async_read(*p_socket, boost::asio::buffer(one_buffer_, 1),
                                [=](const boost::system::error_code&, size_t) {
                                  p_socket->close();
                                });
      }
    } else {
      p_socket->close();
    }
  }

  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  boost::asio::ip::tcp::acceptor acceptor_;
  size_t one_buffer_size_;
  std::vector<uint8_t> one_buffer_;
  boost::thread_group threads_;
};

TEST_F(RemoteStreamForwardTest, transferOnesOverStream) {
  ASSERT_TRUE(Wait());

  std::list<std::promise<bool>> clients_finish;

  boost::recursive_mutex mutex;

  auto download = [&mutex](size_t size, std::promise<bool>& test_client) {
    DummyClient client(size);

    {
      boost::recursive_mutex::scoped_lock lock(mutex);
      EXPECT_TRUE(client.Run());
    }

    client.Stop();
    test_client.set_value(true);
  };

  DummyServer serv;
  serv.Run();

  boost::thread_group client_test_threads;

  for (int i = 0; i < 6; ++i) {
    clients_finish.emplace_front();
    std::promise<bool>& client_finish = clients_finish.front();
    client_test_threads.create_thread(boost::bind<void>(
        download, 1024 * 1024 * i, boost::ref(client_finish)));
  }

  client_test_threads.join_all();
  for (auto& client_finish : clients_finish) {
    client_finish.get_future().wait();
  }

  serv.Stop();
}
