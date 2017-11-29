#include "common/config/proxy.h"

#include <boost/algorithm/string.hpp>

#include <ssf/log/log.h>
#include <ssf/utils/enum.h>

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

void HttpProxy::Update(const Json& proxy_prop) {
  if (proxy_prop.count("host") == 1) {
    host_ = proxy_prop.at("host").get<std::string>();
    boost::trim(host_);
  }

  if (proxy_prop.count("port") == 1) {
    port_ = proxy_prop.at("port").get<std::string>();
    boost::trim(port_);
  }

  if (proxy_prop.count("user_agent") == 1) {
    user_agent_ = proxy_prop.at("user_agent").get<std::string>();
    boost::trim(user_agent_);
  }

  if (proxy_prop.count("credentials") == 1) {
    auto credentials = proxy_prop.at("credentials");

    if (credentials.count("username") == 1) {
      username_ = credentials.at("username").get<std::string>();
      boost::trim(username_);
    }

    if (credentials.count("domain") == 1) {
      domain_ = credentials.at("domain").get<std::string>();
      boost::trim(domain_);
    }

    if (credentials.count("password") == 1) {
      password_ = credentials.at("password").get<std::string>();
      boost::trim(password_);
    }

    if (credentials.count("reuse_ntlm") == 1) {
      reuse_ntlm_ = credentials.at("reuse_ntlm").get<bool>();
    }

    if (credentials.count("reuse_kerb") == 1) {
      reuse_kerb_ = credentials.at("reuse_kerb").get<bool>();
    }
  }
}

void HttpProxy::Log() const {
  if (IsSet()) {
    SSF_LOG("config", info, "[http proxy] <{}:{}>", host_, port_);
    if (!username_.empty()) {
      SSF_LOG("config", info, "[http proxy] username: <{}>", username_);
    }
    SSF_LOG("config", info, "[http proxy] reuse NTLM credentials <{}>",
            (reuse_ntlm_ ? "true" : "false"));
    SSF_LOG("config", info, "[http proxy] reuse Kerberos credentials <{}>",
            (reuse_kerb_ ? "true" : "false"));
  } else {
    SSF_LOG("config", info, "[http proxy] <None>");
  }
}

SocksProxy::SocksProxy()
    : version_(Socks::Version::kVUnknown), host_(""), port_("") {}

void SocksProxy::Update(const Json& proxy_prop) {
  if (proxy_prop.count("version") == 1) {
    int version = proxy_prop.at("version").get<int>();
    if (version == 4) {
      version_ = Socks::Version::kV4;
    }
    if (version == 5) {
      version_ = Socks::Version::kV5;
    }
  }

  if (proxy_prop.count("host") == 1) {
    host_ = proxy_prop.at("host").get<std::string>();
    boost::trim(host_);
  }

  if (proxy_prop.count("port") == 1) {
    port_ = proxy_prop.at("port").get<std::string>();
    boost::trim(port_);
  }
}

void SocksProxy::Log() const {
  if (IsSet()) {
    SSF_LOG("config", info, "[socks proxy] <V{} {}:{}>",
            std::to_string(ToIntegral(version_)), host_, port_);
  } else {
    SSF_LOG("config", info, "[socks proxy] <None>");
  }
}

}  // config
}  // ssf