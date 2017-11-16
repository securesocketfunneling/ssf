#include "services/user_services/shell.h"

#include "tests/services/shell_fixture_test.h"

class ShellTest : public ShellFixtureTest<ssf::services::Shell> {
  ssf::UserServiceParameters CreateUserServiceParameters(
      boost::system::error_code& ec) override {
    return {
        {ServiceTested::GetParseName(), {{{"addr", ""}, {"port", "9071"}}}}};
  }
};

TEST_F(ShellTest, ExecuteCmdTest) { ExecuteCmd("9071"); }

class ShellWildcardTest : public ShellTest {
  void SetServerConfig(ssf::config::Config& config) override {
    const char* new_config = R"RAWSTRING(
{
    "ssf": {
        "services" : {
            "shell": { "enable": true },
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
            "shell": { "enable": true },
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
    return {
        {ServiceTested::GetParseName(), {{{"addr", "*"}, {"port", "9072"}}}}};
  }
};

TEST_F(ShellWildcardTest, ExecuteCmdTest) { ExecuteCmd("9072"); }
