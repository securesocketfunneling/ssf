#ifndef SSF_LAYER_PROXY_WINDOWS_NEGOTIATE_AUTH_WINDOWS_IMPL_H_
#define SSF_LAYER_PROXY_WINDOWS_NEGOTIATE_AUTH_WINDOWS_IMPL_H_

#include "ssf/layer/proxy/negotiate_auth_impl.h"

#define SECURITY_WIN32
#include <windows.h>
#include <sspi.h>

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

class NegotiateAuthWindowsImpl : public NegotiateAuthImpl {
 public:
  NegotiateAuthWindowsImpl(const Proxy& proxy_ctx);

  virtual ~NegotiateAuthWindowsImpl();

  bool Init() override;
  bool ProcessServerToken(const Token& server_token) override;
  Token GetAuthToken() override;

 private:
  CredHandle h_cred_;
  CtxtHandle h_sec_ctx_;
  Token output_token_;
  std::size_t output_token_length_;
  std::string service_name_;
};

}  // detail
}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_WINDOWS_NEGOTIATE_AUTH_WINDOWS_IMPL_H_