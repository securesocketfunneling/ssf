#ifndef SSF_COMMON_CONFIG_CONFIG_H_
#define SSF_COMMON_CONFIG_CONFIG_H_

#include <string>

#include <boost/property_tree/ptree.hpp>
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

class Config {
 public:
  Config();

  void Update(const std::string& filepath, boost::system::error_code& ec);

 private:
  void UpdateTls(const boost::property_tree::ptree& pt);
  void UpdateProxy(const boost::property_tree::ptree& pt);

 public:
  Tls tls;
  Proxy proxy;
};

}  // config

}  // ssf

#endif  // SSF_COMMON_CONFIG_CONFIG_H_
