#include <boost/property_tree/ptree.hpp>

#include <boost/algorithm/string.hpp>

#include <ssf/log/log.h>
#include <ssf/utils/enum.h>

#include "common/config/proxy.h"

namespace ssf {
namespace config {

HttpProxy::HttpProxy()
    : host_(""),
      port_(""),
      username_(""),
      domain_(""),
      password_(""),
      reuse_ntlm_(true),
      reuse_kerb_(true) {}

void HttpProxy::Update(const PTree& proxy_prop) {
  auto host_optional = proxy_prop.get_child_optional("host");
  if (host_optional) {
    host_ = host_optional.get().data();
    boost::trim(host_);
  }

  auto port_optional = proxy_prop.get_child_optional("port");
  if (port_optional) {
    port_ = port_optional.get().data();
    boost::trim(port_);
  }

  auto user_agent_optional = proxy_prop.get_child_optional("user_agent");
  if (user_agent_optional) {
    user_agent_ = user_agent_optional.get().data();
    boost::trim(user_agent_);
  }

  auto cred_username_optional =
      proxy_prop.get_child_optional("credentials.username");
  if (cred_username_optional) {
    username_ = cred_username_optional.get().data();
    boost::trim(username_);
  }

  auto cred_domain_optional =
      proxy_prop.get_child_optional("credentials.domain");
  if (cred_domain_optional) {
    domain_ = cred_domain_optional.get().data();
    boost::trim(domain_);
  }

  auto cred_password_optional =
      proxy_prop.get_child_optional("credentials.password");
  if (cred_password_optional) {
    password_ = cred_password_optional.get().data();
    boost::trim(password_);
  }

  auto cred_reuse_ntlm_optional =
      proxy_prop.get_child_optional("credentials.reuse_ntlm");
  if (cred_reuse_ntlm_optional) {
    reuse_ntlm_ = cred_reuse_ntlm_optional.get().get_value<bool>();
  }

  auto cred_reuse_kerb_optional =
      proxy_prop.get_child_optional("credentials.reuse_kerb");
  if (cred_reuse_kerb_optional) {
    reuse_kerb_ = cred_reuse_kerb_optional.get().get_value<bool>();
  }
}

void HttpProxy::Log() const {
  if (IsSet()) {
    SSF_LOG(kLogInfo) << "[config][http proxy] <" << host_ << ":" << port_
                      << ">";
    if (!username_.empty()) {
      SSF_LOG(kLogInfo) << "[config][http proxy] username: <" << username_
                        << ">";
    }
    SSF_LOG(kLogInfo) << "[config][http proxy] reuse NTLM credentials <"
                      << (reuse_ntlm_ ? "true" : "false") << ">";
    SSF_LOG(kLogInfo) << "[config][http proxy] reuse Kerberos credentials <"
                      << (reuse_kerb_ ? "true" : "false") << ">";
  } else {
    SSF_LOG(kLogInfo) << "[config][http proxy] <None>";
  }
}

SocksProxy::SocksProxy()
    : version_(Socks::Version::kVUnknown), host_(""), port_("") {}

void SocksProxy::Update(const PTree& proxy_prop) {
  auto version_optional = proxy_prop.get_child_optional("version");
  if (version_optional) {
    int version = version_optional.get().get_value<int>();
    if (version == 4) {
      version_ = Socks::Version::kV4;
    }
    if (version == 5) {
      version_ = Socks::Version::kV5;
    }
  }

  auto host_optional = proxy_prop.get_child_optional("host");
  if (host_optional) {
    host_ = host_optional.get().data();
    boost::trim(host_);
  }

  auto port_optional = proxy_prop.get_child_optional("port");
  if (port_optional) {
    port_ = port_optional.get().data();
    boost::trim(port_);
  }
}

void SocksProxy::Log() const {
  if (IsSet()) {
    SSF_LOG(kLogInfo) << "[config][socks proxy] <V"
                      << std::to_string(ToIntegral(version_)) << " " << host_
                      << ":" << port_ << ">";
  } else {
    SSF_LOG(kLogInfo) << "[config][socks proxy] <None>";
  }
}

}  // config
}  // ssf