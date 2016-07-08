#ifndef SSF_LAYER_PROXY_NEGOTIATE_AUTH_IMPL_H_
#define SSF_LAYER_PROXY_NEGOTIATE_AUTH_IMPL_H_

#include <cstdint>

#include <vector>

#include "ssf/layer/proxy/proxy_endpoint_context.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

class NegotiateAuthImpl {
 public:
  enum State { kFailure, kInit, kContinue, kSuccess };
  using Token = std::vector<uint8_t>;

 public:
  virtual ~NegotiateAuthImpl() {}

  virtual bool Init() = 0;
  virtual bool ProcessServerToken(const Token& server_token) = 0;
  virtual Token GetAuthToken() = 0;

 protected:
  NegotiateAuthImpl(const Proxy& proxy_ctx)
      : state_(kInit), proxy_ctx_(proxy_ctx) {}

 protected:
  State state_;
  Proxy proxy_ctx_;
};

}  // detail
}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_NEGOTIATE_AUTH_IMPL_H_