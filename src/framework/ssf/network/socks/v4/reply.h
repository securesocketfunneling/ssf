#ifndef SSF_NETWORK_SOCKS_V4_REPLY_H_
#define SSF_NETWORK_SOCKS_V4_REPLY_H_

#include <cstdint>
#include <array>

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/system/error_code.hpp>

namespace ssf {
namespace network {
namespace socks {
namespace v4 {

class Reply {
 public:
  // Status constants
  enum class Status : uint8_t {
    kGranted = 0x5a,
    kFailed = 0x5b,
    kFailedNoIdentd = 0x5c,
    kFailedBadUser = 0x5d
  };

 public:
  Reply();
  Reply(const boost::system::error_code&,
        const boost::asio::ip::tcp::endpoint&);

  uint8_t null_byte() { return null_byte_; }

  Status status() const { return Status(status_); }

  std::array<boost::asio::const_buffer, 5> ConstBuffer() const;
  std::array<boost::asio::mutable_buffer, 5> MutBuffer();

 private:
  Status ErrorCodeToStatus(const boost::system::error_code& err);

 private:
  uint8_t null_byte_;
  uint8_t status_;
  uint8_t port_high_byte_;
  uint8_t port_low_byte_;
  boost::asio::ip::address_v4::bytes_type address_;
};

}  // v4
}  // socks
}  // network
}  // ssf

#endif  // SSF_NETWORK_SOCKS_V4_REPLY_H_
