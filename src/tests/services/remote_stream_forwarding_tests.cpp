#include "services/user_services/remote_port_forwarding.h"

#include "tests/services/stream_fixture_test.h"

class RemoteStreamForwardTest
    : public StreamFixtureTest<ssf::services::RemotePortForwarding> {
  ssf::UserServiceParameters CreateUserServiceParameters(
      boost::system::error_code& ec) override {
    return {{ServiceTested::GetParseName(),
             {{{"from_addr", ""},
               {"from_port", "5454"},
               {"to_addr", "127.0.0.1"},
               {"to_port", "5555"}}}}};
  }
};

TEST_F(RemoteStreamForwardTest, MultiStreams) {
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

  ssf::UserServiceParameters CreateUserServiceParameters(
      boost::system::error_code& ec) override {
    return {{ServiceTested::GetParseName(),
             {{{"from_addr", "*"},
               {"from_port", "5656"},
               {"to_addr", "127.0.0.1"},
               {"to_port", "5757"}}}}};
  }
};

TEST_F(RemoteStreamForwardWildcardTest, MultiStreams) {
  ASSERT_TRUE(Wait());

  Run("5656", "5757");
}
