#ifndef SSF_LAYER_PROXY_WINDOWS_SSPI_AUTH_IMPL_H_
#define SSF_LAYER_PROXY_WINDOWS_SSPI_AUTH_IMPL_H_

#include <array>
#include <string>

#include "ssf/layer/proxy/platform_auth_impl.h"

#define SECURITY_WIN32
#include <windows.h>
#include <sspi.h>

namespace ssf {
namespace layer {
namespace proxy {

class SSPIAuthImpl : public PlatformAuthImpl {
 private:
  using SecPackageNames = std::array<char*, 2>;

 public:
  enum SecurityPackage { kNTLM = 0, kNegotiate };

 public:
  SSPIAuthImpl(SecurityPackage sec_package, const Proxy& proxy_ctx);

  virtual ~SSPIAuthImpl();

  bool Init() override;
  bool ProcessServerToken(const Token& server_token) override;
  Token GetAuthToken() override;

 private:
  static std::string GenerateSecurityPackageName(SecurityPackage sec_package);
  bool IsSecurityContextSet();
  std::string GenerateServiceName(SecurityPackage sec_package);
  void Clear();

 private:
  static SecPackageNames sec_package_names_;
  SecurityPackage sec_package_;
  CredHandle h_cred_;
  CtxtHandle h_sec_ctx_;
  Token output_token_;
  std::size_t output_token_length_;
  std::string service_name_;
};

}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_WINDOWS_SSPI_AUTH_IMPL_H_