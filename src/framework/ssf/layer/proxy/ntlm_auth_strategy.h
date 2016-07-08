#ifndef SSF_LAYER_PROXY_NTLM_AUTH_STRATEGY_H_
#define SSF_LAYER_PROXY_NTLM_AUTH_STRATEGY_H_

#include "ssf/layer/proxy/auth_strategy.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

class NtlmAuthStrategy : public AuthStrategy {
 public:
  NtlmAuthStrategy(const Proxy& proxy_ctx);

  virtual ~NtlmAuthStrategy(){};

  bool Support(const HttpResponse& response) const override;

  void ProcessResponse(const HttpResponse& response) override;

  void PopulateRequest(HttpRequest* p_request) override;

 private:
  bool request_populated_;
};

}  // detail
}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_NTLM_AUTH_STRATEGY_H_