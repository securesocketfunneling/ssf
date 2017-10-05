#include "common/error/error.h"

#include "tests/services/udp_helpers.h"

namespace tests {
namespace udp {

DummyServer::DummyServer(const std::string& listening_addr,
                         const std::string& listening_port)
    : io_service_(),
      p_worker_(new boost::asio::io_service::work(io_service_)),
      listening_addr_(listening_addr),
      listening_port_(listening_port),
      socket_(io_service_),
      one_buffer_(10240) {
  for (size_t i = 0; i < 10240; ++i) {
    one_buffer_[i] = 1;
  }
}

void DummyServer::Run() {
  for (uint8_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    threads_.emplace_back([&]() { io_service_.run(); });
  }

  boost::asio::ip::udp::resolver r(io_service_);
  boost::asio::ip::udp::resolver::query q(listening_addr_, listening_port_);

  try {
    boost::asio::ip::udp::endpoint endpoint(*r.resolve(q));

    socket_.open(endpoint.protocol());
    socket_.bind(endpoint);
  } catch (const std::exception& e) {
    SSF_LOG(kLogError)
        << "dummy udp server: fail to initialize listening state (" << e.what()
        << ")";
    Stop();
    return;
  }

  DoReceive();
}

void DummyServer::Stop() {
  boost::system::error_code ec;
  socket_.close(ec);

  p_worker_.reset();

  for (auto& thread : threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  io_service_.stop();
}

void DummyServer::DoReceive() {
  auto p_new_size_ = std::make_shared<size_t>(0);
  auto p_send_endpoint_ = std::make_shared<boost::asio::ip::udp::endpoint>();

  socket_.async_receive_from(
      boost::asio::buffer(&(*p_new_size_), sizeof((*p_new_size_))),
      *p_send_endpoint_,
      std::bind(&DummyServer::SizeReceivedHandler, this, p_send_endpoint_,
                p_new_size_, std::placeholders::_1, std::placeholders::_2));
}

void DummyServer::SizeReceivedHandler(
    std::shared_ptr<boost::asio::ip::udp::endpoint> p_endpoint,
    std::shared_ptr<size_t> p_size, const boost::system::error_code& ec,
    size_t length) {
  if (!ec) {
    {
      std::unique_lock<std::recursive_mutex> lock(one_buffer_mutex_);
      socket_.async_send_to(
          boost::asio::buffer(one_buffer_, *p_size), *p_endpoint,
          std::bind(&DummyServer::OneBufferSentHandler, this, p_endpoint,
                    p_size, std::placeholders::_1, std::placeholders::_2));
    }
  }
}

void DummyServer::OneBufferSentHandler(
    std::shared_ptr<boost::asio::ip::udp::endpoint> p_endpoint,
    std::shared_ptr<size_t> p_size, const boost::system::error_code& ec,
    size_t length) {
  if (ec.value() == ::error::message_too_long) {
    {
      std::unique_lock<std::recursive_mutex> lock(one_buffer_mutex_);
      one_buffer_.resize(one_buffer_.size() / 2);
    }
    if (one_buffer_.size()) {
      SizeReceivedHandler(p_endpoint, p_size, boost::system::error_code(), 0);
      return;
    }
  } else {
    DoReceive();
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

bool DummyClient::Init() {
  t_ = std::thread([&]() { io_service_.run(); });

  boost::asio::ip::udp::resolver r(io_service_);
  boost::asio::ip::udp::resolver::query q(target_addr_, target_port_);

  auto it = r.resolve(q);

  endpoint_ = *it;

  boost::system::error_code ec;
  socket_.open(endpoint_.protocol(), ec);
  socket_.connect(endpoint_, ec);

  if (ec) {
    return false;
  } else {
    return true;
  }
}

bool DummyClient::ReceiveOneBuffer() {
  size_t received = 0;

  while (received < size_) {
    boost::system::error_code ec;
    size_t remaining_size = size_ - received;
    socket_.send(boost::asio::buffer(&remaining_size, sizeof(remaining_size)),
                 0, ec);

    if (ec) {
      return false;
    }

    auto n = socket_.receive(boost::asio::buffer(one_buffer_), 0, ec);

    if (ec) {
      return false;
    }

    if (n == 0) {
      return false;
    } else {
      received += n;
      if (!CheckOneBuffer(n)) {
        return false;
      }
      ResetBuffer();
    }
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

void DummyClient::ResetBuffer() {
  for (size_t i = 0; i < one_buffer_.size(); ++i) {
    one_buffer_[i] = 0;
  }
}

bool DummyClient::CheckOneBuffer(size_t n) {
  for (size_t i = 0; i < n; ++i) {
    if (one_buffer_[i] != 1) {
      return false;
    }
  }

  return true;
}

}  // udp
}  // tests