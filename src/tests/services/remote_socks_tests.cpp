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
    return ServiceTested::CreateServiceOptions(":9151", ec);
  }
};

class RemoteSocks4aTest
    : public SocksFixtureTest<ssf::services::RemoteSocks,
                              tests::socks::Socks4DummyClient> {
  std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions(":9153", ec);
  }
};

class RemoteSocks4WildcardTest
    : public RemoteSocksWildcardTest<RemoteSocks4Test> {
  std::shared_ptr<typename RemoteSocks4Test::ServiceTested>
  ServiceCreateServiceOptions(boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions(":9155", ec);
  }
};

TEST_F(RemoteSocks4Test, Socks4) {
  ASSERT_TRUE(Wait());
  Run("9151", "127.0.0.1", "9152");
}

TEST_F(RemoteSocks4aTest, Socks4a) {
  ASSERT_TRUE(Wait());
  Run("9153", "localhost", "9154");
}

TEST_F(RemoteSocks4WildcardTest, Socks4) {
  ASSERT_TRUE(Wait());
  Run("9155", "127.0.0.1", "9156");
}

class RemoteSocks5Test
    : public SocksFixtureTest<ssf::services::RemoteSocks,
                              tests::socks::Socks5DummyClient> {
  std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions(":9157", ec);
  }
};

class RemoteSocks5hTest
    : public SocksFixtureTest<ssf::services::RemoteSocks,
                              tests::socks::Socks5DummyClient> {
  std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions(":9159", ec);
  }
};

class RemoteSocks5WildcardTest
    : public RemoteSocksWildcardTest<RemoteSocks5Test> {
  std::shared_ptr<typename RemoteSocks4Test::ServiceTested>
  ServiceCreateServiceOptions(boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions(":9161", ec);
  }
};

TEST_F(RemoteSocks5Test, Socks5) {
  ASSERT_TRUE(Wait());
  Run("9157", "127.0.0.1", "9158");
}

TEST_F(RemoteSocks5hTest, Socks5h) {
  ASSERT_TRUE(Wait());
  Run("9159", "localhost", "9160");
}

TEST_F(RemoteSocks5WildcardTest, Socks5) {
  ASSERT_TRUE(Wait());
  Run("9161", "127.0.0.1", "9162");
}