#ifndef SSF_LAYER_PROXY_PLATFORM_AUTH_IMPL_H_
#define SSF_LAYER_PROXY_PLATFORM_AUTH_IMPL_H_

#include <cstdint>

#include <vector>

#include "ssf/layer/proxy/proxy_endpoint_context.h"

namespace ssf {
namespace layer {
namespace proxy {

class PlatformAuthImpl {
 public:
  enum State { kFailure, kInit, kContinue, kSuccess };
  using Token = std::vector<uint8_t>;

 public:
  virtual ~PlatformAuthImpl() {}

  virtual bool Init() = 0;
  virtual bool ProcessServerToken(const Token& server_token) = 0;
  virtual Token GetAuthToken() = 0;

 protected:
  PlatformAuthImpl(const Proxy& proxy_ctx)
      : state_(kInit), proxy_ctx_(proxy_ctx) {}

 protected:
  State state_;
  Proxy proxy_ctx_;
};

}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_PLATFORM_AUTH_IMPL_H_