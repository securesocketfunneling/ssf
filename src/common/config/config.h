#ifndef SSF_COMMON_CONFIG_CONFIG_H_
#define SSF_COMMON_CONFIG_CONFIG_H_

#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

#include "common/config/tls.h"
#include "common/config/proxy.h"
#include "common/config/services.h"

namespace ssf {
namespace config {

class Config {
 public:
  using PTree = boost::property_tree::ptree;

 public:
  Config();

 public:
  /**
   * Initialize config with default values
   * Format example (default values):
   * {
   *   "ssf": {
   *     "tls": {
   *       "ca_cert_path": "./certs/trusted/ca.crt",
   *       "cert_path": "./certs/certificate.crt",
   *       "key_path": "./certs/private.key",
   *       "key_password": "",
   *       "dh_path": "./certs/dh4096.pem",
   *       "cipher_alg": "DHE-RSA-AES256-GCM-SHA384"
   *     },
   *     "http_proxy": {
   *       "host": "",
   *       "port": "",
   *       "credentials": {
   *         "username": "",
   *         "password": "",
   *         "domain": "",
   *         "reuse_ntlm": true,
   *         "reuse_nego": true
   *       }
   *     },
   *     "socks_proxy": {
   *       "version": 5,
   *       "host": "",
   *       "port": "1080"
   *     },
   *     "services": {
   *       "datagram_forwarder": { "enable": true },
   *       "datagram_listener": {
   *         "enable": true,
   *         "gateway_ports": false
   *       },
   *       "stream_forwarder": { "enable": true },
   *       "stream_listener": {
   *         "enable": true,
   *         "gateway_ports": false
   *       },
   *       "file_copy": { "enable": false },
   *       "shell": {
   *         "enable": false,
   *         "path": "/bin/bash|C:\\windows\\system32\\cmd.exe",
   *         "args": ""
   *       },
   *       "socks": { "enable": true }
   *     }
   *   }
   * }
   */
  void Init();

  /**
   * Update configuration with JSON file
   * If no file provided, try to load config from "config.json" file
   * @param filepath config filepath (relative or absolute)
   * @param ec error code set if update failed
   */
  void UpdateFromFile(const std::string& filepath,
                      boost::system::error_code& ec);

  /**
   * Update configuration from ptree
   * @param pt
   * @param ec error code set if update failed
   */
  void UpdateFromString(const std::string& config_string,
                        boost::system::error_code& ec);

  /**
   * Log configuration
   */
  void Log() const;

  /**
   * Log status
   */
  void LogStatus() const;

  inline const Tls& tls() const { return tls_; }
  inline Tls& tls() { return tls_; }

  inline const HttpProxy& http_proxy() const { return http_proxy_; }
  inline HttpProxy& http_proxy() { return http_proxy_; }

  inline const SocksProxy& socks_proxy() const { return socks_proxy_; }
  inline SocksProxy& socks_proxy() { return socks_proxy_; }

  inline const Services& services() const { return services_; }
  inline Services& services() { return services_; }

 private:
  void UpdateFromPTree(const PTree& pt);
  void UpdateTls(const PTree& pt);
  void UpdateHttpProxy(const PTree& pt);
  void UpdateSocksProxy(const PTree& pt);
  void UpdateServices(const PTree& pt);

 private:
  static const char* default_config_;
  Tls tls_;
  HttpProxy http_proxy_;
  SocksProxy socks_proxy_;
  Services services_;
};

}  // config
}  // ssf

#endif  // SSF_COMMON_CONFIG_CONFIG_H_
