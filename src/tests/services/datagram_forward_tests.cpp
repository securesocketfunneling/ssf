#include "services/user_services/udp_port_forwarding.h"

#include "tests/services/datagram_fixture_test.h"

class UdpForwardTest
    : public DatagramFixtureTest<ssf::services::UdpPortForwarding> {
  ssf::UserServiceParameters CreateUserServiceParameters(
      boost::system::error_code& ec) override {
    return {{ServiceTested::GetParseName(),
             {{{"from_addr", ""},
               {"from_port", "8484"},
               {"to_addr", "127.0.0.1"},
               {"to_port", "8585"}}}}};
  }
};

TEST_F(UdpForwardTest, MultiDatagrams) {
  ASSERT_TRUE(Wait());

  Run("8484", "8585");
}

class UdpForwardWildcardTest : public UdpForwardTest {
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
               {"from_port", "8686"},
               {"to_addr", "127.0.0.1"},
               {"to_port", "8787"}}}}};
  }
};

TEST_F(UdpForwardWildcardTest, MultiDatagrams) {
  ASSERT_TRUE(Wait());

  Run("8686", "8787");
}
