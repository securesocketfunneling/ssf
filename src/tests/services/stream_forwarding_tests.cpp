#include "services/user_services/port_forwarding.h"

#include "tests/services/stream_fixture_test.h"

class StreamForwardTest
    : public StreamFixtureTest<ssf::services::PortForwarding> {
  ssf::UserServiceParameters CreateUserServiceParameters(
      boost::system::error_code& ec) override {
    return {{ServiceTested::GetParseName(),
             {{{"from_addr", ""},
               {"from_port", "7474"},
               {"to_addr", "127.0.0.1"},
               {"to_port", "7575"}}}}};
  }
};

TEST_F(StreamForwardTest, transferOnesOverStream) {
  ASSERT_TRUE(Wait());

  Run("7474", "7575");
}

class StreamForwardWildcardTest : public StreamForwardTest {
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
               {"from_port", "7676"},
               {"to_addr", "127.0.0.1"},
               {"to_port", "7777"}}}}};
  }
};

TEST_F(StreamForwardWildcardTest, transferOnesOverStream) {
  ASSERT_TRUE(Wait());

  Run("7676", "7777");
}