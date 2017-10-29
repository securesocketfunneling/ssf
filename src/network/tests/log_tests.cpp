#include <gtest/gtest.h>

#include "ssf/log/log.h"

TEST(LogTests, DefaultLog) {
  SSF_LOG("test", critical, "critical");
  SSF_LOG("test", error, "error");
  SSF_LOG("test", warn, "warning");
  SSF_LOG("test", info, "info");
  SSF_LOG("test", debug, "debug");
  SSF_LOG("test", trace, "trace");
}

TEST(LogTests, TraceLog) {
  SetLogLevel(spdlog::level::trace);
  SSF_LOG("test", critical, "critical");
  SSF_LOG("test", error, "error");
  SSF_LOG("test", warn, "warning");
  SSF_LOG("test", info, "info");
  SSF_LOG("test", debug, "debug");
  SSF_LOG("test", trace, "trace");
}

TEST(LogTests, CriticalLog) {
  SetLogLevel(spdlog::level::critical);
  SSF_LOG("test", critical, "critical");
  SSF_LOG("test", error, "error");
  SSF_LOG("test", warn, "warning");
  SSF_LOG("test", info, "info");
  SSF_LOG("test", debug, "debug");
  SSF_LOG("test", trace, "trace");
}

TEST(LogTests, NoLog) {
  SetLogLevel(spdlog::level::off);
  SSF_LOG("test", critical, "critical");
  SSF_LOG("test", error, "error");
  SSF_LOG("test", warn, "warning");
  SSF_LOG("test", info, "info");
  SSF_LOG("test", debug, "debug");
  SSF_LOG("test", trace, "trace");
}