#include "services/socks/v4/reply.h"

#include <iostream>  // NOLINT

namespace ssf {
namespace socks {
namespace v4 {

Reply::Reply(const boost::system::error_code& err,
             const boost::asio::ip::tcp::endpoint& ep)
    : null_byte_(0), status_(ErrorCodeToStatus(err)) {
  uint16_t port = ep.port();
  port_high_byte_ = (port >> 8) & 0xff;
  port_low_byte_ = port & 0xff;
  address_ = ep.address().to_v4().to_bytes();
}

std::array<boost::asio::const_buffer, 5> Reply::Buffer() const {
  std::array<boost::asio::const_buffer, 5> buf = {
      {boost::asio::buffer(&null_byte_, 1), boost::asio::buffer(&status_, 1),
       boost::asio::buffer(&port_high_byte_, 1),
       boost::asio::buffer(&port_low_byte_, 1), boost::asio::buffer(address_)}};
  return buf;
}

Reply::Status Reply::ErrorCodeToStatus(const boost::system::error_code& err) {
  Status s = kFailed;

  if (!err) {
    s = kGranted;
  }

  return s;
}
}  // v4
}  // socks
}  // ssf
