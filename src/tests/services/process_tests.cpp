#include "services/user_services/process.h"

#include "tests/services/process_fixture_test.h"

class ProcessTest : public ProcessFixtureTest<ssf::services::Process> {};

TEST_F(ProcessTest, ExecuteCmdTest) { ExecuteCmd(); }
