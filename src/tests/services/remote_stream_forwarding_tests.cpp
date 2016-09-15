#include "services/user_services/remote_port_forwarding.h"

#include "tests/services/stream_fixture_test.h"

class RemoteStreamForwardTest
    : public StreamFixtureTest<ssf::services::RemotePortForwarding> {
  std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions("5454:127.0.0.1:5555", ec);
  }
};

TEST_F(RemoteStreamForwardTest, transferOnesOverStream) {
  ASSERT_TRUE(Wait());

  Run("5454", "5555");
}

class RemoteStreamForwardWildcardTest : public RemoteStreamForwardTest {
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
    return ServiceTested::CreateServiceOptions(":5656:127.0.0.1:5757", ec);
  }
};

TEST_F(RemoteStreamForwardWildcardTest, transferOnesOverStream) {
  ASSERT_TRUE(Wait());

  Run("5656", "5757");
}
