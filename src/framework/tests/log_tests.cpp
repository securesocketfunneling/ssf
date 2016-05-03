#include <gtest/gtest.h>

#include "ssf/log/log.h"

TEST(LogTests, DefaultLog) {
  SSF_LOG(kLogCritical) << "critical";
  SSF_LOG(kLogError) << "error";
  SSF_LOG(kLogWarning) << "warning";
  SSF_LOG(kLogInfo) << "info";
  SSF_LOG(kLogDebug) << "debug";
  SSF_LOG(kLogTrace) << "trace";
}

TEST(LogTests, TraceLog) {
  ssf::log::Log::SetSeverityLevel(ssf::log::LogLevel::kLogTrace);
  SSF_LOG(kLogCritical) << "critical";
  SSF_LOG(kLogError) << "error";
  SSF_LOG(kLogWarning) << "warning";
  SSF_LOG(kLogInfo) << "info";
  SSF_LOG(kLogDebug) << "debug";
  SSF_LOG(kLogTrace) << "trace";
}

TEST(LogTests, CriticalLog) {
  ssf::log::Log::SetSeverityLevel(ssf::log::LogLevel::kLogCritical);
  SSF_LOG(kLogCritical) << "critical";
  SSF_LOG(kLogError) << "error";
  SSF_LOG(kLogWarning) << "warning";
  SSF_LOG(kLogInfo) << "info";
  SSF_LOG(kLogDebug) << "debug";
  SSF_LOG(kLogTrace) << "trace";
}

TEST(LogTests, NoLog) {
  ssf::log::Log::SetSeverityLevel(ssf::log::LogLevel::kLogNone);
  SSF_LOG(kLogCritical) << "critical";
  SSF_LOG(kLogError) << "error";
  SSF_LOG(kLogWarning) << "warning";
  SSF_LOG(kLogInfo) << "info";
  SSF_LOG(kLogDebug) << "debug";
  SSF_LOG(kLogTrace) << "trace";
}