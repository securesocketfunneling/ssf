#ifndef SSF_LAYER_PROXY_PROXY_ENDPOINT_CONTEXT_H_
#define SSF_LAYER_PROXY_PROXY_ENDPOINT_CONTEXT_H_

#include <string>

namespace ssf {
namespace layer {
namespace proxy {

struct Proxy {
  Proxy();

  std::string addr;
  std::string port;
};

struct ProxyEndpointContext {
  ProxyEndpointContext();

  bool IsProxyEnabled() const;

  bool HttpProxyEnabled() const;

  bool operator==(const ProxyEndpointContext& rhs) const;

  bool operator!=(const ProxyEndpointContext& rhs) const;

  bool operator<(const ProxyEndpointContext& rhs) const;

  bool proxy_enabled;
  Proxy http_proxy;
};

}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_PROXY_ENDPOINT_CONTEXT_H_