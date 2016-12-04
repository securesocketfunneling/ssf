#ifndef SSF_LAYER_PROXY_SOCKS_SESSION_INITIALIZER_H_
#define SSF_LAYER_PROXY_SOCKS_SESSION_INITIALIZER_H_

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include <boost/system/error_code.hpp>

#include "ssf/layer/proxy/proxy_endpoint_context.h"
#include "ssf/layer/proxy/socks4_strategy.h"
#include "ssf/layer/proxy/socks5_strategy.h"

namespace ssf {
namespace layer {
namespace proxy {

class SocksSessionInitializer {
 public:
  using Buffer = std::vector<uint8_t>;
  enum class Status : int { kError = -1, kSuccess = 0, kContinue = 1 };

 public:
  SocksSessionInitializer();

  void Reset(const std::string& target_host, const std::string& target_port,
             const ProxyEndpointContext& proxy_ep_ctx,
             boost::system::error_code& ec);

  inline Status status() { return status_; }

  void PopulateRequest(Buffer* p_request, uint32_t* p_expected_response_size,
                       boost::system::error_code& ec);

  void ProcessResponse(const Buffer& response, boost::system::error_code& ec);

 private:
  Status status_;
  std::string target_host_;
  uint16_t target_port_;
  ProxyEndpointContext proxy_ep_ctx_;

  Socks4Strategy socks4_strategy_;
  Socks5Strategy socks5_strategy_;
};

}  // ssf
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_SOCKS_SESSION_INITIALIZER_H_