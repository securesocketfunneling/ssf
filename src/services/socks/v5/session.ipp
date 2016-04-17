#ifndef SSF_SERVICES_SOCKS_V5_SESSION_IPP_
#define SSF_SERVICES_SOCKS_V5_SESSION_IPP_

#include <boost/bind.hpp>
#include <boost/bind/protect.hpp>
#include <boost/asio/spawn.hpp>

#include "services/socks/v5/request.h"
#include "services/socks/v5/request_auth.h"
#include "services/socks/v5/reply.h"
#include "services/socks/v5/reply_auth.h"

#include <boost/asio/basic_stream_socket.hpp>

namespace ssf {
namespace socks {
namespace v5 {

template <typename Demux>
Session<Demux>::Session(SessionManager* p_session_manager, fiber client)
    : ssf::BaseSession(),
      io_service_(client.get_io_service()),
      p_session_manager_(p_session_manager),
      client_(std::move(client)),
      app_server_(client.get_io_service()) {}

template <typename Demux>
void Session<Demux>::HandleStop() {
  boost::system::error_code ec;
  p_session_manager_->stop(shared_from_this(), ec);
}

template <typename Demux>
void Session<Demux>::stop(boost::system::error_code&) {
  client_.close();
  boost::system::error_code ec;
  app_server_.close(ec);
  if (ec) {
    BOOST_LOG_TRIVIAL(error) << "socks session: stop error " << ec.message()
                             << std::endl;
  }
}

template <typename Demux>
void Session<Demux>::start(boost::system::error_code&) {
  AsyncReadRequestAuth(
      client_, &request_auth_,
      boost::bind(&Session::HandleRequestAuthDispatch, self_shared_from_this(),
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

template <typename Demux>
void Session<Demux>::HandleRequestAuthDispatch(
    const boost::system::error_code& ec, std::size_t) {
  if (ec) {
    HandleStop();
    return;
  }

  // Check for compatible authentications
  if (request_auth_.is_no_auth_present()) {
    DoNoAuth();
  } else {
    DoErrorAuth();
  }
}

template <typename Demux>
void Session<Demux>::DoNoAuth() {
  auto self = self_shared_from_this();
  auto p_reply = std::make_shared<AuthReply>(0x00);

  AsyncSendAuthReply(client_, *p_reply,
                     [this, self, p_reply](boost::system::error_code ec,
                                           std::size_t transferred) {
                       if (ec) {
                         HandleStop();
                         return;
                       }
                       HandleReplyAuthSent();
                     });
}

template <typename Demux>
void Session<Demux>::DoErrorAuth() {
  auto self = self_shared_from_this();
  auto p_reply = std::make_shared<AuthReply>(0xFF);

  AsyncSendAuthReply(client_, *p_reply,
                     [this, self, p_reply](boost::system::error_code,
                                           std::size_t) { HandleStop(); });
}

template <typename Demux>
void Session<Demux>::HandleReplyAuthSent() {
  AsyncReadRequest(
      client_, &request_,
      boost::bind(&Session::HandleRequestDispatch, self_shared_from_this(),
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

template <typename Demux>
void Session<Demux>::HandleRequestDispatch(const boost::system::error_code& ec,
                                           std::size_t) {
  if (ec) {
    HandleStop();
    return;
  }

  // Check command asked
  switch (request_.command()) {
    case Request::kConnect:
      DoConnectRequest();
      break;
    case Request::kBind:
      DoBindRequest();
      break;
    case Request::kUDP:
      DoUDPRequest();
      break;
    default:
      BOOST_LOG_TRIVIAL(error) << "SOCKS session: Invalid v5 command";
      HandleStop();
      break;
  }
}

template <typename Demux>
void Session<Demux>::DoConnectRequest() {
  auto handler =
      boost::bind(&Session::HandleApplicationServerConnect,
                  self_shared_from_this(), boost::asio::placeholders::error);
  boost::system::error_code ec;
  uint16_t port = request_.port();

  if (request_.addressType() == Request::kIPv4) {
    boost::asio::ip::address_v4 address(request_.ipv4());
    app_server_.async_connect(boost::asio::ip::tcp::endpoint(address, port),
                              handler);
  } else if (request_.addressType() == Request::kDNS) {
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::resolver::query query(
        std::string(request_.domain().data(), request_.domain().size()),
        std::to_string(port));
    boost::asio::ip::tcp::resolver::iterator iterator(
        resolver.resolve(query, ec));
    if (ec) {
      handler(ec);
    } else {
      app_server_.async_connect(*iterator, handler);
    }
  } else if (request_.addressType() == Request::kIPv6) {
    boost::asio::ip::address_v6 address(request_.ipv6());
    app_server_.async_connect(boost::asio::ip::tcp::endpoint(address, port),
                              handler);
  } else {
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::resolver::query query(
        std::string(request_.domain().data(), request_.domain().size()),
        std::to_string(port));
    boost::asio::ip::tcp::resolver::iterator iterator(
        resolver.resolve(query, ec));
    if (ec) {
      handler(ec);
    } else {
      app_server_.async_connect(*iterator, handler);
    }
  }
}

template <typename Demux>
void Session<Demux>::DoBindRequest() {
  BOOST_LOG_TRIVIAL(error) << "SOCKS session: Bind Not implemented yet";
  HandleStop();
}

template <typename Demux>
void Session<Demux>::DoUDPRequest() {
  BOOST_LOG_TRIVIAL(error) << "SOCKS session: UDP Not implemented yet";
  HandleStop();
}

template <typename Demux>
void Session<Demux>::HandleApplicationServerConnect(
    const boost::system::error_code& err) {
  auto self = self_shared_from_this();
  auto p_reply = std::make_shared<Reply>(err);

  switch (request_.addressType()) {
    case Request::kIPv4:
      p_reply->set_ipv4(request_.ipv4());
      break;
    case Request::kDNS:
      p_reply->set_domain(request_.domain());
      break;
    case Request::kIPv6:
      p_reply->set_ipv6(request_.ipv6());
      break;
    default:
      HandleStop();
      break;
  }

  p_reply->set_port(request_.port());

  if (err) {  // Error connecting to the server, informing client and stopping
    AsyncSendReply(client_, *p_reply,
                   [this, self, p_reply](boost::system::error_code,
                                         std::size_t) { HandleStop(); });
  } else {  // We successfully connect to application server
    AsyncSendReply(
        client_, *p_reply,
        [this, self, p_reply](boost::system::error_code ec, std::size_t) {
          if (ec) {
            HandleStop();
            return;
          }  // reply successfully sent, Establishing link
          EstablishLink();
        });
  }
}

template <typename Demux>
void Session<Demux>::EstablishLink() {
  auto self = self_shared_from_this();

  upstream_.reset(new StreamBuff);
  downstream_.reset(new StreamBuff);

  AsyncEstablishHDLink(ssf::ReadFrom(client_), ssf::WriteTo(app_server_),
                       boost::asio::buffer(*upstream_),
                       boost::bind(&Session::HandleStop, self));

  AsyncEstablishHDLink(ssf::ReadFrom(app_server_), ssf::WriteTo(client_),
                       boost::asio::buffer(*downstream_),
                       boost::bind(&Session::HandleStop, self));
}
}  // v5
}  // socks
}  // ssf

#endif  // SSF_SERVICES_SOCKS_V5_SESSION_IPP_
