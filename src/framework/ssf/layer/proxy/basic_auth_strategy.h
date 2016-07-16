#ifndef SSF_LAYER_PROXY_BASIC_AUTH_STRATEGY_H_
#define SSF_LAYER_PROXY_BASIC_AUTH_STRATEGY_H_

#include "ssf/layer/proxy/auth_strategy.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

class BasicAuthStrategy : public AuthStrategy {
 public:
  BasicAuthStrategy(const Proxy& proxy);

  virtual ~BasicAuthStrategy(){};

  std::string AuthName() const override;

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

#endif  // SSF_LAYER_PROXY_BASIC_AUTH_STRATEGY_H_