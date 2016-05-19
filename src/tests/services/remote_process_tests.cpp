#include "services/user_services/remote_process.h"

#include "tests/services/process_test_fixture.h"

class RemoteProcessTest
    : public ProcessFixtureTest<ssf::services::RemoteProcess> {};

TEST_F(RemoteProcessTest, ExecuteCmdTest) { ExecuteCmd(); }
