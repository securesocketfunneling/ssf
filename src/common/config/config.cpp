#include <fstream>
#include <regex>
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/asio/detail/config.hpp>
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

  SSF_LOG("config", debug, "default configuration: {}", default_config_);

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

  SSF_LOG("config", info, "loading file <{}>", conf_file);

  try {
    std::ifstream file(conf_file);
    Json config;
    file >> config;

    std::stringstream ss_loaded_config;
    ss_loaded_config << std::setw(4) << config;
    SSF_LOG("config", debug, "custom configuration: {}",
            ss_loaded_config.str());

    UpdateFromJson(config);
  } catch (const std::exception& e) {
    (void)(e);
    SSF_LOG("config", error, "error parsing config file: {}", e.what());
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
  }
}

void Config::UpdateFromString(const std::string& config_string,
                              boost::system::error_code& ec) {
  std::stringstream ss_config;
  ss_config << config_string;

  try {
    Json config;
    ss_config >> config;

    UpdateFromJson(config);
  } catch (const std::exception& e) {
    (void)(e);
    SSF_LOG("config", error, "error parsing config string: {}", e.what());
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

void Config::UpdateFromJson(const Json& json) {
  if (json.count("ssf") == 0) {
    return;
  }
  auto ssf_config = json.at("ssf");
  UpdateTls(ssf_config);
  UpdateHttpProxy(ssf_config);
  UpdateSocksProxy(ssf_config);
  UpdateServices(ssf_config);
  UpdateCircuit(ssf_config);
  UpdateArguments(ssf_config);
}

void Config::UpdateTls(const Json& json) {
  if (json.count("tls") == 0) {
    SSF_LOG("config", debug, "update TLS: configuration not found");
    return;
  }

  tls_.Update(json.at("tls"));
}

void Config::UpdateHttpProxy(const Json& json) {
  if (json.count("http_proxy") == 0) {
    SSF_LOG("config", debug, "update HTTP proxy: configuration not found");
    return;
  }

  http_proxy_.Update(json.at("http_proxy"));
}

void Config::UpdateSocksProxy(const Json& json) {
  if (json.count("socks_proxy") == 0) {
    SSF_LOG("config", debug, "update SOCKS proxy: configuration not found");
    return;
  }

  socks_proxy_.Update(json.at("socks_proxy"));
}

void Config::UpdateServices(const Json& json) {
  if (json.count("services") == 0) {
    SSF_LOG("config", debug, "update services: configuration not found");
    return;
  }

  services_.Update(json.at("services"));
}

void Config::UpdateCircuit(const Json& json) {
  if (json.count("circuit") == 0) {
    SSF_LOG("config", debug, "update circuit: configuration not found");
    return;
  }

  circuit_.Update(json.at("circuit"));
}

void Config::UpdateArguments(const Json& json) {
  if (json.count("arguments") == 0) {
    SSF_LOG("config", debug, "update arguments: configuration not found");
    return;
  }

  // basic arguments parsing
  std::string arguments(json.at("arguments").get<std::string>());
  if (arguments.empty()) {
    return;
  }
  argv_.clear();

  argv_.push_back("bin");
  // quoted arg or arg between space
  std::regex argv_regex("(\"[^\"]+\"|[^\\s\"]+)");
  auto argv_it =
      std::sregex_iterator(arguments.begin(), arguments.end(), argv_regex);
  auto argv_end = std::sregex_iterator();
  while (argv_it != argv_end) {
    auto argv = argv_it->str();
    // trim double quotes
    boost::trim_if(argv, boost::is_any_of("\""));
    boost::trim(argv);
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
      "user_agent": "",
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
