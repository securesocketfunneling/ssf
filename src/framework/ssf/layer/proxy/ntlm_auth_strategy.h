#ifndef SSF_LAYER_PROXY_NTLM_AUTH_STRATEGY_H_
#define SSF_LAYER_PROXY_NTLM_AUTH_STRATEGY_H_

#include "ssf/layer/proxy/auth_strategy.h"
#include "ssf/layer/proxy/platform_auth_impl.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

class NtlmAuthStrategy : public AuthStrategy {
 public:
  NtlmAuthStrategy(const Proxy& proxy_ctx);

  virtual ~NtlmAuthStrategy(){};

  std::string AuthName() const override;

  bool Support(const HttpResponse& response) const override;

  void ProcessResponse(const HttpResponse& response) override;

  void PopulateRequest(HttpRequest* p_request) override;  

 private:
  std::unique_ptr<PlatformAuthImpl> p_impl_;
};

}  // detail
}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_NTLM_AUTH_STRATEGY_H_