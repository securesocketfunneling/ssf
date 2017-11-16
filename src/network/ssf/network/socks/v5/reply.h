#ifndef SSF_NETWORK_SOCKS_V5_REPLY_H_
#define SSF_NETWORK_SOCKS_V5_REPLY_H_

#include <cstdint>
#include <array>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>

#include <boost/system/error_code.hpp>

#include "ssf/network/socks/v5/types.h"
#include "ssf/utils/enum.h"

namespace ssf {
namespace network {
namespace socks {
namespace v5 {

class Reply {
 public:

  // Address type constants
  enum class AddressType : uint8_t { kIPv4 = 0x01, kDNS = 0x03, kIPv6 = 0x04 };

 public:
  Reply();
  Reply(CommandStatus reply_status);

  void Reset();

  uint8_t domain_length() const { return domain_length_; }

  uint8_t status() const { return status_; }

  bool AccessGranted() const {
    return status_ == ToIntegral(CommandStatus::kSucceeded);
  }

  bool IsAddrTypeSet() const {
    return addr_type_ == ToIntegral(AddressType::kIPv4) ||
           addr_type_ == ToIntegral(AddressType::kDNS) ||
           addr_type_ == ToIntegral(AddressType::kIPv6);
  }

  bool IsPortSet() const { return port_high_byte_ != 0 || port_low_byte_ != 0; }

  bool IsComplete() const;

  void set_ipv4(boost::asio::ip::address_v4::bytes_type ipv4);
  void set_domain(const std::vector<char>& domain);
  void set_ipv6(boost::asio::ip::address_v6::bytes_type ipv6);

  void set_port(uint16_t port);

  std::vector<boost::asio::const_buffer> Buffers() const;

  std::vector<boost::asio::mutable_buffer> MutBaseBuffers();
  std::vector<boost::asio::mutable_buffer> MutDynamicBuffers();

  std::vector<boost::asio::mutable_buffer> MutIPV4Buffers();
  std::vector<boost::asio::mutable_buffer> MutIPV6Buffers();
  std::vector<boost::asio::mutable_buffer> MutDomainLengthBuffers();
  std::vector<boost::asio::mutable_buffer> MutDomainBuffers();

 private:
  uint8_t version_;
  uint8_t status_;
  uint8_t reserved_;
  uint8_t addr_type_;

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

#endif  // SSF_NETWORK_SOCKS_V5_REPLY_H_
