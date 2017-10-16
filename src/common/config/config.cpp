#include <fstream>
#include <regex>
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "common/config/config.h"
#include "common/error/error.h"

#if defined(BOOST_ASIO_WINDOWS)
#define SSF_PROCESS_SERVICE_BINARY_PATH \
  "\"C:\\\\windows\\\\system32\\\\cmd.exe\""
#else
#define SSF_PROCESS_SERVICE_BINARY_PATH "\"/bin/bash\""
#endif

namespace ssf {
namespace config {

Config::Config() : tls_(), http_proxy_(), services_() {}

void Config::Init() {
  boost::system::error_code ec;

  SSF_LOG(kLogDebug) << "config[ssf]: default configuration: "
                     << default_config_;

  UpdateFromString(default_config_, ec);
}

void Config::UpdateFromFile(const std::string& filepath,
                            boost::system::error_code& ec) {
  std::string conf_file("config.json");
  ec.assign(::error::success, ::error::get_ssf_category());
  if (filepath.empty()) {
    std::ifstream ifile(conf_file);
    if (!ifile.good()) {
      return;
    }
  } else {
    conf_file = filepath;
  }

  SSF_LOG(kLogInfo) << "config[ssf]: loading file <" << conf_file << ">";

  try {
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(conf_file, pt);

    std::stringstream ss_loaded_config;
    boost::property_tree::write_json(ss_loaded_config, pt);
    SSF_LOG(kLogDebug) << "config[ssf]: file configuration: " << std::endl
                       << ss_loaded_config.str();

    UpdateFromPTree(pt);
  } catch (const std::exception& e) {
    SSF_LOG(kLogError) << "config[ssf]: error reading SSF config file: "
                       << e.what();
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
  }
}

void Config::UpdateFromString(const std::string& config_string,
                              boost::system::error_code& ec) {
  std::stringstream ss_config;
  ss_config << config_string;

  try {
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(ss_config, pt);

    UpdateFromPTree(pt);
  } catch (const std::exception& e) {
    SSF_LOG(kLogError) << "config[ssf]: error reading config string: "
                       << e.what();
  }
}

void Config::Log() const {
  tls_.Log();
  http_proxy_.Log();
  socks_proxy_.Log();
  services_.Log();
  circuit_.Log();
}

void Config::LogStatus() const { services_.LogServiceStatus(); }

std::vector<char*> Config::GetArgv() const {
  std::vector<char*> res_argv(argv_.size() + 1);
  auto res_arg_it = res_argv.begin();
  for (const auto& arg : argv_) {
    *res_arg_it = const_cast<char*>(arg.data());
    ++res_arg_it;
  }
  // last element is nullptr
  *res_arg_it = nullptr;
  return res_argv;
}

void Config::UpdateFromPTree(const PTree& pt) {
  UpdateTls(pt);
  UpdateHttpProxy(pt);
  UpdateSocksProxy(pt);
  UpdateServices(pt);
  UpdateCircuit(pt);
  UpdateArguments(pt);
}

void Config::UpdateTls(const PTree& pt) {
  auto tls_optional = pt.get_child_optional("ssf.tls");
  if (!tls_optional) {
    SSF_LOG(kLogDebug) << "config[update]: TLS configuration not found";
    return;
  }

  tls_.Update(tls_optional.get());
}

void Config::UpdateHttpProxy(const PTree& pt) {
  auto proxy_optional = pt.get_child_optional("ssf.http_proxy");
  if (!proxy_optional) {
    SSF_LOG(kLogDebug) << "config[update]: http proxy configuration not found";
    return;
  }

  http_proxy_.Update(proxy_optional.get());
}

void Config::UpdateSocksProxy(const PTree& pt) {
  auto proxy_optional = pt.get_child_optional("ssf.socks_proxy");
  if (!proxy_optional) {
    SSF_LOG(kLogDebug) << "config[update]: socks proxy configuration not found";
    return;
  }

  socks_proxy_.Update(proxy_optional.get());
}

void Config::UpdateServices(const PTree& pt) {
  auto services_optional = pt.get_child_optional("ssf.services");
  if (!services_optional) {
    SSF_LOG(kLogDebug) << "config[update]: services configuration not found";
    return;
  }

  services_.Update(services_optional.get());
}

void Config::UpdateCircuit(const PTree& pt) {
  auto circuit_optional = pt.get_child_optional("ssf.circuit");
  if (!circuit_optional) {
    SSF_LOG(kLogDebug) << "config[update]: circuit configuration not found";
    return;
  }

  circuit_.Update(circuit_optional.get());
}

void Config::UpdateArguments(const PTree& pt) {
  auto arguments_optional = pt.get_child_optional("ssf.arguments");
  if (!arguments_optional) {
    SSF_LOG(kLogDebug) << "config[update]: arguments configuration not found";
    return;
  }

  // basic arguments parsing
  std::string arguments(arguments_optional.get().data());
  if (arguments.empty()) {
    return;
  }
  argv_.clear();

  argv_.push_back("ssf");
  // quoted arg or arg between space
  std::regex argv_regex("(\"[^\"]+\"|[^\\s\"]+)");
  auto argv_it =
      std::sregex_iterator(arguments.begin(), arguments.end(), argv_regex);
  auto argv_end = std::sregex_iterator();
  while (argv_it != argv_end) {
    auto argv = argv_it->str();
    // trim double quotes
    boost::trim_if(argv, boost::is_any_of("\""));
    argv_.push_back(argv);
    ++argv_it;
  }
}

const char* Config::default_config_ = R"RAWSTRING(
{
  "ssf": {
    "tls": {
      "ca_cert_path": "./certs/trusted/ca.crt",
      "cert_path": "./certs/certificate.crt",
      "key_path": "./certs/private.key",
      "key_password": "",
      "dh_path": "./certs/dh4096.pem",
      "cipher_alg": "DHE-RSA-AES256-GCM-SHA384"
    },
    "http_proxy": {
      "host": "",
      "port": "",
      "credentials": {
        "username": "",
        "password": "",
        "domain": "",
        "reuse_ntlm": true,
        "reuse_nego": true
      }
    },
    "socks_proxy": {
      "version": 5,
      "host": "",
      "port": "1080"
    },
    "services": {
      "datagram_forwarder": { "enable": true },
      "datagram_listener": {
        "enable": true,
        "gateway_ports": false
      },
      "stream_forwarder": { "enable": true },
      "stream_listener": {
        "enable": true,
        "gateway_ports": false
      },
      "copy": { "enable": false },
      "shell": {
        "enable": false,
        "path": )RAWSTRING" SSF_PROCESS_SERVICE_BINARY_PATH R"RAWSTRING(,
        "args": ""
      },
      "socks": { "enable": true }
    },
    "circuit": [],
    "arguments": ""
  }
}
)RAWSTRING";

}  // config
}  // ssf
