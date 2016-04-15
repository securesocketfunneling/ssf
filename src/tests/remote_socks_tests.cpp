#include <vector>
#include <functional>
#include <array>
#include <future>
#include <list>

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
#include "services/user_services/remote_socks.h"

class request {
 public:
  enum command_type { connect = 0x01, bind = 0x02 };

  request(command_type cmd, const boost::asio::ip::tcp::endpoint& endpoint,
          const std::string& user_id)
      : version_(0x04), command_(cmd), user_id_(user_id), null_byte_(0) {
    // Only IPv4 is supported by the SOCKS 4 protocol.
    if (endpoint.protocol() != boost::asio::ip::tcp::v4()) {
      throw boost::system::system_error(
          boost::asio::error::address_family_not_supported);
    }

    // Convert port number to network byte order.
    unsigned short port = endpoint.port();
    port_high_byte_ = (port >> 8) & 0xff;
    port_low_byte_ = port & 0xff;

    // Save IP address in network byte order.
    address_ = endpoint.address().to_v4().to_bytes();
  }

  std::array<boost::asio::const_buffer, 7> buffers() const {
    std::array<boost::asio::const_buffer, 7> bufs = {
        {boost::asio::buffer(&version_, 1), boost::asio::buffer(&command_, 1),
         boost::asio::buffer(&port_high_byte_, 1),
         boost::asio::buffer(&port_low_byte_, 1), boost::asio::buffer(address_),
         boost::asio::buffer(user_id_), boost::asio::buffer(&null_byte_, 1)}};
    return bufs;
  }

 private:
  unsigned char version_;
  unsigned char command_;
  unsigned char port_high_byte_;
  unsigned char port_low_byte_;
  boost::asio::ip::address_v4::bytes_type address_;
  std::string user_id_;
  unsigned char null_byte_;
};

class reply {
 public:
  enum status_type {
    request_granted = 0x5a,
    request_failed = 0x5b,
    request_failed_no_identd = 0x5c,
    request_failed_bad_user_id = 0x5d
  };

  reply() : null_byte_(0), status_() {}

  std::array<boost::asio::mutable_buffer, 5> buffers() {
    std::array<boost::asio::mutable_buffer, 5> bufs = {
        {boost::asio::buffer(&null_byte_, 1), boost::asio::buffer(&status_, 1),
         boost::asio::buffer(&port_high_byte_, 1),
         boost::asio::buffer(&port_low_byte_, 1),
         boost::asio::buffer(address_)}};
    return bufs;
  }

  bool success() const { return null_byte_ == 0 && status_ == request_granted; }

  unsigned char status() const { return status_; }

  boost::asio::ip::tcp::endpoint endpoint() const {
    unsigned short port = port_high_byte_;
    port = (port << 8) & 0xff00;
    port = port | port_low_byte_;

    boost::asio::ip::address_v4 address(address_);

    return boost::asio::ip::tcp::endpoint(address, port);
  }

 private:
  unsigned char null_byte_;
  unsigned char status_;
  unsigned char port_high_byte_;
  unsigned char port_low_byte_;
  boost::asio::ip::address_v4::bytes_type address_;
};

class DummyClient {
 public:
   DummyClient(std::size_t size)
      : io_service_(),
        p_worker_(new boost::asio::io_service::work(io_service_)),
        socket_(io_service_),
        size_(size) {}

  bool Init() {
    t_ = boost::thread([&]() { io_service_.run(); });

    boost::asio::ip::tcp::resolver r(io_service_);
    boost::asio::ip::tcp::resolver::query q("127.0.0.1", "8081");
    boost::system::error_code ec;
    boost::asio::connect(socket_, r.resolve(q), ec);

    if (ec) {
      BOOST_LOG_TRIVIAL(error) << "dummy client : fail to connect "
                               << ec.value();
      Stop();
    }

    return !ec;
  }

  bool InitSocks() {
    boost::system::error_code ec;

    boost::asio::ip::tcp::resolver r2(io_service_);
    boost::asio::ip::tcp::resolver::query q2("127.0.0.1", "8080");
    request req(request::command_type::connect, *r2.resolve(q2), "01");

    boost::asio::write(socket_, req.buffers(), ec);

    if (ec) {
      BOOST_LOG_TRIVIAL(error) << "dummy client : fail to write " << ec.value();
      Stop();
      return false;
    }

    reply rep;

    boost::asio::read(socket_, rep.buffers(), ec);

    if (ec) {
      BOOST_LOG_TRIVIAL(error) << "dummy client : fail to read " << ec.value();
      Stop();
      return false;
    }

    if (!rep.success()) {
      Stop();
      return false;
    }

    boost::asio::write(socket_, boost::asio::buffer(&size_, sizeof(size_)),
                       ec);

    if (ec) {
      BOOST_LOG_TRIVIAL(error) << "dummy client : fail to write " << ec.value();
      Stop();
    }

    return !ec;
  }

  bool ReceiveOneBuffer() {
    std::size_t received(0);

    while (received < size_) {
      boost::system::error_code ec;
      auto n = socket_.receive(boost::asio::buffer(one_buffer_), 0, ec);

      if (ec) {
        Stop();
        return false;
      }

      if (n == 0) {
        Stop();
        return false;
      } else {
        received += n;
        if (!CheckOneBuffer(n)) {
          Stop();
          return false;
        }
      }
    }

    Stop();
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
   bool CheckOneBuffer(std::size_t n) {
     for (std::size_t i = 0; i < n; ++i) {
      if (one_buffer_[i] != 1) {
        return false;
      }
    }

    return true;
  }

 private:
  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  boost::asio::ip::tcp::socket socket_;
  boost::thread t_;
  std::size_t size_;
  std::array<uint8_t, 10240> one_buffer_;
};

class DummyServer {
 public:
  DummyServer()
      : io_service_(),
        p_worker_(new boost::asio::io_service::work(io_service_)),
        acceptor_(io_service_),
        one_buffer_size_(10240) {
    for (std::size_t i = 0; i < one_buffer_size_; ++i) {
      one_buffer_[i] = 1;
    }
  }

  void Run() {
    for (uint8_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
      threads_.create_thread([&]() { io_service_.run(); });
    }

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), 8080);
    boost::asio::socket_base::reuse_address option(true);

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(option);
    acceptor_.bind(endpoint);
    acceptor_.listen();

    DoAccept();
  }

  void Stop() {
    sockets_.clear();
    boost::system::error_code ec;
    acceptor_.close(ec);

    p_worker_.reset();

    threads_.join_all();
    io_service_.stop();
  }

 private:
  void DoAccept() {
    auto p_socket = std::make_shared<boost::asio::ip::tcp::socket>(io_service_);
    sockets_.insert(p_socket);
    acceptor_.async_accept(
        *p_socket, boost::bind(&DummyServer::HandleAccept, this, p_socket, _1));
  }

  void HandleAccept(std::shared_ptr<boost::asio::ip::tcp::socket> p_socket,
                    const boost::system::error_code& ec) {
    if (!ec) {
      auto p_size = std::make_shared<std::size_t>(0);
      boost::asio::async_read(
        *p_socket, boost::asio::buffer(p_size.get(), sizeof(*p_size)),
          boost::bind(&DummyServer::DoSendOnes, this, p_socket, p_size, _1,
                      _2));
      DoAccept();
    } else {
      p_socket->close();
    }
  }

  void DoSendOnes(std::shared_ptr<boost::asio::ip::tcp::socket> p_socket,
                  std::shared_ptr<std::size_t> p_size,
                  const boost::system::error_code& ec, size_t length) {
    if (!ec) {
      p_socket->async_send(boost::asio::buffer(one_buffer_, *p_size),
                           boost::bind(&DummyServer::HandleSend, this, p_socket,
                                       p_size, _1, _2));
    }

    return;
  }

  void HandleSend(std::shared_ptr<boost::asio::ip::tcp::socket> p_socket,
                  std::shared_ptr<std::size_t> p_size,
                  const boost::system::error_code& ec, size_t length) {
    if (!ec) {
      *p_size -= length;
      if (*p_size == 0) {
        return;
      } else {
        DoSendOnes(p_socket, p_size, boost::system::error_code(), 0);
        return;
      }
    } else {
      return;
    }
  }

  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  boost::asio::ip::tcp::acceptor acceptor_;
  std::size_t one_buffer_size_;
  std::array<uint8_t, 10240> one_buffer_;
  std::set<std::shared_ptr<boost::asio::ip::tcp::socket>> sockets_;
  boost::thread_group threads_;
};

class RemoteSocksTest : public ::testing::Test
{
 public:
  using Client =
      ssf::SSFClient<ssf::network::Protocol, ssf::TransportProtocolPolicy>;
  using Server =
      ssf::SSFServer<ssf::network::Protocol, ssf::TransportProtocolPolicy>;
  using demux = Client::demux;
  using BaseUserServicePtr =
      ssf::services::BaseUserService<demux>::BaseUserServicePtr;

 public:
  RemoteSocksTest()
      : client_io_service_(),
        p_client_worker_(new boost::asio::io_service::work(client_io_service_)),
        server_io_service_(),
        p_server_worker_(new boost::asio::io_service::work(server_io_service_)),
        p_ssf_client_(nullptr),
        p_ssf_server_(nullptr) {}

  ~RemoteSocksTest() {}

  virtual void SetUp() {
    StartServer();
    StartClient();
  }

  virtual void TearDown() {
    StopServerThreads();
    StopClientThreads();
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
    auto p_service = ssf::services::RemoteSocks<demux>::CreateServiceOptions(
        "8081", ec);

    client_options.push_back(p_service);

    ssf::Config ssf_config;

    auto endpoint_query =
        ssf::network::GenerateClientQuery("127.0.0.1", "8000", ssf_config, {});

    p_ssf_client_.reset(new Client(
        client_io_service_, client_options,
        boost::bind(&RemoteSocksTest::SSFClientCallback, this, _1, _2, _3)));
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
        p_user_service->GetName() == "remote_socks") {
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
TEST_F(RemoteSocksTest, startStopTransmitSSFRemoteSocks) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);

  ASSERT_TRUE(Wait());

  std::list<std::promise<bool>> clients_finish;

  boost::recursive_mutex mutex;

  auto download = [&mutex](std::size_t size, std::promise<bool>& test_client) {
    DummyClient client(size);
    auto initiated = client.Init();

    {
      boost::recursive_mutex::scoped_lock lock(mutex);
      EXPECT_TRUE(initiated);
    }

    auto sent = client.InitSocks();

    {
      boost::recursive_mutex::scoped_lock lock(mutex);
      EXPECT_TRUE(sent);
    }

    auto received = client.ReceiveOneBuffer();

    {
      boost::recursive_mutex::scoped_lock lock(mutex);
      EXPECT_TRUE(received);
    }

    client.Stop();
    test_client.set_value(true);
  };

  DummyServer serv;
  serv.Run();

  boost::thread_group client_test_threads;

  for (std::size_t i = 0; i < 6; ++i) {
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
