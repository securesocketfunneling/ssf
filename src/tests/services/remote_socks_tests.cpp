#include "tests/services/socks_fixture_test.h"

#include "services/user_services/remote_socks.h"

class RemoteSocksTest : public SocksFixtureTest<ssf::services::RemoteSocks> {
  std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions(":9061", ec);
  }
};

TEST_F(RemoteSocksTest, startStopTransmitSSFRemoteSocks) {
  ASSERT_TRUE(Wait());

  Run("9061", "9062");
}

class RemoteSocksWildcardTest : public RemoteSocksTest {
  void SetServerConfig(ssf::config::Config& config) override {
    const char* new_config = R"RAWSTRING(
{
    "ssf": {
        "services" : {
            "stream_listener": { "gateway_ports": true }
        }
    }
}
)RAWSTRING";

    boost::system::error_code ec;
    config.UpdateFromString(new_config, ec);
    ASSERT_EQ(ec.value(), 0) << "Could not update server config from string "
                             << new_config;
  }

  void SetClientConfig(ssf::config::Config& config) override {
    const char* new_config = R"RAWSTRING(
{
    "ssf": {
        "services" : {
            "stream_listener": { "gateway_ports": true }
        }
    }
}
)RAWSTRING";

    boost::system::error_code ec;
    config.UpdateFromString(new_config, ec);
    ASSERT_EQ(ec.value(), 0) << "Could not update client config from string "
                             << new_config;
  }

  std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions(":9063", ec);
  }
};

TEST_F(RemoteSocksWildcardTest, startStopTransmitSSFSocks) {
  ASSERT_TRUE(Wait());

  Run("9063", "9064");
}
