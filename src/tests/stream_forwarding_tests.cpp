#include <array>
#include <functional>
#include <future>
#include <memory>
#include <vector>

#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "common/config/config.h"

#include "core/network_protocol.h"
#include "core/client/client.h"
#include "core/server/server.h"

#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/initialisation.h"
#include "services/user_services/port_forwarding.h"

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
      BOOST_LOG_TRIVIAL(error) << "dummy client : fail to connect "
                               << ec.value();
      return false;
    }

    boost::asio::write(socket_, boost::asio::buffer(&size_, sizeof(size_t)),
                       ec);

    if (ec) {
      BOOST_LOG_TRIVIAL(error) << "dummy client : fail to write " << ec.value();
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
                                [=](const boost::system::error_code&,
                                    size_t) { p_socket->close(); });
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

class StreamForwardTest : public ::testing::Test {
 public:
  using Client =
      ssf::SSFClient<ssf::network::Protocol, ssf::TransportProtocolPolicy>;
  using Server =
      ssf::SSFServer<ssf::network::Protocol, ssf::TransportProtocolPolicy>;
  using demux = Client::demux;
  using BaseUserServicePtr =
      ssf::services::BaseUserService<demux>::BaseUserServicePtr;

 public:
  StreamForwardTest()
      : client_io_service_(),
        p_client_worker_(new boost::asio::io_service::work(client_io_service_)),
        server_io_service_(),
        p_server_worker_(new boost::asio::io_service::work(server_io_service_)),
        p_ssf_client_(nullptr),
        p_ssf_server_(nullptr) {}

  ~StreamForwardTest() {}

  virtual void SetUp() {
    StartServer();
    StartClient();
  }

  virtual void TearDown() {
    StopClientThreads();
    StopServerThreads();
  }

  void StartServer() {
    ssf::Config ssf_config;

    uint16_t port = 8000;
    auto endpoint_query =
        ssf::network::GenerateServerQuery(std::to_string(port), ssf_config);

    p_ssf_server_.reset(new Server(server_io_service_, ssf_config, 8000));

    StartServerThreads();
    boost::system::error_code run_ec;
    p_ssf_server_->Run(endpoint_query, run_ec);
  }

  void StartClient() {
    std::vector<BaseUserServicePtr> client_options;
    boost::system::error_code ec;
    auto p_service = ssf::services::PortForwading<demux>::CreateServiceOptions(
      "5454:127.0.0.1:5354", ec);

    client_options.push_back(p_service);

    ssf::Config ssf_config;

    auto endpoint_query =
        ssf::network::GenerateClientQuery("127.0.0.1", "8000", ssf_config, {});

    p_ssf_client_.reset(new Client(
        client_io_service_, client_options,
        boost::bind(&StreamForwardTest::SSFClientCallback, this, _1, _2, _3)));

    StartClientThreads();
    boost::system::error_code run_ec;
    p_ssf_client_->Run(endpoint_query, run_ec);
  }

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

  void SSFClientCallback(ssf::services::initialisation::type type,
                         BaseUserServicePtr p_user_service,
                         const boost::system::error_code& ec) {
    if (type == ssf::services::initialisation::NETWORK) {
      network_set_.set_value(!ec);
      if (ec) {
        service_set_.set_value(false);
        transport_set_.set_value(false);
      }

      return;
    }

    if (type == ssf::services::initialisation::TRANSPORT) {
      transport_set_.set_value(!ec);

      return;
    }

    if (type == ssf::services::initialisation::SERVICE &&
        p_user_service->GetName() == "forward") {
      service_set_.set_value(!ec);

      return;
    }
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

  std::promise<bool> network_set_;
  std::promise<bool> transport_set_;
  std::promise<bool> service_set_;
};

//-----------------------------------------------------------------------------
TEST_F(StreamForwardTest, transferOnesOverStream) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);

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