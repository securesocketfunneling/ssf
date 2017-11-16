#include "ssf/network/socks/v5/request.h"

#include <boost/asio/ip/address.hpp>

#include "ssf/error/error.h"
#include "ssf/log/log.h"

#include "ssf/network/socks/socks.h"

#include "ssf/utils/enum.h"

namespace ssf {
namespace network {
namespace socks {
namespace v5 {

void Request::Init(const std::string& target_addr, uint16_t target_port,
                   boost::system::error_code& ec) {
  version_ = ToIntegral(Socks::Version::kV5);
  command_ = ToIntegral(CommandType::kConnect);
  reserved_ = 0x00;

  boost::system::error_code addr_ec;
  auto ip_addr = boost::asio::ip::address::from_string(target_addr, addr_ec);
  if (addr_ec) {
    if (target_addr.size() > 255) {
      ec.assign(ssf::error::bad_address, ssf::error::get_ssf_category());
      return;
    }

    // addr is a domain name
    address_type_ = ToIntegral(AddressType::kDNS);
    domain_length_ = static_cast<uint8_t>(target_addr.size());
    std::copy(target_addr.begin(), target_addr.end(),
              std::back_inserter(domain_));
  } else {
    // addr is an IP address
    if (ip_addr.is_v4()) {
      address_type_ = ToIntegral(AddressType::kIPv4);
      ipv4_ = ip_addr.to_v4().to_bytes();
    } else {
      address_type_ = ToIntegral(AddressType::kIPv6);
      ipv6_ = ip_addr.to_v6().to_bytes();
    }
  }

  port_high_byte_ = (target_port >> 8);
  port_low_byte_ = (target_port & 0x00ff);
}

uint16_t Request::port() const {
  uint16_t port = port_high_byte_;
  port = (port << 8) & 0xff00;
  port = port | port_low_byte_;
  return port;
}

std::vector<boost::asio::const_buffer> Request::ConstBuffers() const {
  std::vector<boost::asio::const_buffer> buf = {
      {boost::asio::buffer(&version_, 1), boost::asio::buffer(&command_, 1),
       boost::asio::buffer(&reserved_, 1),
       boost::asio::buffer(&address_type_, 1)}};

  switch (address_type_) {
    case static_cast<uint8_t>(AddressType::kIPv4):
      buf.push_back(boost::asio::buffer(ipv4_));
      break;
    case static_cast<uint8_t>(AddressType::kIPv6):
      buf.push_back(boost::asio::buffer(ipv6_));
      break;
    case static_cast<uint8_t>(AddressType::kDNS):
      buf.push_back(boost::asio::buffer(&domain_length_, 1));
      buf.push_back(boost::asio::buffer(domain_));
      break;
  }
  buf.push_back(boost::asio::buffer(&port_high_byte_, 1));
  buf.push_back(boost::asio::buffer(&port_low_byte_, 1));

  return buf;
}

std::array<boost::asio::mutable_buffer, 4> Request::FirstPartBuffers() {
  return {{boost::asio::buffer(&version_, 1), boost::asio::buffer(&command_, 1),
           boost::asio::buffer(&reserved_, 1),
           boost::asio::buffer(&address_type_, 1)}};
}

std::array<boost::asio::mutable_buffer, 1> Request::DomainLengthBuffer() {
  return {{boost::asio::buffer(&domain_length_, 1)}};
}

std::vector<boost::asio::mutable_buffer> Request::AddressBuffer() {
  std::vector<boost::asio::mutable_buffer> buf;

  switch (address_type_) {
    case static_cast<uint8_t>(AddressType::kIPv4):
      buf.push_back(boost::asio::buffer(ipv4_));
      break;
    case static_cast<uint8_t>(AddressType::kDNS):
      domain_.resize(domain_length_);
      buf.push_back(boost::asio::buffer(domain_));
      break;
    case static_cast<uint8_t>(AddressType::kIPv6):
      buf.push_back(boost::asio::buffer(ipv6_));
  }

  return buf;
}

std::array<boost::asio::mutable_buffer, 2> Request::PortBuffers() {
  return {{boost::asio::buffer(&port_high_byte_, 1),
           boost::asio::buffer(&port_low_byte_, 1)}};
}

}  // v5
}  // socks
}  // network
}  // ssf
