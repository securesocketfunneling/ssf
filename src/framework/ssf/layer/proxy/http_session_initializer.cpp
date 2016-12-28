#include <boost/format.hpp>

#include "ssf/error/error.h"
#include "ssf/layer/proxy/http_session_initializer.h"
#include "ssf/layer/proxy/basic_auth_strategy.h"
#include "ssf/layer/proxy/digest_auth_strategy.h"
#include "ssf/layer/proxy/ntlm_auth_strategy.h"
#include "ssf/layer/proxy/negotiate_auth_strategy.h"
#include "ssf/log/log.h"

namespace ssf {
namespace layer {
namespace proxy {

HttpSessionInitializer::HttpSessionInitializer()
    : status_(Status::kContinue),
      stage_(Stage::kConnect),
      target_host_(""),
      target_port_(""),
      proxy_ep_ctx_(),
      auth_strategies_(),
      p_current_auth_strategy_(nullptr) {}

void HttpSessionInitializer::Reset(const std::string& target_host,
                                   const std::string& target_port,
                                   const ProxyEndpointContext& proxy_ep_ctx) {
  status_ = Status::kContinue;
  stage_ = Stage::kConnect;
  target_host_ = target_host;
  target_port_ = target_port;
  proxy_ep_ctx_ = proxy_ep_ctx;

  // instantiate auth strategies
  p_current_auth_strategy_ = nullptr;
  auth_strategies_.clear();
  auth_strategies_.emplace_back(
      new NegotiateAuthStrategy(proxy_ep_ctx_.http_proxy()));
  auth_strategies_.emplace_back(
      new NtlmAuthStrategy(proxy_ep_ctx_.http_proxy()));
  auth_strategies_.emplace_back(
      new DigestAuthStrategy(proxy_ep_ctx_.http_proxy()));
  auth_strategies_.emplace_back(
      new BasicAuthStrategy(proxy_ep_ctx_.http_proxy()));
}

void HttpSessionInitializer::PopulateRequest(HttpRequest* p_request,
                                             boost::system::error_code& ec) {
  if (status_ != Status::kContinue) {
    ec.assign(ssf::error::interrupted, ssf::error::get_ssf_category());
  }

  p_request->Reset("CONNECT", target_host_ + ':' + target_port_);

  if (stage_ == kProcessing) {
    if (p_current_auth_strategy_ != nullptr) {
      p_current_auth_strategy_->PopulateRequest(p_request);
    }
  }
}

void HttpSessionInitializer::ProcessResponse(const HttpResponse& response,
                                             boost::system::error_code& ec) {
  if (response.Success()) {
    SSF_LOG(kLogInfo) << "network[http proxy]: connected (auth: "
                      << ((p_current_auth_strategy_ != nullptr)
                              ? (p_current_auth_strategy_->AuthName())
                              : "None") << ")";
    status_ = Status::kSuccess;
    return;
  }

  if (!response.AuthenticationRequired()) {
    // other behaviours (e.g. redirection) not implemented
    status_ = Status::kError;
    return;
  }

  // find auth strategy
  bool reconnect = false;
  if (p_current_auth_strategy_ == nullptr ||
      p_current_auth_strategy_->status() ==
          AuthStrategy::kAuthenticationFailure) {
    p_current_auth_strategy_ = nullptr;
    if (stage_ == kProcessing) {
      stage_ = kConnect;
      reconnect = true;
    } else {
      SetAuthStrategy(response);
    }
  }

  if (reconnect) {
    return;
  }

  if (p_current_auth_strategy_ == nullptr) {
    SSF_LOG(kLogError)
        << "network[http proxy]: authentication strategies failed";
    status_ = Status::kError;
    return;
  }

  stage_ = Stage::kProcessing;

  if (p_current_auth_strategy_ != nullptr) {
    p_current_auth_strategy_->ProcessResponse(response);
    switch (p_current_auth_strategy_->status()) {
      case AuthStrategy::Status::kAuthenticating:
      case AuthStrategy::Status::kAuthenticationFailure:
        break;
      case AuthStrategy::Status::kAuthenticated:
        status_ = Status::kSuccess;
        break;
      default:
        status_ = Status::kError;
        break;
    }
  }
}

void HttpSessionInitializer::SetAuthStrategy(const HttpResponse& response) {
  for (const auto& p_auth_strategy : auth_strategies_) {
    if (p_auth_strategy->Support(response)) {
      p_current_auth_strategy_ = p_auth_strategy.get();
      SSF_LOG(kLogDebug) << "network[http proxy]: try "
                         << p_current_auth_strategy_->AuthName() << " auth";
      break;
    }
  }
}

}  // proxy
}  // layer
}  // ssf