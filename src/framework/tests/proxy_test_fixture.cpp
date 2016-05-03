#include <fstream>

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

bool Address::IsSet() { return addr_ != "" && port_ != ""; }

ssf::layer::LayerParameters Address::ToProxyParam() {
  return {{"http_addr", addr_}, {"http_port", port_}};
}

ssf::layer::LayerParameters Address::ToTCPParam() {
  return {{"addr", addr_}, {"port", port_}};
}

bool ParseConfigFile(const std::string& filepath,
                     std::vector<Address>& addresses) {
  if (filepath == "") {
    return false;
  }
  std::ifstream file(filepath);

  if (!file.is_open()) {
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    size_t position = line.find(":");
    if (position == std::string::npos) {
      return false;
    }
    addresses.emplace_back(line.substr(0, position), line.substr(position + 1));
  }

  file.close();
  return true;
}

}  // tests
}  // ssf

ProxyTestFixture::ProxyTestFixture()
    : config_file_("./proxy/proxy.test"), client_address_(), proxy_address_() {}

ProxyTestFixture::~ProxyTestFixture() {}

void ProxyTestFixture::SetUp() {
  std::vector<ssf::tests::Address> test_addresses;
  if (!ssf::tests::ParseConfigFile(config_file_, test_addresses) ||
      test_addresses.size() < 2) {
    return;
  }
  client_address_ = test_addresses[0];
  proxy_address_ = test_addresses[1];
}

void ProxyTestFixture::TearDown() {}

bool ProxyTestFixture::Initialized() {
  return client_address_.IsSet() && proxy_address_.IsSet();
}
