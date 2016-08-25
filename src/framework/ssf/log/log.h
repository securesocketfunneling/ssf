#ifndef SSF_LOG_LOG_H_
#define SSF_LOG_LOG_H_

#include <string>
#include <sstream>

namespace ssf {
namespace log {

enum LogLevel {
  kLogNone,
  kLogCritical,
  kLogError,
  kLogWarning,
  kLogInfo,
  kLogDebug,
  kLogTrace
};

// Application log
// Example:
//     ssf::log::Log::SetSeverityLevel(ssf::log::LogLevel::kLogTrace);
//     SSF_LOG(kLogCritical) << "critical";
//     SSF_LOG(kLogError) << "error";
//     SSF_LOG(kLogWarning) << "warning";
//     SSF_LOG(kLogInfo) << "info";
//     SSF_LOG(kLogDebug) << "debug";
//     SSF_LOG(kLogTrace) << "trace";
class Log {
 public:
  Log();
  virtual ~Log();

  std::ostream& GetLog(LogLevel level);

 public:
  // Global severity level
  static LogLevel& SeverityLevel();

  // Set global severity level
  static void SetSeverityLevel(LogLevel level);

  // Convert log level to string representation
  static std::string LevelToString(LogLevel level);

 protected:
  std::ostringstream os_;

 private:
  Log(const Log&) = delete;
  Log& operator=(const Log&) = delete;
};

}  // log
}  // ssf

#define SSF_LOG(level)                                  \
  if (ssf::log::Log::SeverityLevel() < ssf::log::level) \
    ;                                                   \
  else                                                  \
  ssf::log::Log().GetLog(ssf::log::level)

#endif  // SSF_LOG_LOG_H_
