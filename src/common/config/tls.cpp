#include <sstream>

#include <boost/algorithm/string.hpp>

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

void Tls::Update(const Json& tls_prop) {
  // ca cert
  if (tls_prop.count("ca_cert_path") == 1) {
    std::string ca_cert_path(tls_prop.at("ca_cert_path").get<std::string>());
    boost::trim(ca_cert_path);
    ca_cert_.Set(TlsParam::Type::kFile, ca_cert_path);
  }
  if (tls_prop.count("ca_cert_buffer") == 1) {
    std::string ca_cert_buffer(
        tls_prop.at("ca_cert_buffer").get<std::string>());
    boost::trim(ca_cert_buffer);
    ca_cert_.Set(TlsParam::Type::kBuffer, ca_cert_buffer);
  }

  // cert
  if (tls_prop.count("cert_path") == 1) {
    std::string cert_path(tls_prop.at("cert_path").get<std::string>());
    boost::trim(cert_path);
    cert_.Set(TlsParam::Type::kFile, cert_path);
  }
  if (tls_prop.count("cert_buffer") == 1) {
    std::string cert_buffer(tls_prop.at("cert_buffer").get<std::string>());
    boost::trim(cert_buffer);
    cert_.Set(TlsParam::Type::kBuffer, cert_buffer);
  }

  // cert key
  if (tls_prop.count("key_path") == 1) {
    std::string key_path(tls_prop.at("key_path").get<std::string>());
    boost::trim(key_path);
    key_.Set(TlsParam::Type::kFile, key_path);
  }
  if (tls_prop.count("key_buffer") == 1) {
    std::string key_buffer(tls_prop.at("key_buffer").get<std::string>());
    boost::trim(key_buffer);
    key_.Set(TlsParam::Type::kBuffer, key_buffer);
  }

  // key password
  if (tls_prop.count("key_password") == 1) {
    key_password_ = tls_prop.at("key_password").get<std::string>();
    boost::trim(key_password_);
  }

  // dh
  if (tls_prop.count("dh_path") == 1) {
    std::string dh_path(tls_prop.at("dh_path").get<std::string>());
    boost::trim(dh_path);
    dh_.Set(TlsParam::Type::kFile, dh_path);
  }
  if (tls_prop.count("dh_buffer") == 1) {
    std::string dh_buffer(tls_prop.at("dh_buffer").get<std::string>());
    boost::trim(dh_buffer);
    dh_.Set(TlsParam::Type::kBuffer, dh_buffer);
  }

  // cipher algorithms
  if (tls_prop.count("cipher_alg") == 1) {
    cipher_alg_ = tls_prop.at("cipher_alg").get<std::string>();
    boost::trim(cipher_alg_);
  }
}

void Tls::Log() const {
#ifdef TLS_OVER_TCP_LINK
  SSF_LOG("config", info, "[tls] CA cert path: <{}>", ca_cert_.ToString());
  SSF_LOG("config", info, "[tls] cert path: <{}>", cert_.ToString());
  SSF_LOG("config", info, "[tls] key path: <{}>", key_.ToString());
  SSF_LOG("config", info, "[tls] key password: <{}>", key_password_);
  SSF_LOG("config", info, "[tls] dh path: <{}>", dh_.ToString());
  SSF_LOG("config", info, "[tls] cipher suite: <{}>", cipher_alg_);
#endif
}

}  // config
}  // ssf
