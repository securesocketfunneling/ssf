#ifndef SSF_SERVICES_SOCKS_V5_REQUEST_H_
#define SSF_SERVICES_SOCKS_V5_REQUEST_H_

#include <cstdint>
#include <string>
#include <array>   // NOLINT
#include <memory>  // NOLINT

#include <boost/asio/read.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/ip/address_v4.hpp>  // NOLINT
#include <boost/asio/buffer.hpp>         // NOLINT
#include <boost/asio/ip/tcp.hpp>         // NOLINT
#include <boost/asio/streambuf.hpp>

#include <boost/system/error_code.hpp>  // NOLINT
#include <boost/tuple/tuple.hpp>        // NOLINT

#include "common/error/error.h"

namespace ssf {
namespace socks {
namespace v5 {

class Request {
 public:
  // Command types constants
  enum CommandId { kConnect = 0x01, kBind = 0x02, kUDP = 0x03 };

  // Address type constants
  enum AddressTypeId { kIPv4 = 0x01, kDNS = 0x03, kIPv6 = 0x04 };

 public:
  uint8_t version() const;
  uint8_t command() const;
  uint8_t addressType() const;
  boost::asio::ip::address_v4::bytes_type ipv4() const;
  uint8_t domainLength() const;
  std::vector<char> domain() const;
  boost::asio::ip::address_v6::bytes_type ipv6() const;
  uint16_t port() const;

  std::array<boost::asio::mutable_buffer, 4> First_Part_Buffers();
  boost::asio::mutable_buffers_1 Domain_Length_Buffer();
  std::vector<boost::asio::mutable_buffer> Address_Buffer();
  std::array<boost::asio::mutable_buffer, 2> Port_Buffers();

 private:
  uint8_t version_;
  uint8_t command_;
  uint8_t reserved_;
  uint8_t addressType_;

  boost::asio::ip::address_v4::bytes_type ipv4_;

  uint8_t domainLength_;
  std::vector<char> domain_;

  boost::asio::ip::address_v6::bytes_type ipv6_;

  uint8_t port_high_byte_;
  uint8_t port_low_byte_;
};

template <class VerifyHandler, class StreamSocket>
class ReadRequestCoro : public boost::asio::coroutine {
 public:
  ReadRequestCoro(StreamSocket& c, Request* p_r, VerifyHandler h)
      : c_(c), r_(*p_r), handler_(h), total_length_(0) {}

#include <boost/asio/yield.hpp>  // NOLINT
  void operator()(const boost::system::error_code& ec, std::size_t length) {
    if (!ec) reenter(this) {
        // Read Request fixed size buffer
        yield boost::asio::async_read(c_, r_.First_Part_Buffers(),
                                      std::move(*this));
        total_length_ += length;

        // Read the address
        if (r_.addressType() == Request::kIPv4) {
          // Read IPv4 address
          yield boost::asio::async_read(c_, r_.Address_Buffer(),
                                        std::move(*this));
          total_length_ += length;
        } else if (r_.addressType() == Request::kDNS) {
          // Read length of the domain name
          yield boost::asio::async_read(c_, r_.Domain_Length_Buffer(),
                                        std::move(*this));
          total_length_ += length;
          // Read domain name
          yield boost::asio::async_read(c_, r_.Address_Buffer(),
                                        std::move(*this));
          total_length_ += length;
        } else if (r_.addressType() == Request::kIPv6) {
          // Read IPv6 address
          yield boost::asio::async_read(c_, r_.Address_Buffer(),
                                        std::move(*this));
          total_length_ += length;
        } else {
          boost::get<0>(handler_)(
              boost::system::error_code(::error::protocol_error,
                                        ::error::get_ssf_category()),
              total_length_);
        }

        yield boost::asio::async_read(c_, r_.Port_Buffers(), std::move(*this));
        total_length_ += length;

        boost::get<0>(handler_)(ec, total_length_);
      }
    else {
      boost::get<0>(handler_)(ec, total_length_);
    }
  }
#include <boost/asio/unyield.hpp>  // NOLINT

 private:
  StreamSocket& c_;
  Request& r_;
  VerifyHandler handler_;
  std::size_t total_length_;
};

template <class VerifyHandler, class StreamSocket>
void AsyncReadRequest(StreamSocket& c, Request* p_r, VerifyHandler handler) {
  ReadRequestCoro<boost::tuple<VerifyHandler>, StreamSocket> RequestReader(
      c, p_r, boost::make_tuple(handler));

  RequestReader(boost::system::error_code(), 0);
}

}  // v5
}  // socks
}  // ssf

#endif  // SSF_SERVICES_SOCKS_V5_REQUEST_H_
