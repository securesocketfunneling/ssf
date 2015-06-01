#ifndef SSF_V4_REQUEST_H_
#define SSF_V4_REQUEST_H_

#include <cstdint>
#include <string>
#include <array>  // NOLINT
#include <memory>  // NOLINT

#include <boost/asio/read_until.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/ip/address_v4.hpp>  // NOLINT
#include <boost/asio/buffer.hpp>  // NOLINT
#include <boost/asio/ip/tcp.hpp>  // NOLINT
#include <boost/asio/streambuf.hpp>

#include <boost/system/error_code.hpp>  // NOLINT
#include <boost/tuple/tuple.hpp>  // NOLINT

namespace ssf { namespace socks { namespace v4 {
//-----------------------------------------------------------------------------
class Request {
 public:
  enum CommandId {
    kConnect = 0x01,
    kBind = 0x02
  };

 public:
  uint8_t Command() const;

  boost::asio::ip::tcp::endpoint Endpoint() const;

  std::array<boost::asio::mutable_buffer, 4> Buffer();

  std::string Name() const;
  std::string Domain() const;
  uint16_t Port() const;

  void SetName(const std::string& name);
  void SetDomain(const std::string& domain);

  bool is_4a_version() const;

 private:
  uint8_t command_;
  uint8_t port_high_byte_;
  uint8_t port_low_byte_;
  boost::asio::ip::address_v4::bytes_type address_;
  std::string name_;
  std::string domain_;
};


//-----------------------------------------------------------------------------
//  R E A D    R E Q U E S T
//-----------------------------------------------------------------------------

template<class VerifyHandler, class StreamSocket>
class ReadRequestCoro : public boost::asio::coroutine {
public:
  ReadRequestCoro(StreamSocket& c, Request* p_r, VerifyHandler h)
    : c_(c), r_(*p_r), handler_(h), total_length_(0),
    p_stream_(new boost::asio::streambuf){ }

#include <boost/asio/yield.hpp>  // NOLINT
  void operator()(const boost::system::error_code& ec, std::size_t length) {
    std::istream is(p_stream_.get());
    std::string name;
    std::string domain;

    if (!ec) reenter(this) {
      // Read Request fixed size buffer
      yield boost::asio::async_read(c_, r_.Buffer(), std::move(*this));
      total_length_ += length;

      // Read Request variable size name (from now, until '\0')
      yield boost::asio::async_read_until(c_,
        *p_stream_, '\0', std::move(*this));
      total_length_ += length;

      // Set the name to complete the request
      r_.SetName(boost::asio::buffer_cast<const char*>(p_stream_->data()));
      p_stream_->consume(length);

      if (r_.is_4a_version()) {
        // Read Request variable size domain (from now, until '\0')
        yield boost::asio::async_read_until(c_,
          *p_stream_, '\0', std::move(*this));
        total_length_ += length;

        // Set the name to complete the request
        r_.SetDomain(boost::asio::buffer_cast<const char*>(p_stream_->data()));
      }

      boost::get<0>(handler_)(ec, total_length_);
    } else {
      boost::get<0>(handler_)(ec, total_length_);
    }
  }
#include <boost/asio/unyield.hpp>  // NOLINT

 private:
  StreamSocket& c_;
  Request& r_;
  VerifyHandler handler_;
  std::size_t total_length_;
  std::shared_ptr<boost::asio::streambuf> p_stream_;
};


template<class VerifyHandler, class StreamSocket>
void AsyncReadRequest(StreamSocket& c, Request* p_r, VerifyHandler handler) {
  ReadRequestCoro<boost::tuple<VerifyHandler>, StreamSocket>
    RequestReader(c, p_r, boost::make_tuple(handler));

  RequestReader(boost::system::error_code(), 0);
}

//-----------------------------------------------------------------------------
}  // v4
}  // socks

void print(const socks::v4::Request&);
}  // ssf


#endif  // SSF_V4_REQUEST_H_
