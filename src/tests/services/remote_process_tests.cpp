#include "services/user_services/remote_process.h"

#include "tests/services/process_fixture_test.h"

class RemoteProcessTest
    : public ProcessFixtureTest<ssf::services::RemoteProcess> {
  std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions("9091", ec);
  }
};

TEST_F(RemoteProcessTest, ExecuteCmdTest) { ExecuteCmd("9091"); }

class RemoteProcessWildcardTest : public RemoteProcessTest {
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

  std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) override {
    return ServiceTested::CreateServiceOptions(":9092", ec);
  }
};

TEST_F(RemoteProcessWildcardTest, ExecuteCmdTest) { ExecuteCmd("9092"); }
