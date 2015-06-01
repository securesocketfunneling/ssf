#ifndef SSF_COMMON_CONFIG_CONFIG_H_
#define SSF_COMMON_CONFIG_CONFIG_H_

#include <boost/system/error_code.hpp>
#include <string>

namespace ssf {
struct Config
{
  Config() : tls() {
  }

  struct Tls
  {
    Tls() : ca_cert_path("./certs/trusted/ca.crt"),
            cert_path("./certs/certificate.crt"),
            key_path("./certs/private.key"),
            key_password(""),
            dh_path("./certs/dh4096.pem"),
            cipher_alg("DHE-RSA-AES256-GCM-SHA384") {
    }

    std::string ca_cert_path;
    std::string cert_path;
    std::string key_path;
    std::string key_password;
    std::string dh_path;
    std::string cipher_alg;
  };
  Tls tls;
};

Config LoadConfig(const std::string& filepath,
                     boost::system::error_code& ec);

}  // ssf
#endif  // SSF_COMMON_CONFIG_CONFIG_H_
