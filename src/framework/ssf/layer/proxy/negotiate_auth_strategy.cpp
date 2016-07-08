#include "ssf/layer/proxy/base64.h"
#include "ssf/layer/proxy/negotiate_auth_strategy.h"

#if defined(WIN32)
#include "ssf/layer/proxy/windows/negotiate_auth_windows_impl.h"
#else

#endif

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

NegotiateAuthStrategy::NegotiateAuthStrategy(const Proxy& proxy_ctx)
    : AuthStrategy(proxy_ctx, Status::kAuthenticating) {
#if defined(WIN32)
  p_impl_.reset(new NegotiateAuthWindowsImpl(proxy_ctx));
#else
#endif
  p_impl_->Init();
}

bool NegotiateAuthStrategy::Support(const HttpResponse& response) const {
  return (response.HeaderValueBeginWith("Proxy-Authenticate", "Negotiate") ||
          response.HeaderValueBeginWith("WWW-Authenticate", "Negotiate"));
}

void NegotiateAuthStrategy::ProcessResponse(const HttpResponse& response) {
  if (response.Success()) {
    status_ = Status::kAuthenticated;
    return;
  }

  if (!Support(response)) {
    status_ = Status::kAuthenticationFailure;
    return;
  }

  set_proxy_authentication(response);

  auto server_token = ExtractNegotiateToken(response);

  if (!p_impl_->ProcessServerToken(server_token)) {
    status_ = Status::kAuthenticationFailure;
    return;
  }
}

void NegotiateAuthStrategy::PopulateRequest(HttpRequest* p_request) {
  if (p_impl_.get() == nullptr) {
    status_ = Status::kAuthenticationFailure;
    return;
  }

  auto auth_token = p_impl_->GetAuthToken();
  if (auth_token.empty()) {
    return;
  }

  p_request->AddHeader(
      proxy_authentication() ? "Proxy-Authorization" : "Authorization",
      Base64::Encode(auth_token));
}

std::vector<uint8_t> NegotiateAuthStrategy::ExtractNegotiateToken(
    const HttpResponse& response) {
  std::string challenge_str, negotiate_str("Negotiate ");
  auto proxy_authenticate_hdr = response.Header(
      proxy_authentication() ? "Proxy-Authenticate" : "WWW-Authenticate");
  for (auto& hdr : proxy_authenticate_hdr) {
    // find digest auth header
    if (hdr.find(negotiate_str) == 0) {
      challenge_str = hdr.substr(negotiate_str.length());
      break;
    }
  }

  if (challenge_str.empty()) {
    return {};
  }

  return Base64::Decode(challenge_str);
}

}  // detail
}  // proxy
}  // layer
}  // ssf