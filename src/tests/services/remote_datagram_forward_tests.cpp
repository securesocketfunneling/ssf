#include "services/user_services/udp_remote_port_forwarding.h"

#include "tests/services/datagram_fixture_test.h"

class RemoteUdpForwardTest
    : public DatagramFixtureTest<ssf::services::UdpRemotePortForwarding> {
  ssf::UserServiceParameters CreateUserServiceParameters(
      boost::system::error_code& ec) override {
    return {{ServiceTested::GetParseName(),
             {{{"from_addr", ""},
               {"from_port", "6464"},
               {"to_addr", "127.0.0.1"},
               {"to_port", "6565"}}}}};
  }
};

TEST_F(RemoteUdpForwardTest, transferOnesOverUdp) {
  ASSERT_TRUE(Wait());

  Run("6464", "6565");
}

class RemoteUdpForwardWildcardTest : public RemoteUdpForwardTest {
  void SetServerConfig(ssf::config::Config& config) override {
    const char* new_config = R"RAWSTRING(
{
    "ssf": {
        "services" : {
            "datagram_listener": { "gateway_ports": true }
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
            "datagram_listener": { "gateway_ports": true }
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
               {"from_port", "6666"},
               {"to_addr", "127.0.0.1"},
               {"to_port", "6767"}}}}};
  }
};

TEST_F(RemoteUdpForwardWildcardTest, transferOnesOverUdp) {
  ASSERT_TRUE(Wait());

  Run("6666", "6767");
}