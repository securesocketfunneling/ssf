#ifndef SSF_V5_REQUEST_AUTH_H_
#define SSF_V5_REQUEST_AUTH_H_

#include <cstdint>
#include <array>  // NOLINT

#include <boost/asio/read.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/buffer.hpp>  // NOLINT

#include <boost/tuple/tuple.hpp>  // NOLINT
#include <boost/system/error_code.hpp>

namespace ssf { namespace socks { namespace v5 {
//-----------------------------------------------------------------------------
class RequestAuth {
 public:
   // Authentication method constants
  enum AuthMethodIds {
    kNoAuth = 0x00,
    kGSSAPI = 0x01,
    kUserPassword = 0x02,
    kError = 0xFF
  };

 public:
  uint8_t numberOfAuthSupported() const;
  void addAuthMethod(uint8_t authMethod);

  std::array<boost::asio::mutable_buffer, 1> Buffers();

  bool is_no_auth_present();

 private:
  uint8_t numberOfAuthSupported_;
  std::vector<uint8_t> authMethods_;
};


//-----------------------------------------------------------------------------
//  R E A D    A U T H E N T I C A T I O N    R E Q U E S T
//-----------------------------------------------------------------------------

template<class VerifyHandler, class StreamSocket>
class ReadRequestAuthCoro : public boost::asio::coroutine {
public:
  ReadRequestAuthCoro(StreamSocket& c, RequestAuth* p_r, VerifyHandler h)
    : c_(c), r_(*p_r), handler_(h), total_length_(0) { }

#include <boost/asio/yield.hpp>  // NOLINT
  void operator()(const boost::system::error_code& ec, std::size_t length) {
    uint8_t numberOfMethods = 0;
    uint8_t method = 0;

    if (!ec) reenter(this) {
      // Read Request fixed size number of authentication methods
      yield boost::asio::async_read(c_, r_.Buffers(), std::move(*this));
      total_length_ += length;

      // Read each supported method
      for (numberOfMethods = 0; numberOfMethods < r_.numberOfAuthSupported(); ++numberOfMethods) {
        yield boost::asio::async_read(c_, 
                                      boost::asio::mutable_buffers_1(&method, 1), 
                                      std::move(*this));
        r_.addAuthMethod(method);
        total_length_ += length;
      }

      boost::get<0>(handler_)(ec, total_length_);
    } else {
      boost::get<0>(handler_)(ec, total_length_);
    }
  }
#include <boost/asio/unyield.hpp>  // NOLINT

 private:
  StreamSocket& c_;
  RequestAuth& r_;
  VerifyHandler handler_;
  std::size_t total_length_;
};


template<class VerifyHandler, class StreamSocket>
void AsyncReadRequestAuth(StreamSocket& c, RequestAuth* p_r, VerifyHandler handler) {
  ReadRequestAuthCoro<boost::tuple<VerifyHandler>, StreamSocket>
    RequestAuthReader(c, p_r, boost::make_tuple(handler));

  RequestAuthReader(boost::system::error_code(), 0);
}

//-----------------------------------------------------------------------------
}  // v5
}  // socks
}  // ssf


#endif  // SSF_V5_REQUEST_AUTH_H_
