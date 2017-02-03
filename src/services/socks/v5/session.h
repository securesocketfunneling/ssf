#ifndef SSF_SERVICES_SOCKS_V5_SESSION_H_
#define SSF_SERVICES_SOCKS_V5_SESSION_H_

#include <memory>

#include <boost/noncopyable.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>

#include <ssf/network/base_session.h>
#include <ssf/network/manager.h>
#include <ssf/network/socket_link.h>
#include <ssf/network/socks/v5/types.h>

#include <ssf/utils/enum.h>

#include "ssf/network/socks/v5/request.h"
#include "ssf/network/socks/v5/request_auth.h"
#include "ssf/network/socks/v5/reply.h"
#include "ssf/network/socks/v5/reply_auth.h"

#include "common/boost/fiber/stream_fiber.hpp"

namespace ssf {
namespace services {
namespace socks {

template <typename Demux>
class SocksServer;

namespace v5 {

template <typename Demux>
class Session : public ssf::BaseSession {
 private:
  using StreamBuf = std::array<char, 50 * 1024>;
  using Tcp = boost::asio::ip::tcp;

  using Fiber = typename boost::asio::fiber::stream_fiber<
      typename Demux::socket_type>::socket;

  using AuthMethod = ssf::network::socks::v5::AuthMethod;
  using RequestAuth = ssf::network::socks::v5::RequestAuth;

  using CommandType = ssf::network::socks::v5::CommandType;
  using AddressType = ssf::network::socks::v5::AddressType;
  using CommandStatus = ssf::network::socks::v5::CommandStatus;
  using ReplyAuth = ssf::network::socks::v5::ReplyAuth;
  using Request = ssf::network::socks::v5::Request;
  using Reply = ssf::network::socks::v5::Reply;

  using Server = SocksServer<Demux>;
  using SocksServerWPtr = std::weak_ptr<Server>;

 public:
  Session(SocksServerWPtr socks_server, Fiber client);

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

  void HandleResolveServerEndpoint(const boost::system::error_code& err,
                                   Tcp::resolver::iterator ep_it);

  void HandleApplicationServerConnect(const boost::system::error_code&);

  void DoErrorCommand(CommandStatus err_status);

  void EstablishLink();

  void HandleStop();

 private:
  std::shared_ptr<Session> SelfFromThis() {
    return std::static_pointer_cast<Session>(shared_from_this());
  }

 private:
  boost::asio::io_service& io_service_;
  SocksServerWPtr socks_server_;

  Fiber client_;
  Tcp::socket server_;
  Tcp::resolver server_resolver_;
  RequestAuth request_auth_;
  Request request_;

  std::shared_ptr<StreamBuf> upstream_;
  std::shared_ptr<StreamBuf> downstream_;
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
    if (ec) {
      handler_(ec, total_length_);
      return;
    }

    reenter(this) {
      // Read Request fixed size number of authentication methods
      yield boost::asio::async_read(c_, r_.MutAuthSupportedBuffers(),
                                    std::move(*this));

      total_length_ += length;

      // Read each supported method
      yield boost::asio::async_read(c_, r_.MutAuthBuffers(), std::move(*this));

      handler_(ec, total_length_);
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
  ReadRequestAuthCoro<VerifyHandler, StreamSocket> RequestAuthReader(c, p_r,
                                                                     handler);

  RequestAuthReader(boost::system::error_code(), 0);
}

template <class VerifyHandler, class StreamSocket>
void AsyncSendAuthReply(StreamSocket& c,
                        const ssf::network::socks::v5::ReplyAuth& r,
                        VerifyHandler handler) {
  boost::asio::async_write(c, r.ConstBuffer(), handler);
}

template <class VerifyHandler, class StreamSocket>
class ReadRequestCoro : public boost::asio::coroutine {
 private:
  using Request = ssf::network::socks::v5::Request;
  using CommandType = ssf::network::socks::v5::CommandType;
  using AddressType = ssf::network::socks::v5::AddressType;
  using CommandStatus = ssf::network::socks::v5::CommandStatus;

 public:
  ReadRequestCoro(StreamSocket& c, ssf::network::socks::v5::Request* p_r,
                  VerifyHandler h)
      : c_(c), r_(*p_r), handler_(h), total_length_(0) {}

#include <boost/asio/yield.hpp>  // NOLINT
  void operator()(const boost::system::error_code& ec, std::size_t length) {
    if (ec) {
      handler_(ec, total_length_);
      return;
    }

    reenter(this) {
      // Read Request fixed size buffer
      yield boost::asio::async_read(c_, r_.FirstPartBuffers(),
                                    std::move(*this));
      total_length_ += length;

      // Read the address (cannot use switch into stackless coroutine)
      if (r_.address_type() == ToIntegral(AddressType::kIPv4) ||
          r_.address_type() == ToIntegral(AddressType::kIPv6)) {
        // Read the address
        yield boost::asio::async_read(c_, r_.AddressBuffer(), std::move(*this));
        total_length_ += length;
      } else if (r_.address_type() == ToIntegral(AddressType::kDNS)) {
        yield boost::asio::async_read(c_, r_.DomainLengthBuffer(),
                                      std::move(*this));
        total_length_ += length;
        // Read domain name
        yield boost::asio::async_read(c_, r_.AddressBuffer(), std::move(*this));
        total_length_ += length;
      } else {
        // address type not supported
        handler_(boost::system::error_code(::error::protocol_error,
                                           ::error::get_ssf_category()),
                 total_length_);
        return;
      }

      yield boost::asio::async_read(c_, r_.PortBuffers(), std::move(*this));
      total_length_ += length;

      handler_(ec, total_length_);
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
  ReadRequestCoro<VerifyHandler, StreamSocket> RequestReader(c, p_r, handler);

  RequestReader(boost::system::error_code(), 0);
}

template <class VerifyHandler, class StreamSocket>
void AsyncSendReply(StreamSocket& c, const ssf::network::socks::v5::Reply& r,
                    VerifyHandler handler) {
  boost::asio::async_write(c, r.Buffers(), handler);
}

}  // v5
}  // socks
}  // services
}  // ssf

#include "services/socks/v5/session.ipp"

#endif  // SSF_SERVICES_SOCKS_V5_SESSION_H_
