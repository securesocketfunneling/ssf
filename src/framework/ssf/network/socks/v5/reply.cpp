#include "ssf/network/socks/v5/reply.h"

#include "ssf/network/socks/socks.h"

#include "ssf/utils/enum.h"

namespace ssf {
namespace network {
namespace socks {
namespace v5 {

Reply::Reply()
    : version_(ToIntegral(Socks::Version::kV5)),
      reserved_(0x00),
      addr_type_(0x00),
      domain_length_(0),
      domain_(0),
      port_high_byte_(0),
      port_low_byte_(0) {}

Reply::Reply(const boost::system::error_code& err)
    : version_(ToIntegral(Socks::Version::kV5)),
      status_(ToIntegral(Status::kFailed)),
      reserved_(0x00) {
  if (!err) {
    status_ = ToIntegral(Status::kGranted);
  }
}

void Reply::Reset() {
  version_ = ToIntegral(Socks::Version::kV5);
  addr_type_ = 0x00;
  reserved_ = 0x00;
  domain_length_ = 0;
  domain_.resize(0);
  port_high_byte_ = 0;
  port_low_byte_ = 0;
}

bool Reply::IsComplete() const {
  if (!IsAddrTypeSet()) {
    return false;
  }

  return IsPortSet();
}

void Reply::set_ipv4(boost::asio::ip::address_v4::bytes_type ipv4) {
  addr_type_ = ToIntegral(AddressType::kIPv4);
  ipv4_ = ipv4;
}

void Reply::set_domain(const std::vector<char>& domain) {
  addr_type_ = ToIntegral(AddressType::kDNS);
  domain_length_ = static_cast<uint8_t>(domain.size());
  domain_ = domain;
}

void Reply::set_ipv6(boost::asio::ip::address_v6::bytes_type ipv6) {
  addr_type_ = ToIntegral(AddressType::kIPv6);
  ipv6_ = ipv6;
}

void Reply::set_port(uint16_t port) {
  port_high_byte_ = (port >> 8) & 0xff;
  port_low_byte_ = port & 0xff;
}

std::vector<boost::asio::const_buffer> Reply::Buffers() const {
  std::vector<boost::asio::const_buffer> buf;

  buf.push_back(boost::asio::buffer(&version_, 1));
  buf.push_back(boost::asio::buffer(&status_, 1));
  buf.push_back(boost::asio::buffer(&reserved_, 1));
  buf.push_back(boost::asio::buffer(&addr_type_, 1));

  switch (addr_type_) {
    case static_cast<uint8_t>(AddressType::kIPv4):
      buf.push_back(boost::asio::buffer(ipv4_));
      break;
    case static_cast<uint8_t>(AddressType::kDNS):
      buf.push_back(boost::asio::buffer(&domain_length_, 1));
      buf.push_back(boost::asio::buffer(domain_));
      break;
    case static_cast<uint8_t>(AddressType::kIPv6):
      buf.push_back(boost::asio::buffer(ipv6_));
      break;
  }

  buf.push_back(boost::asio::buffer(&port_high_byte_, 1));
  buf.push_back(boost::asio::buffer(&port_low_byte_, 1));

  return buf;
}

std::vector<boost::asio::mutable_buffer> Reply::MutBaseBuffers() {
  std::vector<boost::asio::mutable_buffer> buf;

  buf.push_back(boost::asio::buffer(&version_, 1));
  buf.push_back(boost::asio::buffer(&status_, 1));
  buf.push_back(boost::asio::buffer(&reserved_, 1));
  buf.push_back(boost::asio::buffer(&addr_type_, 1));

  return buf;
}

std::vector<boost::asio::mutable_buffer> Reply::MutDynamicBuffers() {
  std::vector<boost::asio::mutable_buffer> buf;
  switch (addr_type_) {
    case static_cast<uint8_t>(AddressType::kIPv4):
      buf = MutIPV4Buffers();
      break;
    case static_cast<uint8_t>(AddressType::kDNS):
      if (domain_length_ == 0) {
        buf = MutDomainLengthBuffers();
      } else {
        buf = MutDomainBuffers();
      }
      break;
    case static_cast<uint8_t>(AddressType::kIPv6):
      buf = MutIPV4Buffers();
      break;
  }

  return buf;
}

std::vector<boost::asio::mutable_buffer> Reply::MutIPV4Buffers() {
  std::vector<boost::asio::mutable_buffer> buf;
  buf.push_back(boost::asio::buffer(ipv4_));
  buf.push_back(boost::asio::buffer(&port_high_byte_, 1));
  buf.push_back(boost::asio::buffer(&port_low_byte_, 1));
  return buf;
}

std::vector<boost::asio::mutable_buffer> Reply::MutIPV6Buffers() {
  std::vector<boost::asio::mutable_buffer> buf;
  buf.push_back(boost::asio::buffer(ipv6_));
  buf.push_back(boost::asio::buffer(&port_high_byte_, 1));
  buf.push_back(boost::asio::buffer(&port_low_byte_, 1));
  return buf;
}

std::vector<boost::asio::mutable_buffer> Reply::MutDomainLengthBuffers() {
  std::vector<boost::asio::mutable_buffer> buf;
  buf.push_back(boost::asio::buffer(&domain_length_, 1));
  return buf;
}

std::vector<boost::asio::mutable_buffer> Reply::MutDomainBuffers() {
  std::vector<boost::asio::mutable_buffer> buf;
  domain_.resize(domain_length_);
  buf.push_back(boost::asio::buffer(domain_));
  buf.push_back(boost::asio::buffer(&port_high_byte_, 1));
  buf.push_back(boost::asio::buffer(&port_low_byte_, 1));
  return buf;
}

}  // v5
}  // socks
}  // network
}  // ssf
