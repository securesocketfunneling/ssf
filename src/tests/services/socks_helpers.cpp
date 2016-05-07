#include "common/error/error.h"

#include "tests/services/socks_helpers.h"

namespace tests {
namespace socks {

Request::Request(CommandType cmd,
                 const boost::asio::ip::tcp::endpoint& endpoint,
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

std::array<boost::asio::const_buffer, 7> Request::Buffers() const {
  std::array<boost::asio::const_buffer, 7> bufs = {
      {boost::asio::buffer(&version_, 1), boost::asio::buffer(&command_, 1),
       boost::asio::buffer(&port_high_byte_, 1),
       boost::asio::buffer(&port_low_byte_, 1), boost::asio::buffer(address_),
       boost::asio::buffer(user_id_), boost::asio::buffer(&null_byte_, 1)}};
  return bufs;
}

Reply::Reply() : null_byte_(0), status_() {}

std::array<boost::asio::mutable_buffer, 5> Reply::Buffers() {
  std::array<boost::asio::mutable_buffer, 5> bufs = {
      {boost::asio::buffer(&null_byte_, 1), boost::asio::buffer(&status_, 1),
       boost::asio::buffer(&port_high_byte_, 1),
       boost::asio::buffer(&port_low_byte_, 1), boost::asio::buffer(address_)}};
  return bufs;
}

bool Reply::Success() const {
  return null_byte_ == 0 && status_ == kRequestGranted;
}

unsigned char Reply::Status() const { return status_; }

boost::asio::ip::tcp::endpoint Reply::Endpoint() const {
  unsigned short port = port_high_byte_;
  port = (port << 8) & 0xff00;
  port = port | port_low_byte_;

  boost::asio::ip::address_v4 address(address_);

  return boost::asio::ip::tcp::endpoint(address, port);
}

DummyClient::DummyClient(const std::string& socks_server_addr,
                         const std::string& socks_server_port,
                         const std::string& target_addr,
                         const std::string& target_port, size_t size)
    : io_service_(),
      socks_server_addr_(socks_server_addr),
      socks_server_port_(socks_server_port),
      target_addr_(target_addr),
      target_port_(target_port),
      p_worker_(new boost::asio::io_service::work(io_service_)),
      socket_(io_service_),
      size_(size) {}

bool DummyClient::Init() {
  t_ = boost::thread([&]() { io_service_.run(); });

  boost::asio::ip::tcp::resolver r(io_service_);
  boost::asio::ip::tcp::resolver::query q(socks_server_addr_,
                                          socks_server_port_);
  boost::system::error_code ec;
  boost::asio::connect(socket_, r.resolve(q), ec);

  if (ec) {
    SSF_LOG(kLogError) << "dummy client: fail to connect " << ec.value();
    Stop();
  }

  return !ec;
}

bool DummyClient::InitSocks() {
  boost::system::error_code ec;

  boost::asio::ip::tcp::resolver r2(io_service_);
  boost::asio::ip::tcp::resolver::query q2(target_addr_, target_port_);
  Request req(Request::CommandType::kConnect, *r2.resolve(q2), "01");

  boost::asio::write(socket_, req.Buffers(), ec);

  if (ec) {
    SSF_LOG(kLogError) << "dummy client: fail to write " << ec.value();
    Stop();
    return false;
  }

  Reply rep;

  boost::asio::read(socket_, rep.Buffers(), ec);

  if (ec) {
    SSF_LOG(kLogError) << "dummy client: fail to read " << ec.value();
    Stop();
    return false;
  }

  if (!rep.Success()) {
    Stop();
    return false;
  }

  boost::asio::write(socket_, boost::asio::buffer(&size_, sizeof(size_t)), ec);

  if (ec) {
    SSF_LOG(kLogError) << "dummy client: fail to write " << ec.value();
    Stop();
  }

  return !ec;
}

bool DummyClient::ReceiveOneBuffer() {
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

void DummyClient::Stop() {
  boost::system::error_code ec;
  socket_.close(ec);

  p_worker_.reset();

  t_.join();
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

}  // socks
}  // tests