#ifndef SSF_LAYER_PROXY_UNIX_NEGOTIATE_AUTH_UNIX_IMPL_H_
#define SSF_LAYER_PROXY_UNIX_NEGOTIATE_AUTH_UNIX_IMPL_H_

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#if defined(MAC_OS_X_VERSION_10_9) && \
    MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_9
#define GSSKRB_APPLE_DEPRECATED(x)
#endif
#endif

#if defined(__APPLE__)
#include <Kerberos/gssapi.h>
#else
#include <gssapi.h>
#endif

#include "ssf/layer/proxy/negotiate_auth_impl.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

class NegotiateAuthUnixImpl : public NegotiateAuthImpl {
 private:
  typedef decltype(&gss_init_sec_context) fct_gss_init_sec_context_t;
  typedef decltype(&gss_import_name) fct_gss_import_name_t;
  typedef decltype(&gss_release_buffer) fct_gss_release_buffer_t;
  typedef decltype(&gss_delete_sec_context) fct_gss_delete_sec_context_t;
  typedef decltype(&gss_release_name) fct_gss_release_name_t;

 public:
  NegotiateAuthUnixImpl(const Proxy& proxy_ctx);

  virtual ~NegotiateAuthUnixImpl();

  bool Init() override;
  bool ProcessServerToken(const Token& server_token) override;
  Token GetAuthToken() override;

 private:
  bool InitLibrary();
  void LogError(OM_uint32 major_status);

 private:
  void* h_gss_api_;
  gss_ctx_id_t h_sec_ctx_;
  gss_name_t server_name_;
  Token auth_token_;

  fct_gss_init_sec_context_t fct_gss_init_sec_context_;
  fct_gss_import_name_t fct_gss_import_name_;
  fct_gss_release_buffer_t fct_gss_release_buffer_;
  fct_gss_delete_sec_context_t fct_gss_delete_sec_context_;
  fct_gss_release_name_t fct_gss_release_name_;
};

}  // detail
}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_UNIX_NEGOTIATE_AUTH_UNIX_IMPL_H_