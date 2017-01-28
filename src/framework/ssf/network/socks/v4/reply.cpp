#include <ssf/utils/enum.h>

#include "ssf/network/socks/v4/reply.h"

#include "ssf/network/socks/socks.h"

namespace ssf {
namespace network {
namespace socks {
namespace v4 {

Reply::Reply()
    : null_byte_(0),
      status_(ToIntegral(Status::kFailed)),
      port_high_byte_(0),
      port_low_byte_(0),
      address_() {}

Reply::Reply(const boost::system::error_code& err,
             const boost::asio::ip::tcp::endpoint& ep)
    : null_byte_(0), status_(ToIntegral(Status::kFailed)) {
  uint16_t port = ep.port();
  port_high_byte_ = (port >> 8) & 0xff;
  port_low_byte_ = port & 0xff;
  address_ = ep.address().to_v4().to_bytes();
  if (!err) {
    status_ = ToIntegral(Status::kGranted);
  }
}

std::array<boost::asio::const_buffer, 5> Reply::ConstBuffer() const {
  return {{boost::asio::buffer(&null_byte_, 1), boost::asio::buffer(&status_, 1),
           boost::asio::buffer(&port_high_byte_, 1),
           boost::asio::buffer(&port_low_byte_, 1),
           boost::asio::buffer(address_)}};
}

std::array<boost::asio::mutable_buffer, 5> Reply::MutBuffer() {
  return {{boost::asio::buffer(&null_byte_, 1), boost::asio::buffer(&status_, 1),
           boost::asio::buffer(&port_high_byte_, 1),
           boost::asio::buffer(&port_low_byte_, 1),
           boost::asio::buffer(address_)}};
}

}  // v4
}  // socks
}  // network
}  // ssf
