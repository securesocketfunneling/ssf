#include "services/user_services/remote_shell.h"

#include "tests/services/shell_fixture_test.h"

class RemoteShellTest
    : public ShellFixtureTest<ssf::services::RemoteShell> {
  ssf::UserServiceParameters CreateUserServiceParameters(
      boost::system::error_code& ec) override {
    return {
        {ServiceTested::GetParseName(), {{{"addr", ""}, {"port", "9081"}}}}};
  }
};

TEST_F(RemoteShellTest, ExecuteCmdTest) { ExecuteCmd("9081"); }

class RemoteShellWildcardTest : public RemoteShellTest {
 protected:
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
        {ServiceTested::GetParseName(), {{{"addr", ""}, {"port", "9082"}}}}};
  }
};

TEST_F(RemoteShellWildcardTest, ExecuteCmdTest) { ExecuteCmd("9082"); }
