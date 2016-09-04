#include <boost/property_tree/ptree.hpp>

#include <ssf/log/log.h>

#include "common/config/proxy.h"
namespace ssf {
namespace config {

Proxy::Proxy()
    : host_(""),
      port_(""),
      username_(""),
      domain_(""),
      password_(""),
      reuse_ntlm_(true),
      reuse_kerb_(true) {}

void Proxy::Update(const PTree& proxy_prop) {
  auto host_optional = proxy_prop.get_child_optional("host");
  if (host_optional) {
    host_ = host_optional.get().data();
  }

  auto port_optional = proxy_prop.get_child_optional("port");
  if (port_optional) {
    port_ = port_optional.get().data();
  }

  auto cred_username_optional =
      proxy_prop.get_child_optional("credentials.username");
  if (cred_username_optional) {
    username_ = cred_username_optional.get().data();
  }

  auto cred_domain_optional =
      proxy_prop.get_child_optional("credentials.domain");
  if (cred_domain_optional) {
    domain_ = cred_domain_optional.get().data();
  }

  auto cred_password_optional =
      proxy_prop.get_child_optional("credentials.password");
  if (cred_password_optional) {
    password_ = cred_password_optional.get().data();
  }

  auto cred_reuse_ntlm_optional =
      proxy_prop.get_child_optional("credentials.reuse_ntlm");
  if (cred_reuse_ntlm_optional) {
    reuse_ntlm_ = cred_reuse_ntlm_optional.get().data() == "true";
  }

  auto cred_reuse_kerb_optional =
      proxy_prop.get_child_optional("credentials.reuse_kerb");
  if (cred_reuse_kerb_optional) {
    reuse_kerb_ = cred_reuse_kerb_optional.get().data() == "true";
  }
}

void Proxy::Log() const {
  if (IsSet()) {
    SSF_LOG(kLogInfo) << "config[HTTP proxy]: <" << host_ << ":" << port_
                      << ">";
    if (!username_.empty()) {
      SSF_LOG(kLogInfo) << "config[HTTP proxy]: username: <" << username_
                        << ">";
    }
    SSF_LOG(kLogInfo) << "config[HTTP proxy]: reuse NTLM credentials <"
                      << (reuse_ntlm_ ? "true" : "false") << ">";
    SSF_LOG(kLogInfo) << "config[HTTP proxy]: reuse Kerberos credentials <"
                      << (reuse_kerb_ ? "true" : "false") << ">";
  } else {
    SSF_LOG(kLogInfo) << "config[HTTP proxy]: <None>";
  }
}

}  // config
}  // ssf