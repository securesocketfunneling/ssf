#ifndef SSF_LAYER_PROXY_PROXY_ENDPOINT_CONTEXT_H_
#define SSF_LAYER_PROXY_PROXY_ENDPOINT_CONTEXT_H_

#include <string>

#include <boost/asio/ip/tcp.hpp>

#include "ssf/layer/physical/host.h"

namespace ssf {
namespace layer {
namespace proxy {

struct Proxy {
  Proxy();

  boost::asio::ip::tcp::endpoint ToTcpEndpoint(
      boost::asio::io_service& io_service) const;

  std::string host;
  std::string port;
  std::string username;
  std::string password;
  std::string domain;
  bool reuse_ntlm;
  bool reuse_kerb;
};

class ProxyEndpointContext {
 public:
  using Host = ssf::layer::physical::Host;

 public:
  ProxyEndpointContext();

  // Init context from proxy layer params
  void Init(const LayerParameters& proxy_parameters);

  // Update remote host component with TCP layer parameters
  // Returns true if remote host was updated
  bool UpdateRemoteHost(const LayerParameters& tcp_parameters);

  bool HttpProxyEnabled() const;

  bool operator==(const ProxyEndpointContext& rhs) const;

  bool operator!=(const ProxyEndpointContext& rhs) const;

  bool operator<(const ProxyEndpointContext& rhs) const;

  inline bool proxy_enabled() const { return proxy_enabled_; }

  inline const Proxy& http_proxy() const { return http_proxy_; }

  inline const Host& remote_host() const { return remote_host_; }

 private:
  bool proxy_enabled_;
  Proxy http_proxy_;
  Host remote_host_;
};

}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_PROXY_ENDPOINT_CONTEXT_H_