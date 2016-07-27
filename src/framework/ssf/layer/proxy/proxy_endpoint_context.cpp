#include "ssf/layer/proxy/proxy_endpoint_context.h"

namespace ssf {
namespace layer {
namespace proxy {

Proxy::Proxy() : host(""), port("") {}

boost::asio::ip::tcp::endpoint Proxy::ToTcpEndpoint(
    boost::asio::io_service& io_service) {
  boost::asio::ip::tcp::resolver resolver(io_service);
  auto endpoint_it = resolver.resolve({host, port});
  return *endpoint_it;
}

ProxyEndpointContext::ProxyEndpointContext()
    : proxy_enabled(false), http_proxy() {}

bool ProxyEndpointContext::IsProxyEnabled() const { return proxy_enabled; }

bool ProxyEndpointContext::HttpProxyEnabled() const {
  return proxy_enabled && !http_proxy.host.empty() && !http_proxy.port.empty();
}

bool ProxyEndpointContext::operator==(const ProxyEndpointContext& rhs) const {
  return proxy_enabled == rhs.proxy_enabled &&
         http_proxy.host == rhs.http_proxy.host &&
         http_proxy.port == rhs.http_proxy.port;
}

bool ProxyEndpointContext::operator!=(const ProxyEndpointContext& rhs) const {
  return !(*this == rhs);
}

bool ProxyEndpointContext::operator<(const ProxyEndpointContext& rhs) const {
  return http_proxy.host < rhs.http_proxy.host;
}

}  // proxy
}  // layer
}  // ssf