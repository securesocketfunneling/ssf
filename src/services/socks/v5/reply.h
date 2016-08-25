#ifndef SSF_SERVICES_SOCKS_V5_REPLY_H_
#define SSF_SERVICES_SOCKS_V5_REPLY_H_

#include <cstdint>
#include <array>

#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/asio/basic_stream_socket.hpp>

#include <boost/system/error_code.hpp>

namespace ssf {
namespace socks {
namespace v5 {

class Reply {
 public:
  // Status constants
  enum Status {
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
  enum AddressTypeId { kIPv4 = 0x01, kDNS = 0x03, kIPv6 = 0x04 };

 public:
  Reply(const boost::system::error_code&);

  void set_ipv4(boost::asio::ip::address_v4::bytes_type ipv4);
  void set_domain(std::vector<char> domain);
  void set_ipv6(boost::asio::ip::address_v6::bytes_type ipv6);

  void set_port(uint16_t port);

  std::vector<boost::asio::const_buffer> Buffers() const;

 private:
  Status ErrorCodeToStatus(const boost::system::error_code& err);

 private:
  uint8_t version_;
  uint8_t status_;
  uint8_t reserved_;
  uint8_t addr_type_;

  boost::asio::ip::address_v4::bytes_type ipv4_;

  uint8_t domainLength_;
  std::vector<char> domain_;

  boost::asio::ip::address_v6::bytes_type ipv6_;

  uint8_t port_high_byte_;
  uint8_t port_low_byte_;
};

template <class VerifyHandler, class StreamSocket>
void AsyncSendReply(StreamSocket& c, const Reply& r, VerifyHandler handler) {
  boost::asio::async_write(c, r.Buffers(), handler);
}

}  // v5
}  // socks
}  // ssf

#endif  // SSF_SERVICES_SOCKS_V5_REPLY_H_
