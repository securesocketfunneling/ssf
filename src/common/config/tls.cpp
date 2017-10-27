#include <sstream>

#include <ssf/log/log.h>

#include "common/config/tls.h"

namespace ssf {
namespace config {

TlsParam::TlsParam(Type param_type, const std::string& param_value) {
  Set(param_type, param_value);
}

void TlsParam::Set(Type param_type, const std::string& param_value) {
  type_ = param_type;
  value_ = param_value;
}

std::string TlsParam::ToString() const {
  std::stringstream ss;
  switch (type_) {
    case Type::kBuffer:
      ss << "buffer";
      break;
    case Type::kFile:
      ss << "file: " << value_;
      break;
    default:
      ss << "unknown src: " << value_;
      break;
  }
  return ss.str();
}

bool TlsParam::IsBuffer() const { return type_ == Type::kBuffer; }

Tls::Tls()
    : ca_cert_(TlsParam::Type::kFile, "./certs/trusted/ca.crt"),
      cert_(TlsParam::Type::kFile, "./certs/certificate.crt"),
      key_(TlsParam::Type::kFile, "./certs/private.key"),
      key_password_(""),
      dh_(TlsParam::Type::kFile, "./certs/dh4096.pem"),
      cipher_alg_("DHE-RSA-AES256-GCM-SHA384") {}

void Tls::Update(const PTree& tls_prop) {
  // ca cert
  auto ca_cert_path_optional = tls_prop.get_child_optional("ca_cert_path");
  if (ca_cert_path_optional) {
    ca_cert_.Set(TlsParam::Type::kFile, ca_cert_path_optional.get().data());
  }
  auto ca_cert_buffer_optional = tls_prop.get_child_optional("ca_cert_buffer");
  if (ca_cert_buffer_optional) {
    ca_cert_.Set(TlsParam::Type::kBuffer, ca_cert_buffer_optional.get().data());
  }

  // cert
  auto cert_path_optional = tls_prop.get_child_optional("cert_path");
  if (cert_path_optional) {
    cert_.Set(TlsParam::Type::kFile, cert_path_optional.get().data());
  }
  auto cert_buffer_optional = tls_prop.get_child_optional("cert_buffer");
  if (cert_buffer_optional) {
    cert_.Set(TlsParam::Type::kBuffer, cert_buffer_optional.get().data());
  }

  // cert key
  auto key_path_optional = tls_prop.get_child_optional("key_path");
  if (key_path_optional) {
    key_.Set(TlsParam::Type::kFile, key_path_optional.get().data());
  }
  auto key_buffer_optional = tls_prop.get_child_optional("key_buffer");
  if (key_buffer_optional) {
    key_.Set(TlsParam::Type::kBuffer, key_buffer_optional.get().data());
  }

  // key password
  auto key_password_optional = tls_prop.get_child_optional("key_password");
  if (key_password_optional) {
    key_password_ = key_password_optional.get().data();
  }

  // dh
  auto dh_path_optional = tls_prop.get_child_optional("dh_path");
  if (dh_path_optional) {
    dh_.Set(TlsParam::Type::kFile, dh_path_optional.get().data());
  }
  auto dh_buffer_optional = tls_prop.get_child_optional("dh_buffer");
  if (dh_buffer_optional) {
    dh_.Set(TlsParam::Type::kBuffer, dh_buffer_optional.get().data());
  }

  // cipher algorithms
  auto cipher_alg_optional = tls_prop.get_child_optional("cipher_alg");
  if (cipher_alg_optional) {
    cipher_alg_ = cipher_alg_optional.get().data();
  }
}

void Tls::Log() const {
#ifdef TLS_OVER_TCP_LINK
  SSF_LOG(kLogInfo) << "[config][tls] CA cert path: <" << ca_cert_.ToString()
                    << ">";
  SSF_LOG(kLogInfo) << "[config][tls] cert path: <" << cert_.ToString() << ">";
  SSF_LOG(kLogInfo) << "[config][tls] key path: <" << key_.ToString() << ">";
  SSF_LOG(kLogInfo) << "[config][tls] key password: <" << key_password_ << ">";
  SSF_LOG(kLogInfo) << "[config][tls] dh path: <" << dh_.ToString() << ">";
  SSF_LOG(kLogInfo) << "[config][tls] cipher suite: <" << cipher_alg_ << ">";
#endif
}

}  // config
}  // ssf
