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

Proxy::Proxy() : http_addr_(""), http_port_("") {}

Config::Config() : tls_(), proxy_() {}

void Config::Update(const std::string& filepath,
                    boost::system::error_code& ec) {
  std::string conf_file("config.json");
  ec.assign(::error::success, ::error::get_ssf_category());
  if (filepath == "") {
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
    UpdateProxy(pt);
  } catch (const std::exception& e) {
    SSF_LOG(kLogError) << "config[ssf]: error reading SSF config file: "
                       << e.what();
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
  }
}

void Config::Log() const {
#ifdef TLS_OVER_TCP_LINK
  SSF_LOG(kLogInfo) << "config[tls]: CA cert path: <" << tls_.ca_cert_path()
                    << ">";
  SSF_LOG(kLogInfo) << "config[tls]: cert path: <" << tls_.cert_path() << ">";
  SSF_LOG(kLogInfo) << "config[tls]: key path: <" << tls_.key_path() << ">";
  SSF_LOG(kLogInfo) << "config[tls]: key password: <" << tls_.key_password()
                    << ">";
  SSF_LOG(kLogInfo) << "config[tls]: dh path: <" << tls_.dh_path() << ">";
  SSF_LOG(kLogInfo) << "config[tls]: cipher suite: <" << tls_.cipher_alg()
                    << ">";
#endif

  if (proxy_.IsSet()) {
    SSF_LOG(kLogInfo) << "config[proxy]: <" << proxy_.http_addr() << ":"
                      << proxy_.http_port() << ">";
  } else {
    SSF_LOG(kLogInfo) << "config[proxy]: <None>";
  }
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

void Config::UpdateProxy(const boost::property_tree::ptree& pt) {
  auto proxy_optional = pt.get_child_optional("ssf.proxy");
  if (!proxy_optional) {
    SSF_LOG(kLogDebug) << "config[update]: proxy configuration not found";
    return;
  }

  auto& proxy_prop = proxy_optional.get();

  auto http_addr_optional = proxy_prop.get_child_optional("http_addr");
  if (http_addr_optional) {
    proxy_.set_http_addr(http_addr_optional.get().data());
  }

  auto http_port_optional = proxy_prop.get_child_optional("http_port");
  if (http_port_optional) {
    proxy_.set_http_port(http_port_optional.get().data());
  }
}

}  // config
}  // ssf
