#ifndef SSF_SERVICES_SOCKS_V5_SESSION_H_
#define SSF_SERVICES_SOCKS_V5_SESSION_H_

#include <memory>

#include <boost/noncopyable.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>

#include <ssf/network/base_session.h>
#include <ssf/network/socket_link.h>
#include <ssf/network/manager.h>
#include <ssf/network/base_session.h>

#include <ssf/utils/enum.h>

#include "ssf/network/socks/v5/request.h"
#include "ssf/network/socks/v5/request_auth.h"
#include "ssf/network/socks/v5/reply.h"
#include "ssf/network/socks/v5/reply_auth.h"

#include "common/boost/fiber/stream_fiber.hpp"

namespace ssf {
namespace socks {
namespace v5 {

template <typename Demux>
class Session : public ssf::BaseSession {
 private:
  using StreamBuff = std::array<char, 50 * 1024>;

  using socket = boost::asio::ip::tcp::socket;
  using fiber = typename boost::asio::fiber::stream_fiber<
      typename Demux::socket_type>::socket;

  using RequestAuth = ssf::network::socks::v5::RequestAuth;
  using Request = ssf::network::socks::v5::Request;
  using AuthReply = ssf::network::socks::v5::AuthReply;
  using Reply = ssf::network::socks::v5::Reply;

  using SessionManager = ItemManager<BaseSessionPtr>;

 public:
  Session(SessionManager* sm, fiber client);

 public:
  virtual void start(boost::system::error_code&);

  virtual void stop(boost::system::error_code&);

 private:
  void HandleRequestAuthDispatch(const boost::system::error_code& ec,
                                 std::size_t);

  void DoNoAuth();
  void DoErrorAuth();

  void HandleReplyAuthSent();

  void HandleRequestDispatch(const boost::system::error_code&, std::size_t);

  void DoConnectRequest();

  void DoBindRequest();

  void DoUDPRequest();

  void HandleApplicationServerConnect(const boost::system::error_code&);

  void EstablishLink();

  void HandleStop();

 private:
  std::shared_ptr<Session> self_shared_from_this() {
    return std::static_pointer_cast<Session>(shared_from_this());
  }

 private:
  boost::asio::io_service& io_service_;
  SessionManager* p_session_manager_;

  fiber client_;
  socket app_server_;
  RequestAuth request_auth_;
  Request request_;

  std::shared_ptr<StreamBuff> upstream_;
  std::shared_ptr<StreamBuff> downstream_;
};

template <class VerifyHandler, class StreamSocket>
class ReadRequestAuthCoro : public boost::asio::coroutine {
 private:
  using RequestAuth = ssf::network::socks::v5::RequestAuth;

 public:
  ReadRequestAuthCoro(StreamSocket& c,
                      ssf::network::socks::v5::RequestAuth* p_r,
                      VerifyHandler h)
      : c_(c), r_(*p_r), handler_(h), total_length_(0) {}

#include <boost/asio/yield.hpp>  // NOLINT
  void operator()(const boost::system::error_code& ec, std::size_t length) {
    uint8_t method_count = 0;
    uint8_t method = 0;

    if (!ec) reenter(this) {
        // Read Request fixed size number of authentication methods
        yield boost::asio::async_read(c_, r_.MutBuffers(), std::move(*this));
        total_length_ += length;

        // Read each supported method
        for (method_count = 0; method_count < r_.auth_supported_count();
             ++method_count) {
          yield boost::asio::async_read(
              c_, boost::asio::mutable_buffers_1(&method, 1), std::move(*this));
          r_.AddAuthMethod(method);
          total_length_ += length;
        }

        boost::get<0>(handler_)(ec, total_length_);
      }
    else {
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

template <class VerifyHandler, class StreamSocket>
void AsyncReadRequestAuth(StreamSocket& c,
                          ssf::network::socks::v5::RequestAuth* p_r,
                          VerifyHandler handler) {
  ReadRequestAuthCoro<boost::tuple<VerifyHandler>, StreamSocket>
      RequestAuthReader(c, p_r, boost::make_tuple(handler));

  RequestAuthReader(boost::system::error_code(), 0);
}

template <class VerifyHandler, class StreamSocket>
void AsyncSendAuthReply(StreamSocket& c,
                        const ssf::network::socks::v5::AuthReply& r,
                        VerifyHandler handler) {
  boost::asio::async_write(c, r.ConstBuffer(), handler);
}

template <class VerifyHandler, class StreamSocket>
class ReadRequestCoro : public boost::asio::coroutine {
 private:
  using Request = ssf::network::socks::v5::Request;

 public:
  ReadRequestCoro(StreamSocket& c, ssf::network::socks::v5::Request* p_r,
                  VerifyHandler h)
      : c_(c), r_(*p_r), handler_(h), total_length_(0) {}

#include <boost/asio/yield.hpp>  // NOLINT
  void operator()(const boost::system::error_code& ec, std::size_t length) {
    if (!ec) reenter(this) {
        // Read Request fixed size buffer
        yield boost::asio::async_read(c_, r_.FirstPartBuffers(),
                                      std::move(*this));
        total_length_ += length;

        // Read the address
        if (r_.address_type() == ToIntegral(Request::AddressType::kIPv4)) {
          // Read IPv4 address
          yield boost::asio::async_read(c_, r_.AddressBuffer(),
                                        std::move(*this));
          total_length_ += length;
        } else if (r_.address_type() ==
                   ToIntegral(Request::AddressType::kDNS)) {
          // Read length of the domain name
          yield boost::asio::async_read(c_, r_.DomainLengthBuffer(),
                                        std::move(*this));
          total_length_ += length;
          // Read domain name
          yield boost::asio::async_read(c_, r_.AddressBuffer(),
                                        std::move(*this));
          total_length_ += length;
        } else if (r_.address_type() ==
                   ToIntegral(Request::AddressType::kIPv6)) {
          // Read IPv6 address
          yield boost::asio::async_read(c_, r_.AddressBuffer(),
                                        std::move(*this));
          total_length_ += length;
        } else {
          boost::get<0>(handler_)(
              boost::system::error_code(::error::protocol_error,
                                        ::error::get_ssf_category()),
              total_length_);
        }

        yield boost::asio::async_read(c_, r_.PortBuffers(), std::move(*this));
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
  ssf::network::socks::v5::Request& r_;
  VerifyHandler handler_;
  std::size_t total_length_;
};

template <class VerifyHandler, class StreamSocket>
void AsyncReadRequest(StreamSocket& c, ssf::network::socks::v5::Request* p_r,
                      VerifyHandler handler) {
  ReadRequestCoro<boost::tuple<VerifyHandler>, StreamSocket> RequestReader(
      c, p_r, boost::make_tuple(handler));

  RequestReader(boost::system::error_code(), 0);
}

template <class VerifyHandler, class StreamSocket>
void AsyncSendReply(StreamSocket& c, const ssf::network::socks::v5::Reply& r,
                    VerifyHandler handler) {
  boost::asio::async_write(c, r.Buffers(), handler);
}

}  // v5
}  // socks
}  // ssf

#include "services/socks/v5/session.ipp"

#endif  // SSF_SERVICES_SOCKS_V5_SESSION_H_
