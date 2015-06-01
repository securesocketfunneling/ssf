#include "services/socks/v5/request.h"

#include <string>  // NOLINT
#include <iostream>  // NOLINT

#include <boost/asio.hpp>  // NOLINT
#include <boost/asio/coroutine.hpp>  // NOLINT


namespace ssf { namespace socks { namespace v5 {
//-----------------------------------------------------------------------------
uint8_t Request::version() const { return version_; }

uint8_t Request::command() const { return command_; }

uint8_t Request::addressType() const { return addressType_; }

boost::asio::ip::address_v4::bytes_type Request::ipv4() const { return ipv4_; }

uint8_t Request::domainLength() const { return domainLength_; }

std::vector<char> Request::domain() const { return domain_; }

boost::asio::ip::address_v6::bytes_type Request::ipv6() const { return ipv6_; }

uint16_t Request::port() const { 
  uint16_t port = port_high_byte_;
  port = (port << 8) & 0xff00;
  port = port | port_low_byte_;
  return port;
}

//-----------------------------------------------------------------------------
std::array<boost::asio::mutable_buffer, 4> Request::First_Part_Buffers() {
  std::array<boost::asio::mutable_buffer, 4> buf = {
    {
      boost::asio::buffer(&version_, 1),
      boost::asio::buffer(&command_, 1),
      boost::asio::buffer(&reserved_, 1),
      boost::asio::buffer(&addressType_, 1)
    }
  };
  return buf;
}

boost::asio::mutable_buffers_1 Request::Domain_Length_Buffer() {
  return boost::asio::mutable_buffers_1(&domainLength_, 1);
}

std::vector<boost::asio::mutable_buffer> Request::Address_Buffer() {
  std::vector<boost::asio::mutable_buffer> buf;

  switch (addressType_) {
  case kIPv4:
    buf.push_back(boost::asio::buffer(ipv4_));
    break;
  case kDNS:
    while (domain_.size() < domainLength_) {
      domain_.push_back(0);
    }
    for (size_t i = 0; i < domainLength_; ++i) {
      buf.push_back(boost::asio::mutable_buffer(&(domain_[i]), 1));
    }
    break;
  case kIPv6:
    buf.push_back(boost::asio::buffer(ipv6_));
  }

  return buf;
}

std::array<boost::asio::mutable_buffer, 2> Request::Port_Buffers() {
  std::array<boost::asio::mutable_buffer, 2> buf = {
    {
      boost::asio::buffer(&port_high_byte_, 1),
      boost::asio::buffer(&port_low_byte_, 1)
    }
  };
  return buf;
}

}  // v5
}  // socks
}  // ssf

