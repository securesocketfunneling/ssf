#ifndef SSF_LAYER_PROXY_NEGOTIATE_AUTH_STRATEGY_H_
#define SSF_LAYER_PROXY_NEGOTIATE_AUTH_STRATEGY_H_

#include <memory>
#include <vector>

#include "ssf/layer/proxy/auth_strategy.h"
#include "ssf/layer/proxy/platform_auth_impl.h"

namespace ssf {
namespace layer {
namespace proxy {

class NegotiateAuthStrategy : public AuthStrategy {
 public:
  NegotiateAuthStrategy(const HttpProxy& proxy_ctx);

  virtual ~NegotiateAuthStrategy(){};

  std::string AuthName() const override;

  bool Support(const HttpResponse& response) const override;

  void ProcessResponse(const HttpResponse& response) override;

  void PopulateRequest(HttpRequest* p_request) override;

 private:
  std::unique_ptr<PlatformAuthImpl> p_impl_;
};

}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_NEGOTIATE_AUTH_STRATEGY_H_