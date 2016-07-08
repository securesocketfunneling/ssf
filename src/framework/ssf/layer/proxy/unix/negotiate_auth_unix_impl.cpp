#include "ssf/layer/proxy/unix/negotiate_auth_unix_impl.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

NegotiateAuthUnixImpl::NegotiateAuthUnixImpl(const Proxy& proxy_ctx)
    : NegotiateAuthImpl(proxy_ctx) {}

NegotiateAuthUnixImpl::~NegotiateAuthUnixImpl() {}

bool NegotiateAuthUnixImpl::Init() { return false; }

bool NegotiateAuthUnixImpl::ProcessServerToken(const Token& server_token) {
  return false;
}

NegotiateAuthUnixImpl::Token NegotiateAuthUnixImpl::GetAuthToken() {
  if (state_ == kFailure) {
    return {};
  }
  return {};
}

}  // detail
}  // proxy
}  // layer
}  // ssf
