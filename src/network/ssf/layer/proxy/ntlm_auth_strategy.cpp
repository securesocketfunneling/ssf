#include "ssf/layer/proxy/base64.h"
#include "ssf/layer/proxy/ntlm_auth_strategy.h"
#include "ssf/log/log.h"

#if defined(WIN32)
#include "ssf/layer/proxy/windows/sspi_auth_impl.h"
#endif

namespace ssf {
namespace layer {
namespace proxy {

NtlmAuthStrategy::NtlmAuthStrategy(const HttpProxy& proxy_ctx)
    : AuthStrategy(proxy_ctx, Status::kAuthenticating), p_impl_(nullptr) {
#if defined(WIN32)
  p_impl_.reset(new SSPIAuthImpl(SSPIAuthImpl::kNTLM, proxy_ctx));
#endif

  if (p_impl_.get() != nullptr) {
    if (!p_impl_->Init()) {
      SSF_LOG(kLogDebug) << "network[proxy]: ntlm: could not initialize "
                         << "platform impl";
      status_ = Status::kAuthenticationFailure;
    }
  } else {
    SSF_LOG(kLogDebug) << "network[proxy]: ntlm: no platform impl found";
    status_ = Status::kAuthenticationFailure;
  }
}

std::string NtlmAuthStrategy::AuthName() const { return "NTLM"; }

bool NtlmAuthStrategy::Support(const HttpResponse& response) const {
  return p_impl_.get() != nullptr &&
         status_ != Status::kAuthenticationFailure &&
         response.IsAuthenticationAllowed(AuthName());
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

  set_proxy_authentication(response);

  auto server_token = Base64::Decode(ExtractAuthToken(response));

  if (!p_impl_->ProcessServerToken(server_token)) {
    SSF_LOG(kLogDebug)
        << "network[proxy]: ntlm: could not process server token";
    status_ = Status::kAuthenticationFailure;
    return;
  }
}

void NtlmAuthStrategy::PopulateRequest(HttpRequest* p_request) {
  if (p_impl_.get() == nullptr) {
    status_ = Status::kAuthenticationFailure;
    return;
  }

  auto auth_token = p_impl_->GetAuthToken();
  if (auth_token.empty()) {
    SSF_LOG(kLogDebug) << "network[proxy]: ntlm: response token empty";
    status_ = Status::kAuthenticationFailure;
    return;
  }

  std::string ntlm_value = AuthName() + " " + Base64::Encode(auth_token);
  p_request->AddHeader(
      proxy_authentication() ? "Proxy-Authorization" : "Authorization",
      ntlm_value);
}

}  // proxy
}  // layer
}  // ssf