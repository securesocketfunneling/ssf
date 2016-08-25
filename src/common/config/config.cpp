#include <string>

#include <fstream>
#include <boost/property_tree/json_parser.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "common/config/config.h"
#include "common/error/error.h"

namespace ssf {
namespace config {

Tls::Tls()
    : ca_cert_path_("./certs/trusted/ca.crt"),
      cert_path_("./certs/certificate.crt"),
      key_path_("./certs/private.key"),
      key_password_(""),
      dh_path_("./certs/dh4096.pem"),
      cipher_alg_("DHE-RSA-AES256-GCM-SHA384") {}

void Tls::Log() const {
#ifdef TLS_OVER_TCP_LINK
  SSF_LOG(kLogInfo) << "config[tls]: CA cert path: <" << ca_cert_path_ << ">";
  SSF_LOG(kLogInfo) << "config[tls]: cert path: <" << cert_path_ << ">";
  SSF_LOG(kLogInfo) << "config[tls]: key path: <" << key_path_ << ">";
  SSF_LOG(kLogInfo) << "config[tls]: key password: <" << key_password_ << ">";
  SSF_LOG(kLogInfo) << "config[tls]: dh path: <" << dh_path_ << ">";
  SSF_LOG(kLogInfo) << "config[tls]: cipher suite: <" << cipher_alg_ << ">";
#endif
}

Proxy::Proxy()
    : host_(""),
      port_(""),
      username_(""),
      domain_(""),
      password_(""),
      reuse_ntlm_(true),
      reuse_kerb_(true) {}

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

ProcessService::ProcessService()
    : path_(SSF_PROCESS_SERVICE_BINARY_PATH), args_("") {}

ProcessService::ProcessService(const ProcessService& process_service)
    : path_(process_service.path_), args_(process_service.args_) {}

Services::Services() : process_() {}

Services::Services(const Services& services) : process_(services.process_) {}

void Services::UpdateProcessService(const boost::property_tree::ptree& pt) {
  auto shell_optional = pt.get_child_optional("shell");
  if (!shell_optional) {
    SSF_LOG(kLogDebug)
        << "config[update]: shell service configuration not found";
    return;
  }

  auto& shell_prop = shell_optional.get();
  auto path = shell_prop.get_child_optional("path");
  if (path) {
    process_.set_path(path.get().data());
  }
  auto args = shell_prop.get_child_optional("args");
  if (args) {
    process_.set_args(args.get().data());
  }
}

void Services::Log() const {
  SSF_LOG(kLogInfo) << "config[services][shell]: path: <"
                    << process_service().path() << ">";
  std::string args(process_service().args());
  if (!args.empty()) {
    SSF_LOG(kLogInfo) << "config[services][shell]: args: <" << args << ">";
  }
}

Config::Config() : tls_(), http_proxy_(), services_() {}

void Config::Update(const std::string& filepath,
                    boost::system::error_code& ec) {
  std::string conf_file("config.json");
  ec.assign(::error::success, ::error::get_ssf_category());
  if (filepath.empty()) {
    std::ifstream ifile(conf_file);
    if (!ifile.good()) {
      return;
    }
  } else {
    conf_file = filepath;
  }

  SSF_LOG(kLogInfo) << "config[ssf]: loading file <" << conf_file << ">";

  try {
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(conf_file, pt);

    UpdateTls(pt);
    UpdateHttpProxy(pt);
    UpdateServices(pt);
  } catch (const std::exception& e) {
    SSF_LOG(kLogError) << "config[ssf]: error reading SSF config file: "
                       << e.what();
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
  }
}

void Config::Log() const {
  tls_.Log();
  http_proxy_.Log();
  services_.Log();
}

void Config::UpdateTls(const boost::property_tree::ptree& pt) {
  auto tls_optional = pt.get_child_optional("ssf.tls");
  if (!tls_optional) {
    SSF_LOG(kLogDebug) << "config[update]: TLS configuration not found";
    return;
  }

  auto& tls_prop = tls_optional.get();

  auto ca_cert_path_optional = tls_prop.get_child_optional("ca_cert_path");
  if (ca_cert_path_optional) {
    tls_.set_ca_cert_path(ca_cert_path_optional.get().data());
  }

  auto cert_path_optional = tls_prop.get_child_optional("cert_path");
  if (cert_path_optional) {
    tls_.set_cert_path(cert_path_optional.get().data());
  }

  auto key_path_optional = tls_prop.get_child_optional("key_path");
  if (key_path_optional) {
    tls_.set_key_path(key_path_optional.get().data());
  }

  auto key_password_optional = tls_prop.get_child_optional("key_password");
  if (key_password_optional) {
    tls_.set_key_password(key_password_optional.get().data());
  }

  auto dh_path_optional = tls_prop.get_child_optional("dh_path");
  if (dh_path_optional) {
    tls_.set_dh_path(dh_path_optional.get().data());
  }

  auto cipher_alg_optional = tls_prop.get_child_optional("cipher_alg");
  if (cipher_alg_optional) {
    tls_.set_cipher_alg(cipher_alg_optional.get().data());
  }
}

void Config::UpdateHttpProxy(const boost::property_tree::ptree& pt) {
  auto proxy_optional = pt.get_child_optional("ssf.http_proxy");
  if (!proxy_optional) {
    SSF_LOG(kLogDebug) << "config[update]: proxy configuration not found";
    return;
  }

  auto& proxy_prop = proxy_optional.get();

  auto host_optional = proxy_prop.get_child_optional("host");
  if (host_optional) {
    http_proxy_.set_host(host_optional.get().data());
  }

  auto port_optional = proxy_prop.get_child_optional("port");
  if (port_optional) {
    http_proxy_.set_port(port_optional.get().data());
  }

  auto cred_username_optional =
      proxy_prop.get_child_optional("credentials.username");
  if (cred_username_optional) {
    http_proxy_.set_username(cred_username_optional.get().data());
  }

  auto cred_domain_optional =
      proxy_prop.get_child_optional("credentials.domain");
  if (cred_domain_optional) {
    http_proxy_.set_domain(cred_domain_optional.get().data());
  }

  auto cred_password_optional =
      proxy_prop.get_child_optional("credentials.password");
  if (cred_password_optional) {
    http_proxy_.set_password(cred_password_optional.get().data());
  }

  auto cred_reuse_ntlm_optional =
      proxy_prop.get_child_optional("credentials.reuse_ntlm");
  if (cred_reuse_ntlm_optional) {
    http_proxy_.set_reuse_ntlm(cred_reuse_ntlm_optional.get().data() == "true");
  }

  auto cred_reuse_kerb_optional =
      proxy_prop.get_child_optional("credentials.reuse_kerb");
  if (cred_reuse_kerb_optional) {
    http_proxy_.set_reuse_kerb(cred_reuse_kerb_optional.get().data() == "true");
  }
}

void Config::UpdateServices(const boost::property_tree::ptree& pt) {
  auto services_optional = pt.get_child_optional("ssf.services");
  if (!services_optional) {
    SSF_LOG(kLogDebug) << "config[update]: services configuration not found";
    return;
  }

  services_.UpdateProcessService(services_optional.get());
}

}  // config
}  // ssf
