#ifndef SSF_SERVICES_SOCKS_V4_SESSION_IPP_
#define SSF_SERVICES_SOCKS_V4_SESSION_IPP_

#include <boost/bind.hpp>
#include <boost/bind/protect.hpp>
#include <boost/asio/spawn.hpp>

#include <ssf/log/log.h>

#include <ssf/utils/enum.h>

#include <boost/asio/basic_stream_socket.hpp>

namespace ssf {
namespace socks {
namespace v4 {

template <typename Demux>
Session<Demux>::Session(SessionManager* p_session_manager, fiber client)
    : ssf::BaseSession(),
      io_service_(client.get_io_service()),
      p_session_manager_(p_session_manager),
      client_(std::move(client)),
      server_(io_service_),
      server_resolver_(io_service_) {}

template <typename Demux>
void Session<Demux>::HandleStop() {
  boost::system::error_code ec;
  p_session_manager_->stop(shared_from_this(), ec);
}

template <typename Demux>
void Session<Demux>::stop(boost::system::error_code&) {
  client_.close();
  boost::system::error_code ec;
  server_.close(ec);
  if (ec) {
    SSF_LOG(kLogError) << "session[socks v4]: stop error " << ec.message();
  }
}

template <typename Demux>
void Session<Demux>::start(boost::system::error_code&) {
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

  // Dispatch request according to its command
  switch (request_.command()) {
    case static_cast<uint8_t>(Request::Command::kConnect):
      DoConnectRequest();
      break;
    case static_cast<uint8_t>(Request::Command::kBind):
      DoBindRequest();
      break;
    default:
      SSF_LOG(kLogError) << "session[socks v4]: invalid v4 command";
      break;
  }
}

template <typename Demux>
void Session<Demux>::DoConnectRequest() {
  auto connect_handler =
      boost::bind(&Session::HandleApplicationServerConnect,
                  self_shared_from_this(), boost::asio::placeholders::error);

  if (request_.Is4aVersion()) {
    // socks4a: address needs to be resolved
    auto resolve_handler = boost::bind(
        &Session::HandleResolveServerEndpoint, self_shared_from_this(),
        boost::asio::placeholders::error, boost::asio::placeholders::iterator);
    boost::asio::ip::tcp::resolver::query query(
        request_.domain(), std::to_string(request_.port()));
    server_resolver_.async_resolve(query, resolve_handler);
  } else {
    server_.async_connect(request_.Endpoint(), connect_handler);
  }
}

template <typename Demux>
void Session<Demux>::DoBindRequest() {
  SSF_LOG(kLogError) << "session[socks v4]: Bind not implemented yet";
  HandleStop();
}

template <typename Demux>
void Session<Demux>::HandleResolveServerEndpoint(
    const boost::system::error_code& ec, tcp_resolver::iterator ep_it) {
  auto connect_handler =
      boost::bind(&Session::HandleApplicationServerConnect,
                  self_shared_from_this(), boost::asio::placeholders::error);
  if (ec) {
    connect_handler(ec);
    return;
  }

  server_.async_connect(*ep_it, connect_handler);
}

template <typename Demux>
void Session<Demux>::HandleApplicationServerConnect(
    const boost::system::error_code& err) {
  auto self = self_shared_from_this();
  auto p_reply = std::make_shared<Reply>(err, boost::asio::ip::tcp::endpoint());

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

  // Two half duplex links
  AsyncEstablishHDLink(ssf::ReadFrom(client_), ssf::WriteTo(server_),
                       boost::asio::buffer(*upstream_),
                       boost::bind(&Session::HandleStop, self));

  AsyncEstablishHDLink(ssf::ReadFrom(server_), ssf::WriteTo(client_),
                       boost::asio::buffer(*downstream_),
                       boost::bind(&Session::HandleStop, self));
}

}  // v4
}  // socks
}  // ssf

#endif  // SSF_SERVICES_SOCKS_V4_SESSION_IPP_
