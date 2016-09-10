#include "ssf/log/log.h"
#include "ssf/utils/map_helpers.h"

#include "ssf/layer/proxy/proxy_endpoint_context.h"
#include "ssf/layer/proxy/proxy_helpers.h"

namespace ssf {
namespace layer {
namespace proxy {

Proxy::Proxy() : host(""), port("") {}

boost::asio::ip::tcp::endpoint Proxy::ToTcpEndpoint(
    boost::asio::io_service& io_service) const {
  boost::system::error_code ec;
  boost::asio::ip::tcp::resolver resolver(io_service);
  auto endpoint_it = resolver.resolve({host, port}, ec);
  if (ec.value() != 0) {
    return boost::asio::ip::tcp::endpoint();
  }

  return *endpoint_it;
}

ProxyEndpointContext::ProxyEndpointContext()
    : proxy_enabled_(false),
      acceptor_endpoint_(false),
      http_proxy_(),
      remote_host_() {}

void ProxyEndpointContext::Init(const LayerParameters& proxy_parameters) {
  proxy_enabled_ = false;
  auto http_host =
      ssf::helpers::GetField<std::string>("http_host", proxy_parameters);
  auto http_port =
      ssf::helpers::GetField<std::string>("http_port", proxy_parameters);
  if (http_port.empty() || http_host.empty()) {
    return;
  }

  proxy_enabled_ = true;

  acceptor_endpoint_ = (ssf::helpers::GetField<std::string>(
                          "acceptor_endpoint", proxy_parameters) == "true");

  http_proxy_.host = http_host;
  http_proxy_.port = http_port;
  http_proxy_.username =
      ssf::helpers::GetField<std::string>("http_username", proxy_parameters);
  http_proxy_.domain =
      ssf::helpers::GetField<std::string>("http_domain", proxy_parameters);
  http_proxy_.password =
      ssf::helpers::GetField<std::string>("http_password", proxy_parameters);
  http_proxy_.reuse_ntlm = (ssf::helpers::GetField<std::string>(
                                "http_reuse_ntlm", proxy_parameters) == "true");
  http_proxy_.reuse_kerb = (ssf::helpers::GetField<std::string>(
                                "http_reuse_kerb", proxy_parameters) == "true");
}

bool ProxyEndpointContext::HttpProxyEnabled() const {
  return proxy_enabled_ && !http_proxy_.host.empty() &&
         !http_proxy_.port.empty();
}

bool ProxyEndpointContext::UpdateRemoteHost(const LayerParameters& parameters) {
  if (proxy_enabled_) {
    remote_host_ = ProxyEndpointContext::Host(parameters);
    return true;
  }
  return false;
}

bool ProxyEndpointContext::operator==(const ProxyEndpointContext& rhs) const {
  return proxy_enabled_ == rhs.proxy_enabled_ &&
         http_proxy_.host == rhs.http_proxy_.host &&
         http_proxy_.port == rhs.http_proxy_.port;
}

bool ProxyEndpointContext::operator!=(const ProxyEndpointContext& rhs) const {
  return !(*this == rhs);
}

bool ProxyEndpointContext::operator<(const ProxyEndpointContext& rhs) const {
  return http_proxy_.host < rhs.http_proxy_.host;
}

}  // proxy
}  // layer
}  // ssf