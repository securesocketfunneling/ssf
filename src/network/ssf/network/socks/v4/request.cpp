#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/address.hpp>

#include "ssf/utils/enum.h"
#include "ssf/error/error.h"
#include "ssf/network/socks/socks.h"
#include "ssf/network/socks/v4/request.h"

namespace ssf {
namespace network {
namespace socks {
namespace v4 {

void Request::Init(Command command, const std::string& addr, uint16_t port,
                   boost::system::error_code& ec) {
  version_ = ToIntegral(Socks::Version::kV4);
  command_ = ToIntegral(command);
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

uint16_t Request::port() const {
  uint16_t port = port_high_byte_;
  port = (port << 8) & 0xff00;
  port = port | port_low_byte_;

  return port;
}

bool Request::Is4aVersion() const {
  return (address_[0] == 0) && (address_[1] == 0) && (address_[2] == 0) &&
         (address_[3] != 0);
}

boost::asio::ip::tcp::endpoint Request::Endpoint() const {
  uint16_t port = port_high_byte_;
  port = (port << 8) & 0xff00;
  port = port | port_low_byte_;

  if (Is4aVersion()) {
    return boost::asio::ip::tcp::endpoint();
  } else {
    boost::asio::ip::address_v4 address(address_);
    return boost::asio::ip::tcp::endpoint(address, port);
  }
}

std::array<boost::asio::mutable_buffer, 4> Request::MutBuffer() {
  return {{boost::asio::buffer(&command_, 1),
           boost::asio::buffer(&port_high_byte_, 1),
           boost::asio::buffer(&port_low_byte_, 1),
           boost::asio::buffer(address_)}};
}

std::vector<boost::asio::const_buffer> Request::ConstBuffer() const {
  std::vector<boost::asio::const_buffer> buf = {
      {boost::asio::buffer(&version_, 1), boost::asio::buffer(&command_, 1),
       boost::asio::buffer(&port_high_byte_, 1),
       boost::asio::buffer(&port_low_byte_, 1), boost::asio::buffer(address_),
       boost::asio::buffer(name_), boost::asio::buffer(&null_byte_, 1)}};
  if (Is4aVersion()) {
    buf.push_back(boost::asio::buffer(domain_));
    buf.push_back(boost::asio::buffer(&null_byte_, 1));
  }
  return buf;
}

}  // v4
}  // socks
}  // network
}  // ssf
