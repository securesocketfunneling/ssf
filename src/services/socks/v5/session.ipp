#ifndef SSF_SERVICES_SOCKS_V5_SESSION_IPP_
#define SSF_SERVICES_SOCKS_V5_SESSION_IPP_

#include <functional>

#include <boost/asio/basic_stream_socket.hpp>

#include <ssf/utils/enum.h>

namespace ssf {
namespace services {
namespace socks {
namespace v5 {

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
    SSF_LOG(kLogError) << "session[socks v5]: stop error " << ec.message();
  }
}

template <typename Demux>
void Session<Demux>::start(boost::system::error_code&) {
  AsyncReadRequestAuth(
      client_, &request_auth_,
      std::bind(&Session::HandleRequestAuthDispatch, SelfFromThis(),
                std::placeholders::_1, std::placeholders::_2));
}

template <typename Demux>
void Session<Demux>::HandleRequestAuthDispatch(
    const boost::system::error_code& ec, std::size_t) {
  if (ec) {
    SSF_LOG(kLogError) << "session[socks v5]: request auth failed "
                       << ec.message();
    HandleStop();
    return;
  }

  // Check for compatible authentications
  if (request_auth_.IsNoAuthPresent()) {
    DoNoAuth();
  } else {
    DoErrorAuth();
  }
}

template <typename Demux>
void Session<Demux>::DoNoAuth() {
  auto self = SelfFromThis();
  auto p_reply = std::make_shared<ReplyAuth>(AuthMethod::kNoAuth);

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
  auto self = SelfFromThis();
  auto p_reply = std::make_shared<ReplyAuth>(AuthMethod::kUnsupportedAuth);

  AsyncSendAuthReply(client_, *p_reply,
                     [this, self, p_reply](boost::system::error_code,
                                           std::size_t) { HandleStop(); });
}

template <typename Demux>
void Session<Demux>::HandleReplyAuthSent() {
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

  // Check command asked
  switch (request_.command()) {
    case static_cast<uint8_t>(CommandType::kConnect):
      DoConnectRequest();
      break;
    case static_cast<uint8_t>(CommandType::kBind):
      DoBindRequest();
      break;
    case static_cast<uint8_t>(CommandType::kUDP):
      DoUDPRequest();
      break;
    default:
      SSF_LOG(kLogError) << "session[socks v5]: invalid v5 command";
      DoErrorCommand(CommandStatus::kCommandNotSupported);
      break;
  }
}

template <typename Demux>
void Session<Demux>::DoConnectRequest() {
  auto connect_handler = std::bind(&Session::HandleApplicationServerConnect,
                                   SelfFromThis(), std::placeholders::_1);

  boost::system::error_code ec;
  uint16_t port = request_.port();

  switch (request_.address_type()) {
    case static_cast<uint8_t>(AddressType::kIPv4): {
      boost::asio::ip::address_v4 address(request_.ipv4());
      server_.async_connect(boost::asio::ip::tcp::endpoint(address, port),
                            connect_handler);
      break;
    }
    case static_cast<uint8_t>(AddressType::kIPv6): {
      boost::asio::ip::address_v6 address(request_.ipv6());
      server_.async_connect(boost::asio::ip::tcp::endpoint(address, port),
                            connect_handler);
      break;
    }
    case static_cast<uint8_t>(AddressType::kDNS): {
      auto resolve_handler =
          std::bind(&Session::HandleResolveServerEndpoint, SelfFromThis(),
                    std::placeholders::_1, std::placeholders::_2);

      boost::asio::ip::tcp::resolver::query query(
          std::string(request_.domain().data(), request_.domain().size()),
          std::to_string(port));

      server_resolver_.async_resolve(query, resolve_handler);
      break;
    }
    default:
      SSF_LOG(kLogError) << "session[socks v5]: unsupported address type";
      ec.assign(ssf::error::connection_refused, ssf::error::get_ssf_category());
      connect_handler(ec);
      break;
  };
}

template <typename Demux>
void Session<Demux>::DoBindRequest() {
  SSF_LOG(kLogError) << "session[socks v5]: Bind not implemented yet";
  DoErrorCommand(CommandStatus::kCommandNotSupported);
}

template <typename Demux>
void Session<Demux>::DoUDPRequest() {
  SSF_LOG(kLogError) << "session[socks v5]: UDP not implemented yet";
  DoErrorCommand(CommandStatus::kCommandNotSupported);
}

template <typename Demux>
void Session<Demux>::DoErrorCommand(CommandStatus err_status) {
  auto self = SelfFromThis();
  auto p_reply = std::make_shared<Reply>(err_status);

  AsyncSendReply(client_, *p_reply,
                 [this, self, p_reply](boost::system::error_code, std::size_t) {
                   HandleStop();
                 });
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
  auto p_reply = std::make_shared<Reply>(
      !err ? CommandStatus::kSucceeded : CommandStatus::kConnectionRefused);

  boost::system::error_code ep_err;
  boost::asio::ip::tcp::endpoint local_ep;
  if (!err) {
    local_ep = server_.local_endpoint(ep_err);
  }

  if (!err && !ep_err) {
    auto address = local_ep.address();
    if (address.is_v4()) {
      p_reply->set_ipv4(address.to_v4().to_bytes());
    } else {
      p_reply->set_ipv6(address.to_v6().to_bytes());
    }
    p_reply->set_port(local_ep.port());
  } else {
    // copy request data into reply in case of failure
    switch (request_.address_type()) {
      case static_cast<uint8_t>(AddressType::kIPv4):
        p_reply->set_ipv4(request_.ipv4());
        break;
      case static_cast<uint8_t>(AddressType::kDNS):
        p_reply->set_domain(request_.domain());
        break;
      case static_cast<uint8_t>(AddressType::kIPv6):
        p_reply->set_ipv6(request_.ipv6());
        break;
    };
    p_reply->set_port(request_.port());
  }

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

  AsyncEstablishHDLink(ssf::ReadFrom(client_), ssf::WriteTo(server_),
                       boost::asio::buffer(*upstream_),
                       std::bind(&Session::HandleStop, self));

  AsyncEstablishHDLink(ssf::ReadFrom(server_), ssf::WriteTo(client_),
                       boost::asio::buffer(*downstream_),
                       std::bind(&Session::HandleStop, self));
}
}  // v5
}  // socks
}  // services
}  // ssf

#endif  // SSF_SERVICES_SOCKS_V5_SESSION_IPP_
