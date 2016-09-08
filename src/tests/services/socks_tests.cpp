#include "tests/services/socks_fixture_test.h"

#include "services/user_services/socks.h"

class SocksTest : public SocksFixtureTest<ssf::services::Socks> {
  std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions(":9091", ec);
  }
};

TEST_F(SocksTest, startStopTransmitSSFSocks) {
  ASSERT_TRUE(Wait());
  Run("9091", "9092");
}

class SocksWildcardTest : public SocksTest {
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
    return ServiceTested::CreateServiceOptions(":9093", ec);
  }
};

TEST_F(SocksWildcardTest, startStopTransmitSSFSocks) {
  ASSERT_TRUE(Wait());

  Run("9093", "9094");
}
