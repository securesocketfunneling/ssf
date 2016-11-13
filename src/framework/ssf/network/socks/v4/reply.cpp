#include "ssf/network/socks/v4/reply.h"

#include "ssf/network/socks/socks.h"

namespace ssf {
namespace network {
namespace socks {
namespace v4 {

Reply::Reply()
    : null_byte_(0),
      status_(static_cast<uint8_t>(Status::kFailed)),
      port_high_byte_(0),
      port_low_byte_(0),
      address_() {}

Reply::Reply(const boost::system::error_code& err,
             const boost::asio::ip::tcp::endpoint& ep)
    : null_byte_(0), status_(static_cast<uint8_t>(ErrorCodeToStatus(err))) {
  uint16_t port = ep.port();
  port_high_byte_ = (port >> 8) & 0xff;
  port_low_byte_ = port & 0xff;
  address_ = ep.address().to_v4().to_bytes();
}

std::array<boost::asio::const_buffer, 5> Reply::ConstBuffer() const {
  std::array<boost::asio::const_buffer, 5> buf = {
      {boost::asio::buffer(&null_byte_, 1), boost::asio::buffer(&status_, 1),
       boost::asio::buffer(&port_high_byte_, 1),
       boost::asio::buffer(&port_low_byte_, 1), boost::asio::buffer(address_)}};
  return buf;
}

std::array<boost::asio::mutable_buffer, 5> Reply::MutBuffer() {
  std::array<boost::asio::mutable_buffer, 5> buf = {
      {boost::asio::buffer(&null_byte_, 1), boost::asio::buffer(&status_, 1),
       boost::asio::buffer(&port_high_byte_, 1),
       boost::asio::buffer(&port_low_byte_, 1), boost::asio::buffer(address_)}};
  return buf;
}

Reply::Status Reply::ErrorCodeToStatus(const boost::system::error_code& err) {
  Status s = Status::kFailed;

  if (!err) {
    s = Status::kGranted;
  }

  return s;
}

}  // v4
}  // socks
}  // network
}  // ssf
