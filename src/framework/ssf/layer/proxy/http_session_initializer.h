#ifndef SSF_LAYER_PROXY_HTTP_SESSION_INITIALIZER_H_
#define SSF_LAYER_PROXY_HTTP_SESSION_INITIALIZER_H_

#include <list>
#include <memory>
#include <string>

#include <boost/system/error_code.hpp>

#include "ssf/layer/proxy/auth_strategy.h"
#include "ssf/layer/proxy/http_response.h"
#include "ssf/layer/proxy/proxy_endpoint_context.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

class HttpSessionInitializer {
 private:
  using AuthList = std::list<std::unique_ptr<AuthStrategy>>;

 public:
  enum Status : int { kError = -1, kSuccess = 0, kContinue = 1 };
  enum Stage : int { kConnect = 1, kProcessing = 2 };

 public:
  HttpSessionInitializer();

  void Reset(const std::string& target_host, const std::string& target_port,
             const ProxyEndpointContext& proxy_ep_ctx);

  inline Status status() { return status_; }

  inline Stage stage() { return stage_; }

  void PopulateRequest(HttpRequest* p_request, boost::system::error_code& ec);

  void ProcessResponse(const HttpResponse& response,
                       boost::system::error_code& ec);

 private:
  void SetAuthStrategy(const HttpResponse& response);

 private:
  Status status_;
  Stage stage_;
  std::string target_host_;
  std::string target_port_;
  ProxyEndpointContext proxy_ep_ctx_;
  AuthList auth_strategies_;
  AuthStrategy* p_current_auth_strategy_;
};

}  // detail
}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_HTTP_SESSION_INITIALIZER_H_