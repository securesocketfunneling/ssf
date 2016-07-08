#include "ssf/layer/proxy/base64.h"
#include "ssf/layer/proxy/ntlm_auth_strategy.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

NtlmAuthStrategy::NtlmAuthStrategy(const Proxy& proxy_ctx)
    : AuthStrategy(proxy_ctx, Status::kAuthenticating) {}

bool NtlmAuthStrategy::Support(const HttpResponse& response) const {
  return false;
}

void NtlmAuthStrategy::ProcessResponse(const HttpResponse& response) {
  if (response.Success()) {
    status_ = Status::kAuthenticated;
    return;
  }

  if (!Support(response)) {
    status_ = Status::kAuthenticationFailure;
    return;
  }
}

void NtlmAuthStrategy::PopulateRequest(HttpRequest* p_request) {}

}  // detail
}  // proxy
}  // layer
}  // ssf