#ifndef SSF_COMMON_CONFIG_CONFIG_H_
#define SSF_COMMON_CONFIG_CONFIG_H_

#include <string>

#include <boost/asio/detail/config.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

#if defined(BOOST_ASIO_WINDOWS)
#define SSF_PROCESS_SERVICE_BINARY_PATH "C:\\windows\\system32\\cmd.exe"
#else
#define SSF_PROCESS_SERVICE_BINARY_PATH "/bin/bash"
#endif

namespace ssf {
namespace config {

class Tls {
 public:
  Tls();

 public:
  void Log() const;

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

 public:
  void Log() const;

  inline bool IsSet() const {
    return !host_.empty() && !port_.empty();
  }

  inline std::string host() const { return host_; }

  inline void set_host(const std::string& host) {
    host_ = host;
  }

  inline std::string port() const { return port_; }

  inline void set_port(const std::string& port) {
    port_ = port;
  }

  inline std::string username() const { return username_; }

  inline void set_username(const std::string& username) {
    username_ = username;
  }

  inline std::string domain() const { return domain_; }

  inline void set_domain(const std::string& domain) {
    domain_ = domain;
  }

  inline std::string password() const { return password_; }

  inline void set_password(const std::string& password) {
    password_ = password;
  }

  inline bool reuse_ntlm() const { return reuse_ntlm_; }

  inline void set_reuse_ntlm(bool reuse_ntlm) {
    reuse_ntlm_ = reuse_ntlm;
  }

  inline bool reuse_kerb() const { return reuse_kerb_; }

  inline void set_reuse_kerb(bool reuse_kerb) {
    reuse_kerb_ = reuse_kerb;
  }

 private:
  // Proxy host
  std::string host_;
  // Proxy port
  std::string port_;
  // Credentials username
  std::string username_;
  // Credentials user's domain
  std::string domain_;
  // Credentials password
  std::string password_;
  // Reuse default NTLM credentials
  bool reuse_ntlm_;
  // Reuse default Kerberos/Negotiate credentials
  bool reuse_kerb_;
};

class ProcessService {
 public:
  ProcessService();
  ProcessService(const ProcessService& process_service);

  inline std::string path() const { return path_; }
  inline void set_path(const std::string& path) { path_ = path; }

  inline std::string args() const { return args_; }
  inline void set_args(const std::string& args) { args_ = args; }

 private:
  std::string path_;
  std::string args_;
};

class Services {
 public:
  Services();
  Services(const Services& services);

  inline const ProcessService& process_service() const { return process_; }
  inline ProcessService& process() { return process_; }

  void UpdateProcessService(const boost::property_tree::ptree& pt);
  void Log() const;

 private:
  ProcessService process_;
};

class Config {
 public:
  Config();

 public:
  /**
   * Update configuration with JSON file
   * If no file provided, try to load config from "config.json" file
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
   *       "host": "",
   *       "port": "",
   *       "credentials": {
   *         "username": "",
   *         "password": "",
   *         "domain": "",
   *         "reuse_ntlm": "true",
   *         "reuse_nego": "true"
   *       }
   *     },
   *     "services": {
   *       "process": {
   *         "path": "/bin/bash",
   *         "args": ""
   *       }
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

  inline const Proxy& http_proxy() const { return http_proxy_; }
  inline Proxy& http_proxy() { return http_proxy_; }

  inline const Services& services() const { return services_; }
  inline Services& services() { return services_; }

 private:
  void UpdateTls(const boost::property_tree::ptree& pt);
  void UpdateHttpProxy(const boost::property_tree::ptree& pt);
  void UpdateServices(const boost::property_tree::ptree& pt);

 private:
  Tls tls_;
  Proxy http_proxy_;
  Services services_;
};

}  // config
}  // ssf

#endif  // SSF_COMMON_CONFIG_CONFIG_H_
