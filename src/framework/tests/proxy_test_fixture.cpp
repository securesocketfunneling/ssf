#include <fstream>

#include <boost/property_tree/json_parser.hpp>

#include "tests/proxy_test_fixture.h"

namespace ssf {
namespace tests {

Address::Address() : addr_(""), port_("") {}

Address::Address(const std::string& addr, const std::string& port)
    : addr_(addr), port_(port) {}

Address::Address(const Address& address)
    : addr_(address.addr_), port_(address.port_) {}

Address& Address::operator=(const Address& address) {
  addr_ = address.addr_;
  port_ = address.port_;

  return *this;
}

bool Address::IsSet() { return !addr_.empty() && !port_.empty(); }

ssf::layer::LayerParameters Address::ToProxyParam() {
  return {{"http_host", addr_}, {"http_port", port_}};
}

ssf::layer::LayerParameters Address::ToTCPParam() {
  return {{"host", addr_}, {"port", port_}};
}

}  // tests
}  // ssf

ProxyTestFixture::ProxyTestFixture()
    : config_file_("./proxy/proxy.json"), config_options_() {}

ProxyTestFixture::~ProxyTestFixture() {}

void ProxyTestFixture::SetUp() { ParseConfigFile(config_file_); }

void ProxyTestFixture::TearDown() {}

bool ProxyTestFixture::Initialized() {
  return config_options_.count("target_host") > 0 &&
         config_options_.count("target_port") > 0 &&
         config_options_.count("proxy_host") > 0 &&
         config_options_.count("proxy_port") > 0;
}

std::string ProxyTestFixture::GetOption(const std::string& name) const {
  auto opt_it = config_options_.find(name);

  return opt_it != config_options_.end() ? opt_it->second : "";
}

ssf::layer::LayerParameters ProxyTestFixture::GetTcpParam() const {
  ssf::layer::LayerParameters tcp_params;
  tcp_params["addr"] = GetOption("target_host");
  tcp_params["port"] = GetOption("target_port");

  return tcp_params;
}

ssf::layer::LayerParameters ProxyTestFixture::GetProxyParam() const {
  ssf::layer::LayerParameters proxy_params;
  proxy_params["http_host"] = GetOption("proxy_host");
  proxy_params["http_port"] = GetOption("proxy_port");
  proxy_params["http_username"] = GetOption("username");
  proxy_params["http_domain"] = GetOption("domain");
  proxy_params["http_password"] = GetOption("password");
  proxy_params["http_reuse_ntlm"] = GetOption("reuse_ntlm");
  proxy_params["http_reuse_kerb"] = GetOption("reuse_kerb");

  return proxy_params;
}

bool ProxyTestFixture::ParseConfigFile(const std::string& filepath) {
  if (filepath.empty()) {
    return false;
  }

  std::ifstream file(filepath);
  if (!file.is_open()) {
    return false;
  }
  file.close();

  try {
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(filepath, pt);
    for (const auto& child : pt) {
      config_options_[child.first] = child.second.data();
    }
    return true;
  } catch (const std::exception&) {
    return false;
  }
}
