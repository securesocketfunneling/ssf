#ifndef SSF_COMMON_CONFIG_CONFIG_H_
#define SSF_COMMON_CONFIG_CONFIG_H_

#include <string>

#include <boost/system/error_code.hpp>

namespace ssf {
namespace config {

struct Tls {
  Tls();

  std::string ca_cert_path;
  std::string cert_path;
  std::string key_path;
  std::string key_password;
  std::string dh_path;
  std::string cipher_alg;
};

struct Proxy {
  Proxy();

  std::string http_addr;
  std::string http_port;
};

struct Config {
  Config();
  void Update(const std::string& filepath, boost::system::error_code& ec);

  Tls tls;
  Proxy proxy;
};

}  // config

}  // ssf

#endif  // SSF_COMMON_CONFIG_CONFIG_H_
