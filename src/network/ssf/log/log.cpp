#include "ssf/log/log.h"

#ifdef SSF_DISABLE_LOGS

void SetLogLevel(spdlog::level::level_enum level) {}

#else

#if defined(_MSC_VER)
#include "spdlog/sinks/msvc_sink.h"
#elif defined(__unix__) || defined(__APPLE__)
#include "spdlog/sinks/ansicolor_sink.h"
#ifdef SSF_ENABLE_SYSLOG
#define SPDLOG_ENABLE_SYSLOG
#include "spdlog/sinks/syslog_sink.h"
#endif
#endif  // defined(_MSC_VER)

void SetLogLevel(spdlog::level::level_enum level) {
  ssf::log::GetManager().SetLevel(level);
}

namespace ssf {
namespace log {

Manager& GetManager() {
  static Manager manager;
  return manager;
}

Manager::Manager() {
  spdlog::set_level(spdlog::level::info);
  spdlog::set_pattern("[%Y-%m-%dT%H:%M:%S%z] [%l] [%n] %v");
}

std::shared_ptr<spdlog::logger> Manager::GetChannel(const std::string& name) {
  auto channel = spdlog::get(name);
  if (!channel) {
    channel = CreateChannel(name);
  }
  return channel;
}

void Manager::SetLevel(spdlog::level::level_enum level) {
  spdlog::set_level(level);
}

std::shared_ptr<spdlog::logger> Manager::CreateChannel(
    const std::string& name) {
  std::vector<spdlog::sink_ptr> sinks;

#if defined(_MSC_VER)
  sinks.push_back(std::make_shared<spdlog::sinks::wincolor_stderr_sink_mt>());
  sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#elif defined(__unix__) || defined(__APPLE__)
  sinks.push_back(std::make_shared<spdlog::sinks::ansicolor_stderr_sink_mt>());
#if defined(SSF_ENABLE_SYSLOG)
  sinks.push_back(std::make_shared<spdlog::sinks::syslog_sink>());
#endif  // defined(SSF_ENABLE_SYSLOG)
#endif  // defined(_MSC_VER)

  std::shared_ptr<spdlog::logger> logger;
  try {
    logger = spdlog::create(name, sinks.cbegin(), sinks.cend());
  } catch (const std::exception&) {
    logger = spdlog::get(name);
  }

  return logger;
}

}  // log
}  // ssf

#endif  // SSF_DISABLE_LOGS
