#include "services/socks/v5/reply.h"

#include <iostream>  // NOLINT

namespace ssf { namespace socks { namespace v5 {
//-----------------------------------------------------------------------------
Reply::Reply(const boost::system::error_code& err)
  : version_(0x05),
    status_(ErrorCodeToStatus(err)),
    reserved_(0x00) { }

//-----------------------------------------------------------------------------
void Reply::set_ipv4(boost::asio::ip::address_v4::bytes_type ipv4) {
  addr_type_ = 0x01;
  ipv4_ = ipv4;
}

void Reply::set_domain(std::vector<char> domain) {
  addr_type_ = 0x03;
  domainLength_ = (uint8_t)domain.size();
  domain_ = domain;
}

void Reply::set_ipv6(boost::asio::ip::address_v6::bytes_type ipv6) {
  addr_type_ = 0x04;
  ipv6_ = ipv6;
}

void Reply::set_port(uint16_t port) {
  port_high_byte_ = (port >> 8) & 0xff;
  port_low_byte_ = port & 0xff;
}

//-----------------------------------------------------------------------------
std::vector<boost::asio::const_buffer> Reply::Buffers() const {
  std::vector<boost::asio::const_buffer> buf;

  buf.push_back(boost::asio::buffer(&version_, 1));
  buf.push_back(boost::asio::buffer(&status_, 1));
  buf.push_back(boost::asio::buffer(&reserved_, 1));
  buf.push_back(boost::asio::buffer(&addr_type_, 1));

  switch (addr_type_) {
  case kIPv4:
    buf.push_back(boost::asio::buffer(ipv4_));
    break;
  case kDNS:
    buf.push_back(boost::asio::buffer(&domainLength_, 1));
    buf.push_back(boost::asio::buffer(domain_));
    break;
  case kIPv6:
    buf.push_back(boost::asio::buffer(ipv6_));
    break;
  }

  buf.push_back(boost::asio::buffer(&port_high_byte_, 1));
  buf.push_back(boost::asio::buffer(&port_low_byte_, 1));
  
  return buf;
}


//-----------------------------------------------------------------------------
Reply::Status Reply::ErrorCodeToStatus(const boost::system::error_code& err) {
  Status s = kFailed;

  if (!err) {
    s = kGranted;
  }
  return s;
}
}  // v5
}  // socks
}  // ssf
