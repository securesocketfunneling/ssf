#include <vector>

#include <gtest/gtest.h>

#include <boost/system/error_code.hpp>

#include "core/command_line/standard/command_line.h"
#include "core/command_line/copy/command_line.h"

TEST(StandardCommandLineTests, ServerTest) {
  ssf::command_line::StandardCommandLine cmd(true);

  ASSERT_FALSE(cmd.host_set());
  ASSERT_EQ("", cmd.host());
  ASSERT_FALSE(cmd.port_set());
  ASSERT_EQ(0, cmd.port());
  ASSERT_TRUE(cmd.config_file().empty());
  ASSERT_EQ(ssf::log::kLogInfo, cmd.log_level());

  ASSERT_FALSE(cmd.show_status());
  ASSERT_FALSE(cmd.relay_only());
  ASSERT_FALSE(cmd.gateway_ports());

  boost::system::error_code ec;

  std::vector<const char*> argv = {"test_exec", "-p", "8012", "-c",
                             "config_file.json", "-v", "critical", "-S", "-R",
                             "-g", "127.0.0.1"};

  cmd.Parse(static_cast<int>(argv.size()), const_cast<char**>(argv.data()), ec);

  ASSERT_EQ(0, ec.value()) << "Parsing failed";
  ASSERT_TRUE(cmd.host_set());
  ASSERT_EQ("127.0.0.1", cmd.host());
  ASSERT_TRUE(cmd.port_set());
  ASSERT_EQ(8012, cmd.port());
  ASSERT_FALSE(cmd.config_file().empty());
  ASSERT_EQ(cmd.config_file(), "config_file.json");
  ASSERT_EQ(ssf::log::kLogCritical, cmd.log_level());

  ASSERT_TRUE(cmd.show_status());
  ASSERT_TRUE(cmd.relay_only());
  ASSERT_TRUE(cmd.gateway_ports());
}

TEST(StandardCommandLineTests, ClientTest) {
  ssf::command_line::StandardCommandLine cmd(false);

  ASSERT_FALSE(cmd.host_set());
  ASSERT_EQ("", cmd.host());
  ASSERT_FALSE(cmd.port_set());
  ASSERT_EQ(0, cmd.port());
  ASSERT_TRUE(cmd.config_file().empty());
  ASSERT_EQ(ssf::log::kLogInfo, cmd.log_level());

  ASSERT_FALSE(cmd.show_status());
  ASSERT_FALSE(cmd.relay_only());
  ASSERT_FALSE(cmd.gateway_ports());

  boost::system::error_code ec;

  std::vector<const char*> argv = {"test_exec", "-p", "8012", "-c",
                             "config_file.json", "-v", "critical", "-S", "-g",
                             "127.0.0.1"};

  cmd.Parse(static_cast<int>(argv.size()), const_cast<char**>(argv.data()), ec);

  ASSERT_EQ(0, ec.value()) << "Parsing failed";
  ASSERT_TRUE(cmd.host_set());
  ASSERT_EQ("127.0.0.1", cmd.host());
  ASSERT_TRUE(cmd.port_set());
  ASSERT_EQ(8012, cmd.port());
  ASSERT_FALSE(cmd.config_file().empty());
  ASSERT_EQ(cmd.config_file(), "config_file.json");
  ASSERT_EQ(ssf::log::kLogCritical, cmd.log_level());

  ASSERT_TRUE(cmd.show_status());
  ASSERT_FALSE(cmd.relay_only());
  ASSERT_TRUE(cmd.gateway_ports());
}

TEST(CopyCommandLineTests, FromStdinToServerTest) {
  ssf::command_line::CopyCommandLine cmd;

  ASSERT_FALSE(cmd.host_set());
  ASSERT_EQ("", cmd.host());
  ASSERT_FALSE(cmd.port_set());
  ASSERT_EQ(0, cmd.port());
  ASSERT_TRUE(cmd.config_file().empty());
  ASSERT_EQ(ssf::log::kLogInfo, cmd.log_level());

  boost::system::error_code ec;

  std::vector<const char*> argv = {"test_exec", "-p", "8012", "-c",
                             "config_file.json", "-v", "critical", "-t",
                             "127.0.0.1@/tmp/test_out/output"};

  cmd.Parse(static_cast<int>(argv.size()), const_cast<char**>(argv.data()), ec);

  ASSERT_EQ(0, ec.value()) << "Parsing failed";
  ASSERT_TRUE(cmd.host_set());
  ASSERT_EQ("127.0.0.1", cmd.host());
  ASSERT_TRUE(cmd.port_set());
  ASSERT_EQ(8012, cmd.port());
  ASSERT_FALSE(cmd.config_file().empty());
  ASSERT_EQ(cmd.config_file(), "config_file.json");
  ASSERT_EQ(ssf::log::kLogCritical, cmd.log_level());

  ASSERT_TRUE(cmd.from_stdin());
  ASSERT_TRUE(cmd.from_local_to_remote());
  ASSERT_EQ("/tmp/test_out/output", cmd.output_pattern());
}

TEST(CopyCommandLineTests, ClientToServerTest) {
  ssf::command_line::CopyCommandLine cmd;

  ASSERT_FALSE(cmd.host_set());
  ASSERT_EQ("", cmd.host());
  ASSERT_FALSE(cmd.port_set());
  ASSERT_EQ(0, cmd.port());
  ASSERT_TRUE(cmd.config_file().empty());
  ASSERT_EQ(ssf::log::kLogInfo, cmd.log_level());

  boost::system::error_code ec;

  std::vector<const char*> argv = {"test_exec", "-p", "8012", "-c",
                             "config_file.json", "-v", "critical",
                             "/tmp/test_in", "127.0.0.1@/tmp/test_out"};

  cmd.Parse(static_cast<int>(argv.size()), const_cast<char**>(argv.data()), ec);

  ASSERT_EQ(0, ec.value()) << "Parsing failed";
  ASSERT_TRUE(cmd.host_set());
  ASSERT_EQ("127.0.0.1", cmd.host());
  ASSERT_TRUE(cmd.port_set());
  ASSERT_EQ(8012, cmd.port());
  ASSERT_FALSE(cmd.config_file().empty());
  ASSERT_EQ(cmd.config_file(), "config_file.json");
  ASSERT_EQ(ssf::log::kLogCritical, cmd.log_level());

  ASSERT_FALSE(cmd.from_stdin());
  ASSERT_TRUE(cmd.from_local_to_remote());
  ASSERT_EQ("/tmp/test_in", cmd.input_pattern());
  ASSERT_EQ("/tmp/test_out/", cmd.output_pattern());
}

TEST(CopyCommandLineTests, ServerToClientTest) {
  ssf::command_line::CopyCommandLine cmd;

  ASSERT_FALSE(cmd.host_set());
  ASSERT_EQ(cmd.host(), "");
  ASSERT_FALSE(cmd.port_set());
  ASSERT_EQ(cmd.port(), 0);
  ASSERT_TRUE(cmd.config_file().empty());
  ASSERT_EQ(cmd.log_level(), ssf::log::kLogInfo);

  boost::system::error_code ec;

  std::vector<const char*> argv = {"test_exec", "-p", "8012", "-c",
                             "config_file.json", "-v", "critical",
                             "127.0.0.1@/tmp/test_in", "/tmp/test_out"};

  cmd.Parse(static_cast<int>(argv.size()), const_cast<char**>(argv.data()), ec);

  ASSERT_EQ(ec.value(), 0) << "Parsing failed";
  ASSERT_TRUE(cmd.host_set());
  ASSERT_EQ(cmd.host(), "127.0.0.1");
  ASSERT_TRUE(cmd.port_set());
  ASSERT_EQ(cmd.port(), 8012);
  ASSERT_FALSE(cmd.config_file().empty());
  ASSERT_EQ(cmd.config_file(), "config_file.json");
  ASSERT_EQ(cmd.log_level(), ssf::log::kLogCritical);

  ASSERT_FALSE(cmd.from_stdin());
  ASSERT_FALSE(cmd.from_local_to_remote());
  ASSERT_EQ("/tmp/test_in", cmd.input_pattern());
  ASSERT_EQ("/tmp/test_out/", cmd.output_pattern());
}
