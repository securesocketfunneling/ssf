#include "tests/services/socks_fixture_test.h"

#include "services/user_services/remote_socks.h"

template <class RemoteSocksTest>
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
};

class RemoteSocks4Test
    : public SocksFixtureTest<ssf::services::RemoteSocks,
                              tests::socks::Socks4DummyClient> {
  std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions(":9061", ec);
  }
};

class RemoteSocks4WildcardTest
    : public RemoteSocksWildcardTest<RemoteSocks4Test> {
  std::shared_ptr<typename RemoteSocks4Test::ServiceTested>
  ServiceCreateServiceOptions(boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions(":9063", ec);
  }
};

TEST_F(RemoteSocks4Test, startStopTransmitSSFRemoteSocks) {
  ASSERT_TRUE(Wait());

  Run("9061", "9062");
}

TEST_F(RemoteSocks4WildcardTest, startStopTransmitSSFSocks) {
  ASSERT_TRUE(Wait());

  Run("9063", "9064");
}

class RemoteSocks5Test
    : public SocksFixtureTest<ssf::services::RemoteSocks,
                              tests::socks::Socks5DummyClient> {
  std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions(":9065", ec);
  }
};

class RemoteSocks5WildcardTest
    : public RemoteSocksWildcardTest<RemoteSocks5Test> {
  std::shared_ptr<typename RemoteSocks4Test::ServiceTested>
  ServiceCreateServiceOptions(boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions(":9067", ec);
  }
};

TEST_F(RemoteSocks5Test, startStopTransmitSSFRemoteSocks) {
  ASSERT_TRUE(Wait());

  Run("9065", "9066");
}

TEST_F(RemoteSocks5WildcardTest, startStopTransmitSSFSocks) {
  ASSERT_TRUE(Wait());

  Run("9067", "9068");
}