#include <ctime>
#include <cstdio>

#include "ssf/log/log.h"

namespace ssf {
namespace log {

Log::Log() : os_() {}

Log::~Log() {
  os_ << std::endl;
  fprintf(stderr, "%s", os_.str().c_str());
  fflush(stderr);
}

std::ostream& Log::GetLog(LogLevel level) {
  // generate timestamp string
  char time_str[32];
  time_t current_time;
  struct tm* timeinfo;
  time(&current_time);
  timeinfo = localtime(&current_time);
  strftime(time_str, sizeof(time_str), "[%Y-%m-%d %H:%M:%S]", timeinfo);

  os_ << time_str;
  os_ << "[" << LevelToString(level) << "] ";
  return os_;
}

LogLevel& Log::SeverityLevel() {
  static LogLevel level = kLogInfo;
  return level;
}

void Log::SetSeverityLevel(LogLevel level) { Log::SeverityLevel() = level; }

std::string Log::LevelToString(LogLevel level) {
  switch (level) {
    case kLogCritical:
      return "critical";
    case kLogError:
      return "error";
    case kLogWarning:
      return "warning";
    case kLogInfo:
      return "info";
    case kLogDebug:
      return "debug";
    case kLogTrace:
      return "trace";
    default:
      return "unknown";
  }
}

}  // log
}  // ssf
