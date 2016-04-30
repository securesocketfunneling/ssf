#ifndef SSF_SERVICES_SOCKS_V4_REPLY_H_
#define SSF_SERVICES_SOCKS_V4_REPLY_H_

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
namespace v4 {

class Reply {
 public:
  // Status constants
  enum Status {
    kGranted = 0x5a,
    kFailed = 0x5b,
    kFailedNoIdentd = 0x5c,
    kFailedBadUser = 0x5d
  };

 public:
  Reply(const boost::system::error_code&,
        const boost::asio::ip::tcp::endpoint&);

  std::array<boost::asio::const_buffer, 5> Buffer() const;

 private:
  Status ErrorCodeToStatus(const boost::system::error_code& err);

 private:
  uint8_t null_byte_;
  uint8_t status_;
  uint8_t port_high_byte_;
  uint8_t port_low_byte_;
  boost::asio::ip::address_v4::bytes_type address_;
};

template <class VerifyHandler, class StreamSocket>
void AsyncSendReply(StreamSocket& c, const Reply& r, VerifyHandler handler) {
  boost::asio::async_write(c, r.Buffer(), handler);
}

}  // v4
}  // socks
}  // ssf

#endif  // SSF_SERVICES_SOCKS_V4_REPLY_H_
