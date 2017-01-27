#ifndef SSF_NETWORK_SOCKS_V5_REQUEST_H_
#define SSF_NETWORK_SOCKS_V5_REQUEST_H_

#include <cstdint>
#include <string>
#include <array>
#include <memory>
#include <vector>

#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/asio/buffer.hpp>

#include <boost/system/error_code.hpp>

#include "common/error/error.h"

#include "ssf/network/socks/v5/types.h"

namespace ssf {
namespace network {
namespace socks {
namespace v5 {

class Request {
 public:
  void Init(const std::string& target_addr, uint16_t target_port,
            boost::system::error_code& ec);

  uint8_t version() const { return version_; }

  uint8_t command() const { return command_; }

  uint8_t address_type() const { return address_type_; }

  boost::asio::ip::address_v4::bytes_type ipv4() const { return ipv4_; }

  uint8_t domain_length() const { return domain_length_; }

  std::vector<char> domain() const { return domain_; }

  boost::asio::ip::address_v6::bytes_type ipv6() const { return ipv6_; }

  uint16_t port() const;

  std::vector<boost::asio::const_buffer> ConstBuffers() const;

  std::array<boost::asio::mutable_buffer, 4> FirstPartBuffers();
  std::array<boost::asio::mutable_buffer, 1> DomainLengthBuffer();
  std::vector<boost::asio::mutable_buffer> AddressBuffer();
  std::array<boost::asio::mutable_buffer, 2> PortBuffers();

 private:
  uint8_t version_;
  uint8_t command_;
  uint8_t reserved_;
  uint8_t address_type_;

  boost::asio::ip::address_v4::bytes_type ipv4_;

  uint8_t domain_length_;
  std::vector<char> domain_;

  boost::asio::ip::address_v6::bytes_type ipv6_;

  uint8_t port_high_byte_;
  uint8_t port_low_byte_;
};

}  // v5
}  // socks
}  // network
}  // ssf

#endif  // SSF_NETWORK_SOCKS_V5_REQUEST_H_
