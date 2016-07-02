#ifndef SSF_LAYER_PROXY_BASIC_AUTH_STRATEGY_H_
#define SSF_LAYER_PROXY_BASIC_AUTH_STRATEGY_H_

#include "ssf/layer/proxy/auth_strategy.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

class BasicAuthStrategy : public AuthStrategy {
 public:
  BasicAuthStrategy();

  virtual ~BasicAuthStrategy(){};

  bool Support(const HttpResponse& response) const override;

  void ProcessResponse(const HttpResponse& response) override;

  void PopulateRequest(const ProxyEndpointContext& proxy_ep_ctx,
                       HttpRequest* p_request) override;

 private:
  static std::string Base64Encode(const std::string& input);

 private:
  bool request_populated_;
};

}  // detail
}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_BASIC_AUTH_STRATEGY_H_