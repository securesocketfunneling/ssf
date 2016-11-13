#include "ssf/network/socks/v4/request.h"

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/address.hpp>

#include "ssf/error/error.h"

#include "ssf/network/socks/socks.h"

namespace ssf {
namespace network {
namespace socks {
namespace v4 {

void Request::Init(Command command, const std::string& addr, uint16_t port,
                   boost::system::error_code& ec) {
  version_ = static_cast<uint8_t>(Socks::Version::kV4);
  command_ = static_cast<uint8_t>(command);
  port_high_byte_ = (port >> 8);
  port_low_byte_ = (port & 0x00ff);
  null_byte_ = 0;

  boost::system::error_code addr_ec;
  auto ip_addr = boost::asio::ip::address::from_string(addr, addr_ec);
  if (addr_ec) {
    // addr is a domain name, set invalid ip in ip field
    address_[0] = 0;
    address_[1] = 0;
    address_[2] = 0;
    address_[3] = 0xff;
    domain_ = addr;
    return;
  }

  if (!ip_addr.is_v4()) {
    ec.assign(ssf::error::bad_address, ssf::error::get_ssf_category());
    return;
  }

  address_ = ip_addr.to_v4().to_bytes();
}

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

std::array<boost::asio::mutable_buffer, 4> Request::MutBuffer() {
  std::array<boost::asio::mutable_buffer, 4> buf = {
      {boost::asio::buffer(&command_, 1),
       boost::asio::buffer(&port_high_byte_, 1),
       boost::asio::buffer(&port_low_byte_, 1), boost::asio::buffer(address_)}};
  return buf;
}

std::vector<boost::asio::const_buffer> Request::ConstBuffer() const {
  std::vector<boost::asio::const_buffer> buf = {
      {boost::asio::buffer(&version_, 1), boost::asio::buffer(&command_, 1),
       boost::asio::buffer(&port_high_byte_, 1),
       boost::asio::buffer(&port_low_byte_, 1), boost::asio::buffer(address_),
       boost::asio::buffer(name_), boost::asio::buffer(&null_byte_, 1)}};
  if (is_4a_version()) {
    buf.push_back(boost::asio::buffer(domain_));
    buf.push_back(boost::asio::buffer(&null_byte_, 1));
  }
  return buf;
}

}  // v4
}  // socks
}  // network
}  // ssf
