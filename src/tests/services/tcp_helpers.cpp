#include <functional>

#include <boost/asio/connect.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include "common/error/error.h"

#include "tests/services/tcp_helpers.h"

namespace tests {
namespace tcp {

DummyServer::DummyServer(const std::string& listening_addr,
                         const std::string& listening_port)
    : io_service_(),
      p_worker_(new boost::asio::io_service::work(io_service_)),
      acceptor_(io_service_),
      listening_addr_(listening_addr),
      listening_port_(listening_port),
      one_buffer_size_(10240) {
  for (size_t i = 0; i < one_buffer_size_; ++i) {
    one_buffer_[i] = 1;
  }
}

void DummyServer::Run() {
  for (uint8_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    threads_.emplace_back([&]() { io_service_.run(); });
  }

  boost::asio::ip::tcp::resolver r(io_service_);
  boost::asio::ip::tcp::resolver::query q(listening_addr_, listening_port_);

  boost::asio::socket_base::reuse_address option(true);
  try {
    boost::asio::ip::tcp::endpoint endpoint(*r.resolve(q));
    acceptor_.open(endpoint.protocol());

    acceptor_.set_option(option);
    acceptor_.bind(endpoint);
    acceptor_.listen();
  } catch (const std::exception& e) {
    SSF_LOG(kLogError) << "dummy tcp server: fail to initialize acceptor ("
                       << e.what() << ")";
    Stop();
    return;
  }

  DoAccept();
}

void DummyServer::Stop() {
  boost::system::error_code ec;
  sockets_.clear();
  acceptor_.close(ec);

  p_worker_.reset();

  for (auto& thread : threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
  io_service_.stop();
}

void DummyServer::DoAccept() {
  auto p_socket = std::make_shared<boost::asio::ip::tcp::socket>(io_service_);
  sockets_.insert(p_socket);
  acceptor_.async_accept(*p_socket, std::bind(&DummyServer::HandleAccept, this,
                                              p_socket, std::placeholders::_1));
}

void DummyServer::HandleAccept(
    std::shared_ptr<boost::asio::ip::tcp::socket> p_socket,
    const boost::system::error_code& ec) {
  if (!ec) {
    auto p_size = std::make_shared<std::size_t>(0);
    boost::asio::async_read(
        *p_socket, boost::asio::buffer(p_size.get(), sizeof(*p_size)),
        std::bind(&DummyServer::DoSendOnes, this, p_socket, p_size,
                  std::placeholders::_1, std::placeholders::_2));
    DoAccept();
  } else {
    p_socket->close();
  }
}

void DummyServer::DoSendOnes(
    std::shared_ptr<boost::asio::ip::tcp::socket> p_socket,
    std::shared_ptr<std::size_t> p_size, const boost::system::error_code& ec,
    size_t length) {
  if (!ec) {
    p_socket->async_send(
        boost::asio::buffer(one_buffer_, *p_size),
        std::bind(&DummyServer::HandleSend, this, p_socket, p_size,
                  std::placeholders::_1, std::placeholders::_2));
  }

  return;
}

void DummyServer::HandleSend(
    std::shared_ptr<boost::asio::ip::tcp::socket> p_socket,
    std::shared_ptr<std::size_t> p_size, const boost::system::error_code& ec,
    size_t length) {
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

DummyClient::DummyClient(const std::string& target_addr,
                         const std::string& target_port, size_t size)
    : io_service_(),
      p_worker_(new boost::asio::io_service::work(io_service_)),
      socket_(io_service_),
      target_addr_(target_addr),
      target_port_(target_port),
      size_(size) {}

bool DummyClient::Run() {
  t_ = std::thread([&]() { io_service_.run(); });

  boost::asio::ip::tcp::resolver r(io_service_);
  boost::asio::ip::tcp::resolver::query q(target_addr_, target_port_);
  boost::system::error_code ec;
  boost::asio::connect(socket_, r.resolve(q), ec);

  if (ec) {
    SSF_LOG(kLogError) << "dummy client: fail to connect " << ec.value();
    return false;
  }

  boost::asio::write(socket_, boost::asio::buffer(&size_, sizeof(size_t)), ec);

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

void DummyClient::Stop() {
  boost::system::error_code ec;
  socket_.close(ec);

  p_worker_.reset();

  if (t_.joinable()) {
    t_.join();
  }
  io_service_.stop();
}

bool DummyClient::CheckOneBuffer(size_t n) {
  for (size_t i = 0; i < n; ++i) {
    if (one_buffer_[i] != 1) {
      return false;
    }
  }

  return true;
}

}  // tcp
}  // tests
