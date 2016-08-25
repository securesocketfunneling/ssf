#include "services/socks/v4/request.h"

#include <string>    // NOLINT
#include <iostream>  // NOLINT

#include <boost/asio.hpp>            // NOLINT
#include <boost/asio/coroutine.hpp>  // NOLINT

namespace ssf {
namespace socks {
namespace v4 {

uint8_t Request::Command() const { return command_; }

boost::asio::ip::tcp::endpoint Request::Endpoint() const {
  uint16_t port = port_high_byte_;
  port = (port << 8) & 0xff00;
  port = port | port_low_byte_;

  if (is_4a_version()) {
    return boost::asio::ip::tcp::endpoint();
  } else {
    boost::asio::ip::address_v4 address(address_);
    return boost::asio::ip::tcp::endpoint(address, port);
  }
}

std::array<boost::asio::mutable_buffer, 4> Request::Buffer() {
  std::array<boost::asio::mutable_buffer, 4> buf = {
      {boost::asio::buffer(&command_, 1),
       boost::asio::buffer(&port_high_byte_, 1),
       boost::asio::buffer(&port_low_byte_, 1), boost::asio::buffer(address_)}};
  return buf;
}

std::string Request::Name() const { return name_; }

std::string Request::Domain() const { return domain_; }

uint16_t Request::Port() const {
  uint16_t port = port_high_byte_;
  port = (port << 8) & 0xff00;
  port = port | port_low_byte_;

  return port;
}

void Request::SetName(const std::string& name) { name_ = name; }

void Request::SetDomain(const std::string& domain) { domain_ = domain; }

bool Request::is_4a_version() const {
  return (address_[0] == 0) && (address_[1] == 0) && (address_[2] == 0) &&
         (address_[3] != 0);
}

}  // v4
}  // socks
}  // ssf
