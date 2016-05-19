#include "services/user_services/process.h"

#include "tests/services/process_test_fixture.h"

class ProcessTest : public ProcessFixtureTest<ssf::services::Process> {};

TEST_F(ProcessTest, ExecuteCmdTest) { ExecuteCmd(); }
