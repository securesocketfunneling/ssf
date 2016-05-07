#include <string>

#include <boost/property_tree/json_parser.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "common/config/config.h"
#include "common/error/error.h"

namespace ssf {
namespace config {

Tls::Tls()
    : ca_cert_path("./certs/trusted/ca.crt"),
      cert_path("./certs/certificate.crt"),
      key_path("./certs/private.key"),
      key_password(""),
      dh_path("./certs/dh4096.pem"),
      cipher_alg("DHE-RSA-AES256-GCM-SHA384") {}

Proxy::Proxy() : http_addr(""), http_port("") {}

Config::Config() : tls(), proxy() {}

void Config::Update(const std::string& filepath,
                    boost::system::error_code& ec) {
  ec.assign(::error::success, ::error::get_ssf_category());
  if (filepath == "") {
    return;
  }

  try {
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(filepath, pt);

    UpdateTls(pt);
    UpdateProxy(pt);
  } catch (const std::exception& e) {
    SSF_LOG(kLogError) << "config: error reading SSF config file: " << e.what();
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
  }
}

void Config::UpdateTls(const boost::property_tree::ptree& pt) {
  auto tls_optional = pt.get_child_optional("ssf.tls");
  if (!tls_optional) {
    return;
  }

  auto& tls_prop = tls_optional.get();

  auto ca_cert_path_optional = tls_prop.get_child_optional("ca_cert_path");
  if (ca_cert_path_optional) {
    tls.ca_cert_path = ca_cert_path_optional.get().data();
  }

  auto cert_path_optional = tls_prop.get_child_optional("cert_path");
  if (cert_path_optional) {
    tls.cert_path = cert_path_optional.get().data();
  }

  auto key_path_optional = tls_prop.get_child_optional("key_path");
  if (key_path_optional) {
    tls.key_path = key_path_optional.get().data();
  }

  auto key_password_optional = tls_prop.get_child_optional("key_password");
  if (key_password_optional) {
    tls.key_password = key_password_optional.get().data();
  }

  auto dh_path_optional = tls_prop.get_child_optional("dh_path");
  if (dh_path_optional) {
    tls.dh_path = dh_path_optional.get().data();
  }

  auto cipher_alg_optional = tls_prop.get_child_optional("cipher_alg");
  if (cipher_alg_optional) {
    tls.cipher_alg = cipher_alg_optional.get().data();
  }
}

void Config::UpdateProxy(const boost::property_tree::ptree& pt) {
  auto proxy_optional = pt.get_child_optional("ssf.proxy");
  if (!proxy_optional) {
    return;
  }

  auto& proxy_prop = proxy_optional.get();

  auto http_addr_optional = proxy_prop.get_child_optional("http_addr");
  if (http_addr_optional) {
    proxy.http_addr = http_addr_optional.get().data();
  }

  auto http_port_optional = proxy_prop.get_child_optional("http_port");
  if (http_port_optional) {
    proxy.http_port = http_port_optional.get().data();
  }
}

}  // config
}  // ssf
