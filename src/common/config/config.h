#ifndef SSF_COMMON_CONFIG_CONFIG_H_
#define SSF_COMMON_CONFIG_CONFIG_H_

#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

namespace ssf {
namespace config {

class Tls {
 public:
  Tls();

 public:
  inline std::string ca_cert_path() const { return ca_cert_path_; }

  inline void set_ca_cert_path(const std::string& ca_cert_path) {
    ca_cert_path_ = ca_cert_path;
  }

  inline std::string cert_path() const { return cert_path_; }

  inline void set_cert_path(const std::string& cert_path) {
    cert_path_ = cert_path;
  }

  inline std::string key_path() const { return key_path_; }

  inline void set_key_path(const std::string& key_path) {
    key_path_ = key_path;
  }

  inline std::string key_password() const { return key_password_; }

  inline void set_key_password(const std::string& key_password) {
    key_password_ = key_password;
  }

  inline std::string dh_path() const { return dh_path_; }

  inline void set_dh_path(const std::string& dh_path) { dh_path_ = dh_path; }

  inline std::string cipher_alg() const { return cipher_alg_; }

  inline void set_cipher_alg(const std::string& cipher_alg) {
    cipher_alg_ = cipher_alg;
  }

 private:
  // CA certificate filepath
  std::string ca_cert_path_;
  // Client certificate filepath
  std::string cert_path_;
  // Client key filepath
  std::string key_path_;
  // Client key password
  std::string key_password_;
  // Diffie-Hellman ephemeral parameters filepath
  std::string dh_path_;
  // Allowed cipher suite algorithms
  std::string cipher_alg_;
};

struct Proxy {
 public:
  Proxy();

  inline bool IsSet() const { return http_addr_ != "" && http_port_ != ""; }

 public:
  inline std::string http_addr() const { return http_addr_; }

  inline void set_http_addr(const std::string& http_addr) {
    http_addr_ = http_addr;
  }

  inline std::string http_port() const { return http_port_; }

  inline void set_http_port(const std::string& http_port) {
    http_port_ = http_port;
  }

 private:
  // HTTP proxy address
  std::string http_addr_;
  // HTTP proxy port
  std::string http_port_;
};

class Config {
 public:
  Config();

 public:
  /**
   * Update configuration with JSON file
   * @param filepath config filepath (relative or absolute)
   * @param ec error code set if update failed
   *
   * Format example (default values):
   * {
   *   "ssf": {
   *     "tls" : {
   *       "ca_cert_path": "./certs/trusted/ca.crt",
   *       "cert_path": "./certs/certificate.crt",
   *       "key_path": "./certs/private.key",
   *       "key_password": "",
   *       "dh_path": "./certs/dh4096.pem",
   *       "cipher_alg": "DHE-RSA-AES256-GCM-SHA384"
   *     },
   *     "proxy" : {
   *       "http_addr": "",
   *       "http_port": ""
   *     }
   *   }
   * }
   */
  void Update(const std::string& filepath, boost::system::error_code& ec);

  /**
   * Log configuration
   */
  void Log() const;
  
  inline const Tls& tls() const { return tls_; }
  inline Tls& tls() { return tls_; }
  
  inline const Proxy& proxy() const { return proxy_; }
  inline Proxy& proxy() { return proxy_; }

 private:
  void UpdateTls(const boost::property_tree::ptree& pt);
  void UpdateProxy(const boost::property_tree::ptree& pt);

 private:
  Tls tls_;
  Proxy proxy_;
};

}  // config
}  // ssf

#endif  // SSF_COMMON_CONFIG_CONFIG_H_
