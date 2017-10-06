#ifndef SSF_SERVICES_SOCKS_V4_SESSION_IPP_
#define SSF_SERVICES_SOCKS_V4_SESSION_IPP_

#include <functional>

#include <ssf/log/log.h>

#include <ssf/utils/enum.h>

#include <boost/asio/basic_stream_socket.hpp>

namespace ssf {
namespace services {
namespace socks {
namespace v4 {

template <typename Demux>
Session<Demux>::Session(SocksServerWPtr socks_server, Fiber client)
    : ssf::BaseSession(),
      io_service_(client.get_io_service()),
      socks_server_(socks_server),
      client_(std::move(client)),
      server_(io_service_),
      server_resolver_(io_service_) {}

template <typename Demux>
void Session<Demux>::HandleStop() {
  boost::system::error_code stop_err;
  if (auto p_socks_server = socks_server_.lock()) {
    p_socks_server->StopSession(this->SelfFromThis(), stop_err);
  }
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
  AsyncReadRequest(client_, &request_,
                   std::bind(&Session::HandleRequestDispatch, SelfFromThis(),
                             std::placeholders::_1, std::placeholders::_2));
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
  auto connect_handler = std::bind(&Session::HandleApplicationServerConnect,
                                   SelfFromThis(), std::placeholders::_1);

  if (request_.Is4aVersion()) {
    // socks4a: address needs to be resolved
    auto resolve_handler =
        std::bind(&Session::HandleResolveServerEndpoint, SelfFromThis(),
                  std::placeholders::_1, std::placeholders::_2);
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
    const boost::system::error_code& ec, Tcp::resolver::iterator ep_it) {
  auto connect_handler = std::bind(&Session::HandleApplicationServerConnect,
                                   SelfFromThis(), std::placeholders::_1);
  if (ec) {
    connect_handler(ec);
    return;
  }

  server_.async_connect(*ep_it, connect_handler);
}

template <typename Demux>
void Session<Demux>::HandleApplicationServerConnect(
    const boost::system::error_code& err) {
  auto self = SelfFromThis();
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
  auto self = SelfFromThis();

  upstream_.reset(new StreamBuf());
  downstream_.reset(new StreamBuf());

  // Two half duplex links
  AsyncEstablishHDLink(ssf::ReadFrom(client_), ssf::WriteTo(server_),
                       boost::asio::buffer(*upstream_),
                       std::bind(&Session::HandleStop, self));

  AsyncEstablishHDLink(ssf::ReadFrom(server_), ssf::WriteTo(client_),
                       boost::asio::buffer(*downstream_),
                       std::bind(&Session::HandleStop, self));
}

}  // v4
}  // socks
}  // services
}  // ssf

#endif  // SSF_SERVICES_SOCKS_V4_SESSION_IPP_
