#include "common/config/config.h"

#include <string>

#include <boost/log/trivial.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/system/error_code.hpp>

#include "common/error/error.h"

ssf::Config ssf::LoadConfig(const std::string& filepath,
                            boost::system::error_code& ec) {
  using boost::property_tree::ptree;
  ptree pt;
  ssf::Config config;

  try {
    if (filepath != "") {
      boost::property_tree::read_json(filepath, pt);

      auto tls_optional = pt.get_child_optional("ssf.tls");
      if (tls_optional) {
        auto& tls = tls_optional.get();

        auto ca_cert_path_optional = tls.get_child_optional("ca_cert_path");
        if (ca_cert_path_optional) {
          config.tls.ca_cert_path = ca_cert_path_optional.get().data();
        }

        auto cert_path_optional = tls.get_child_optional("cert_path");
        if (cert_path_optional) {
          config.tls.cert_path = cert_path_optional.get().data();
        }

        auto key_path_optional = tls.get_child_optional("key_path");
        if (key_path_optional) {
          config.tls.key_path = key_path_optional.get().data();
        }

        auto key_password_optional = tls.get_child_optional("key_password");
        if (key_password_optional) {
          config.tls.key_password = key_password_optional.get().data();
        }

        auto dh_path_optional = tls.get_child_optional("dh_path");
        if (dh_path_optional) {
          config.tls.dh_path = dh_path_optional.get().data();
        }

        auto cipher_alg_optional = tls.get_child_optional("cipher_alg");
        if (cipher_alg_optional) {
          config.tls.cipher_alg = cipher_alg_optional.get().data();
        }
      }
    }
    ec.assign(::error::success, ::error::get_ssf_category());

    return config;
  } catch (const std::exception& e) {
    BOOST_LOG_TRIVIAL(error)
        << "config: error reading SSF config file : " << e.what();
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());

    return ssf::Config();
  }
}
