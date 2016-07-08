#ifndef SSF_LAYER_PROXY_UNIX_NEGOTIATE_AUTH_UNIX_IMPL_H_
#define SSF_LAYER_PROXY_UNIX_NEGOTIATE_AUTH_UNIX_IMPL_H_

#include "ssf/layer/proxy/negotiate_auth_impl.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

class NegotiateAuthUnixImpl : public NegotiateAuthImpl {
 public:
  NegotiateAuthUnixImpl(const Proxy& proxy_ctx);

  virtual ~NegotiateAuthUnixImpl();

  bool Init() override;
  bool ProcessServerToken(const Token& server_token) override;
  Token GetAuthToken() override;
};

}  // detail
}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_UNIX_NEGOTIATE_AUTH_UNIX_IMPL_H_