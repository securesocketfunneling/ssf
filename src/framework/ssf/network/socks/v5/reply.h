#ifndef SSF_NETWORK_SOCKS_V5_REPLY_H_
#define SSF_NETWORK_SOCKS_V5_REPLY_H_

#include <cstdint>
#include <array>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>

#include <boost/system/error_code.hpp>

namespace ssf {
namespace network {
namespace socks {
namespace v5 {

class Reply {
 public:
  // Status constants
  enum class Status : uint8_t {
    kGranted = 0x00,
    kFailed = 0x01,
    kConnNotAllowed = 0x02,
    kNetUnreach = 0x03,
    kHostUnreach = 0x04,
    kConnRefused = 0x05,
    kTTLEx = 0x06,
    kError = 0x07,
    kBadAddrType = 0x08
  };

  // Address type constants
  enum class AddressType : uint8_t { kIPv4 = 0x01, kDNS = 0x03, kIPv6 = 0x04 };

 public:
  Reply();
  Reply(const boost::system::error_code&);

  void Reset();

  uint8_t domain_length() const { return domain_length_; }

  uint8_t status() const { return status_; }

  bool AccessGranted() const {
    return status_ == static_cast<uint8_t>(Status::kGranted);
  }

  bool IsAddrTypeSet() const {
    return addr_type_ == static_cast<uint8_t>(AddressType::kIPv4) ||
           addr_type_ == static_cast<uint8_t>(AddressType::kDNS) ||
           addr_type_ == static_cast<uint8_t>(AddressType::kIPv6);
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
  Status ErrorCodeToStatus(const boost::system::error_code& err);

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
