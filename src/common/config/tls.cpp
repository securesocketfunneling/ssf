#include <ssf/log/log.h>

#include "common/config/tls.h"

namespace ssf {
namespace config {

Tls::Tls()
    : ca_cert_path_("./certs/trusted/ca.crt"),
      cert_path_("./certs/certificate.crt"),
      key_path_("./certs/private.key"),
      key_password_(""),
      dh_path_("./certs/dh4096.pem"),
      cipher_alg_("DHE-RSA-AES256-GCM-SHA384") {}

void Tls::Update(const PTree& tls_prop) {
  auto ca_cert_path_optional = tls_prop.get_child_optional("ca_cert_path");
  if (ca_cert_path_optional) {
    ca_cert_path_ = ca_cert_path_optional.get().data();
  }

  auto cert_path_optional = tls_prop.get_child_optional("cert_path");
  if (cert_path_optional) {
    cert_path_ = cert_path_optional.get().data();
  }

  auto key_path_optional = tls_prop.get_child_optional("key_path");
  if (key_path_optional) {
    key_path_ = key_path_optional.get().data();
  }

  auto key_password_optional = tls_prop.get_child_optional("key_password");
  if (key_password_optional) {
    key_password_ = key_password_optional.get().data();
  }

  auto dh_path_optional = tls_prop.get_child_optional("dh_path");
  if (dh_path_optional) {
    dh_path_ = dh_path_optional.get().data();
  }

  auto cipher_alg_optional = tls_prop.get_child_optional("cipher_alg");
  if (cipher_alg_optional) {
    cipher_alg_ = cipher_alg_optional.get().data();
  }
}

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

}  // config
}  // ssf
