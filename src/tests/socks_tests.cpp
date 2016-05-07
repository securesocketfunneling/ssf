#include <vector>
#include <functional>
#include <array>
#include <future>
#include <list>

#include <gtest/gtest.h>
#include <boost/asio.hpp>

#include <ssf/log/log.h>

#include "services/initialisation.h"
#include "services/user_services/socks.h"

#include "tests/service_test_fixture.h"

#define SSF_ADDR "127.0.0.1"
#define SSF_PORT "9000"
#define SSF_SOCKS_ADDR "127.0.0.1"
#define SSF_SOCKS_PORT "9091"
#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT "9090"

class SocksTest : public ServiceFixtureTest<ssf::services::Socks> {
 public:
  SocksTest() {}

  ~SocksTest() {}

  virtual void SetUp() {
    StartServer(SSF_ADDR, SSF_PORT);
    StartClient(SSF_ADDR, SSF_PORT);
  }

  virtual std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) {
    return ServiceTested::CreateServiceOptions(SSF_SOCKS_PORT, ec);
  }
};

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
  DummyClient(size_t size)
      : io_service_(),
        p_worker_(new boost::asio::io_service::work(io_service_)),
        socket_(io_service_),
        size_(size) {}

  bool Init() {
    t_ = boost::thread([&]() { io_service_.run(); });

    boost::asio::ip::tcp::resolver r(io_service_);
    boost::asio::ip::tcp::resolver::query q(SSF_SOCKS_ADDR, SSF_SOCKS_PORT);
    boost::system::error_code ec;
    boost::asio::connect(socket_, r.resolve(q), ec);

    if (ec) {
      SSF_LOG(kLogError) << "dummy client: fail to connect " << ec.value();
      Stop();
    }

    return !ec;
  }

  bool InitSocks() {
    boost::system::error_code ec;

    boost::asio::ip::tcp::resolver r2(io_service_);
    boost::asio::ip::tcp::resolver::query q2(SERVER_ADDR, SERVER_PORT);
    request req(request::command_type::connect, *r2.resolve(q2), "01");

    boost::asio::write(socket_, req.buffers(), ec);

    if (ec) {
      SSF_LOG(kLogError) << "dummy client: fail to write " << ec.value();
      Stop();
      return false;
    }

    reply rep;

    boost::asio::read(socket_, rep.buffers(), ec);

    if (ec) {
      SSF_LOG(kLogError) << "dummy client: fail to read " << ec.value();
      Stop();
      return false;
    }

    if (!rep.success()) {
      Stop();
      return false;
    }

    boost::asio::write(socket_, boost::asio::buffer(&size_, sizeof(size_t)),
                       ec);

    if (ec) {
      SSF_LOG(kLogError) << "dummy client: fail to write " << ec.value();
      Stop();
    }

    return !ec;
  }

  bool ReceiveOneBuffer() {
    size_t received(0);

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
  bool CheckOneBuffer(size_t n) {
    for (size_t i = 0; i < n; ++i) {
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
    for (size_t i = 0; i < one_buffer_size_; ++i) {
      one_buffer_[i] = 1;
    }
  }

  void Run() {
    for (uint8_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
      threads_.create_thread([&]() { io_service_.run(); });
    }

    boost::asio::ip::tcp::resolver r(io_service_);
    boost::asio::ip::tcp::resolver::query q(SERVER_ADDR, SERVER_PORT);

    boost::asio::socket_base::reuse_address option(true);
    try {
      boost::asio::ip::tcp::endpoint endpoint(*r.resolve(q));
      acceptor_.open(endpoint.protocol());

      acceptor_.set_option(option);
      acceptor_.bind(endpoint);
      acceptor_.listen();
    } catch (const std::exception& e) {
      SSF_LOG(kLogError) << "dummy server: fail to initialize acceptor ("
                         << e.what() << ")";
      Stop();
      return;
    }

    DoAccept();
  }

  void Stop() {
    boost::system::error_code ec;
    sockets_.clear();
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
  size_t one_buffer_size_;
  std::array<uint8_t, 10240> one_buffer_;
  std::set<std::shared_ptr<boost::asio::ip::tcp::socket>> sockets_;
  boost::thread_group threads_;
};

//-----------------------------------------------------------------------------
TEST_F(SocksTest, startStopTransmitSSFSocks) {
  ASSERT_TRUE(Wait());

  std::list<std::promise<bool>> clients_finish;

  boost::recursive_mutex mutex;

  auto download = [&mutex](size_t size, std::promise<bool>& test_client) {
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
