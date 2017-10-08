#include "services/user_services/process.h"

#include "tests/services/process_fixture_test.h"

class ProcessTest : public ProcessFixtureTest<ssf::services::Process> {
  ssf::UserServiceParameters CreateUserServiceParameters(
      boost::system::error_code& ec) override {
    return {
        {ServiceTested::GetParseName(), {{{"addr", ""}, {"port", "9071"}}}}};
  }
};

TEST_F(ProcessTest, ExecuteCmdTest) { ExecuteCmd("9071"); }

class ProcessWildcardTest : public ProcessTest {
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

TEST_F(ProcessWildcardTest, ExecuteCmdTest) { ExecuteCmd("9072"); }