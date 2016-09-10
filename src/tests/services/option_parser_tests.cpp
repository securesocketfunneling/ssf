#include <boost/system/error_code.hpp>

#include <gtest/gtest.h>

#include "services/user_services/option_parser.h"

TEST(OptionParserTests, BasicForwardTests) {
  using ssf::services::OptionParser;
  boost::system::error_code ec;

  auto result =
      OptionParser::ParseForwardOptions("1234:remote.example.com:5678", ec);
  ASSERT_EQ(result.from.addr, "") << "from addr is not empty";
  ASSERT_EQ(result.from.port, 1234) << "from port is incorrect";
  ASSERT_EQ(result.to.addr, "remote.example.com") << "to addr is incorrect";
  ASSERT_EQ(result.to.port, 5678) << "to port is incorrect";

  result = OptionParser::ParseForwardOptions(":127.0.0.1:5678", ec);
  ASSERT_NE(ec.value(), 0) << "Parsing should be in error";

  result = OptionParser::ParseForwardOptions("test:127.0.0.1:5678", ec);
  ASSERT_NE(ec.value(), 0) << "Parsing should be in error";

  result = OptionParser::ParseForwardOptions("65536:127.0.0.1:5678", ec);
  ASSERT_NE(ec.value(), 0) << "Parsing should be in error (port out of range)";
}

TEST(OptionParserTests, ExtendedForwardTests) {
  using ssf::services::OptionParser;
  boost::system::error_code ec;

  auto result =
      OptionParser::ParseForwardOptions("127.0.0.1:12345:10.0.0.1:56789", ec);
  ASSERT_EQ(result.from.addr, "127.0.0.1") << "from addr is incorrect";
  ASSERT_EQ(result.from.port, 12345) << "from port is incorrect";
  ASSERT_EQ(result.to.addr, "10.0.0.1") << "to addr is incorrect";
  ASSERT_EQ(result.to.port, 56789) << "to port is incorrect";

  result = OptionParser::ParseForwardOptions(":12345:10.0.0.1:56789", ec);
  ASSERT_EQ(result.from.addr, "*") << "from addr is incorrect";
  ASSERT_EQ(result.from.port, 12345) << "from port is incorrect";
  ASSERT_EQ(result.to.addr, "10.0.0.1") << "to addr is incorrect";
  ASSERT_EQ(result.to.port, 56789) << "to port is incorrect";

  result = OptionParser::ParseForwardOptions("::10.0.0.1:56789", ec);
  ASSERT_NE(ec.value(), 0) << "Parsing should be in error";

  result = OptionParser::ParseForwardOptions(":65536:10.0.0.1:56789", ec);
  ASSERT_NE(ec.value(), 0) << "Parsing should be in error (port out of range)";
}

TEST(OptionParserTests, BasicListeningTest) {
  using ssf::services::OptionParser;
  boost::system::error_code ec;

  auto result = OptionParser::ParseListeningOption("12345", ec);
  ASSERT_EQ(result.addr, "") << "addr is incorrect";
  ASSERT_EQ(result.port, 12345) << "port is incorrect";

  result = OptionParser::ParseListeningOption("test", ec);
  ASSERT_NE(ec.value(), 0) << "Parsing should be in error";

  result = OptionParser::ParseListeningOption("65536", ec);
  ASSERT_NE(ec.value(), 0) << "Parsing should be in error (port out of range)";
}

TEST(OptionParserTests, ExtendedListeningTest) {
  using ssf::services::OptionParser;
  boost::system::error_code ec;

  auto result = OptionParser::ParseListeningOption("127.0.0.1:12345", ec);
  ASSERT_EQ(result.addr, "127.0.0.1") << "addr is incorrect";
  ASSERT_EQ(result.port, 12345) << "port is incorrect";

  result = OptionParser::ParseListeningOption(":12345", ec);
  ASSERT_EQ(result.addr, "*") << "addr is incorrect";
  ASSERT_EQ(result.port, 12345) << "port is incorrect";

  result = OptionParser::ParseListeningOption(":test", ec);
  ASSERT_NE(ec.value(), 0) << "Parsing should be in error";

  result = OptionParser::ParseListeningOption(":65536", ec);
  ASSERT_NE(ec.value(), 0) << "Parsing should be in error (port out of range)";

  result = OptionParser::ParseListeningOption("10.0.0.1:65536", ec);
  ASSERT_NE(ec.value(), 0) << "Parsing should be in error (port out of range)";
}