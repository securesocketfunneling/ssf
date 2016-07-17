#ifndef SSF_LAYER_PROXY_AUTH_STRATEGY_H_
#define SSF_LAYER_PROXY_AUTH_STRATEGY_H_

#include <string>

#include "ssf/layer/proxy/http_request.h"
#include "ssf/layer/proxy/http_response.h"
#include "ssf/layer/proxy/proxy_endpoint_context.h"

namespace ssf {
namespace layer {
namespace proxy {

class AuthStrategy {
 public:
  enum Status : int {
    kAuthenticationFailure = -1,
    kAuthenticating = 0,
    kAuthenticated = 1
  };

 public:
  virtual ~AuthStrategy() {}

  virtual std::string AuthName() const = 0;

  virtual bool Support(const HttpResponse& response) const = 0;

  virtual void ProcessResponse(const HttpResponse& response) = 0;

  virtual void PopulateRequest(HttpRequest* p_request) = 0;

  inline Status status() const { return status_; }

 protected:
  AuthStrategy(const Proxy& proxy_ctx, Status status)
      : proxy_ctx_(proxy_ctx), status_(status), proxy_authentication_(false) {}

  inline bool proxy_authentication() const { return proxy_authentication_; }
  inline void set_proxy_authentication(const HttpResponse& response) {
    proxy_authentication_ = !response.Header("Proxy-Authenticate").empty();
  }

  std::string ExtractAuthToken(const HttpResponse& response) const;

 protected:
  Proxy proxy_ctx_;
  Status status_;

 private:
  bool proxy_authentication_;
};

}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_AUTH_STRATEGY__H