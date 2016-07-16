#include <boost/format.hpp>

#include "ssf/error/error.h"
#include "ssf/layer/proxy/http_session_initializer.h"
#include "ssf/layer/proxy/basic_auth_strategy.h"
#include "ssf/layer/proxy/digest_auth_strategy.h"
#include "ssf/layer/proxy/negotiate_auth_strategy.h"
#include "ssf/log/log.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

HttpSessionInitializer::HttpSessionInitializer()
    : status_(Status::kContinue),
      target_host_(""),
      target_port_(""),
      proxy_ep_ctx_(),
      auth_strategies_(),
      p_current_auth_strategy_(nullptr) {}

void HttpSessionInitializer::Reset(const std::string& target_host,
                                   const std::string& target_port,
                                   const ProxyEndpointContext& proxy_ep_ctx) {
  status_ = Status::kContinue;
  target_host_ = target_host;
  target_port_ = target_port;
  proxy_ep_ctx_ = proxy_ep_ctx;

  // instanciate auth strategies
  p_current_auth_strategy_ = nullptr;
  auth_strategies_.clear();
  auth_strategies_.emplace_back(
      new detail::NegotiateAuthStrategy(proxy_ep_ctx_.http_proxy));
  auth_strategies_.emplace_back(
      new detail::DigestAuthStrategy(proxy_ep_ctx_.http_proxy));
  auth_strategies_.emplace_back(
      new detail::BasicAuthStrategy(proxy_ep_ctx_.http_proxy));
}

void HttpSessionInitializer::PopulateRequest(HttpRequest* p_request,
                                             boost::system::error_code& ec) {
  if (status_ != Status::kContinue) {
    ec.assign(ssf::error::interrupted, ssf::error::get_ssf_category());
  }

  p_request->Reset("CONNECT", target_host_ + ':' + target_port_);

  if (p_current_auth_strategy_ != nullptr) {
    p_current_auth_strategy_->PopulateRequest(p_request);
  }
}

void HttpSessionInitializer::ProcessResponse(const HttpResponse& response,
                                             boost::system::error_code& ec) {
  if (p_current_auth_strategy_ == nullptr && response.Success()) {
    // no proxy authentication, continue
    status_ = Status::kSuccess;
    return;
  }

  if (response.AuthenticationRequired()) {
    // find auth strategy
    if (p_current_auth_strategy_ == nullptr ||
        p_current_auth_strategy_->status() ==
            AuthStrategy::kAuthenticationFailure) {
      p_current_auth_strategy_ = nullptr;
      for (auto& p_auth_strategy : auth_strategies_) {
        if (p_auth_strategy->Support(response)) {
          p_current_auth_strategy_ = p_auth_strategy.get();
          break;
        }
      }
    }

    if (p_current_auth_strategy_ == nullptr) {
      SSF_LOG(kLogError) << "network[proxy]: authentication strategies failed";
      status_ = Status::kError;
      return;
    }
  }

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
    return;
  }

  // other behaviours (e.g. redirection) not implemented
  status_ = Status::kError;
  return;
}

}  // detail
}  // proxy
}  // layer
}  // ssf