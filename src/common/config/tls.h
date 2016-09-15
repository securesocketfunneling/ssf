#ifndef SSF_COMMON_CONFIG_TLS_H_
#define SSF_COMMON_CONFIG_TLS_H_

#include <string>

#include <boost/property_tree/ptree.hpp>

namespace ssf {
namespace config {

class Tls {
 public:
  using PTree = boost::property_tree::ptree;

 public:
  Tls();

 public:
  void Update(const PTree& pt);

  void Log() const;

  inline std::string ca_cert_path() const { return ca_cert_path_; }

  inline std::string cert_path() const { return cert_path_; }

  inline std::string key_path() const { return key_path_; }

  inline std::string key_password() const { return key_password_; }

  inline std::string dh_path() const { return dh_path_; }

  inline std::string cipher_alg() const { return cipher_alg_; }

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

}  // config
}  // ssf

#endif  // SSF_COMMON_CONFIG_TLS_H_