#include "ssf/layer/proxy/proxy_endpoint_context.h"

namespace ssf {
namespace layer {
namespace proxy {

Proxy::Proxy() : addr(""), port("") {}

ProxyEndpointContext::ProxyEndpointContext()
    : proxy_enabled(false), http_proxy() {}

bool ProxyEndpointContext::IsProxyEnabled() const { return proxy_enabled; }

bool ProxyEndpointContext::HttpProxyEnabled() const {
  return proxy_enabled && http_proxy.addr != "" && http_proxy.port != "";
}

bool ProxyEndpointContext::operator==(const ProxyEndpointContext& rhs) const {
  return proxy_enabled == rhs.proxy_enabled &&
         http_proxy.addr == rhs.http_proxy.addr &&
         http_proxy.port == rhs.http_proxy.port;
}

bool ProxyEndpointContext::operator!=(const ProxyEndpointContext& rhs) const {
  return !(*this == rhs);
}

bool ProxyEndpointContext::operator<(const ProxyEndpointContext& rhs) const {
  return http_proxy.addr < rhs.http_proxy.addr;
}

}  // proxy
}  // layer
}  // ssf