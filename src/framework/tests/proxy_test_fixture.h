#ifndef SSF_TESTS_PROXY_TEST_FIXTURE_H_
#define SSF_TESTS_PROXY_TEST_FIXTURE_H_

#include <string>

#include <gtest/gtest.h>

#include "ssf/layer/parameters.h"

namespace ssf {
namespace tests {

class Address {
 public:
  Address();
  Address(const std::string& addr, const std::string& port);
  Address(const Address& address);
  Address& operator=(const Address& address);

  ssf::layer::LayerParameters ToProxyParam();

  ssf::layer::LayerParameters ToTCPParam();

  bool IsSet();

 private:
  std::string addr_;
  std::string port_;
};

}  // tests
}  // ssf

class ProxyTestFixture : public ::testing::Test {
 protected:
  ProxyTestFixture();

  virtual ~ProxyTestFixture();

  virtual void SetUp();

  virtual void TearDown();

  bool Initialized();

  ssf::layer::LayerParameters GetProxyTcpParam() const;

  ssf::layer::LayerParameters GetLocalTcpParam() const;

  ssf::layer::LayerParameters GetProxyParam() const;

 private:
  bool ParseConfigFile(const std::string& filepath);
  std::string GetOption(const std::string& name) const;

 protected:
  std::string config_file_;
  std::map<std::string, std::string> config_options_;
};

#endif  // SSF_TESTS_PROXY_TEST_FIXTURE_H_
