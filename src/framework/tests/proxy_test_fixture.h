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

bool ParseConfigFile(const std::string& filepath,
                     std::vector<Address>& addresses);

}  // tests
}  // ssf

class ProxyTestFixture : public ::testing::Test {
 protected:
  ProxyTestFixture();

  virtual ~ProxyTestFixture();

  virtual void SetUp();

  virtual void TearDown();

  bool Initialized();

 protected:
  std::string config_file_;
  ssf::tests::Address client_address_;
  ssf::tests::Address proxy_address_;
};

#endif  // SSF_TESTS_PROXY_TEST_FIXTURE_H_
