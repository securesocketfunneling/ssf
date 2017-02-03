#include "services/user_services/udp_port_forwarding.h"

#include "tests/services/datagram_fixture_test.h"

class UdpForwardTest
    : public DatagramFixtureTest<ssf::services::UdpPortForwarding> {
  std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions("8484:127.0.0.1:8585", ec);
  }
};

TEST_F(UdpForwardTest, transferOnesOverUdp) {
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

  std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions(":8686:127.0.0.1:8787", ec);
  }
};

TEST_F(UdpForwardWildcardTest, transferOnesOverStream) {
  ASSERT_TRUE(Wait());

  Run("8686", "8787");
}
