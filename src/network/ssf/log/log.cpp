#include "ssf/log/log.h"

#ifdef SSF_DISABLE_LOGS

void SetLogLevel(spdlog::level::level_enum level) {}

#else

#if defined(_MSC_VER)
#include "spdlog/sinks/msvc_sink.h"
#elif defined(__unix__)
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

Manager::Manager() : level_(::spdlog::level::info) {}

std::shared_ptr<spdlog::logger> Manager::GetChannel(const std::string& name) {
  auto channel = spdlog::get(name);
  if (!channel) {
    channel = CreateChannel(name);
  }
  return channel;
}

void Manager::SetLevel(spdlog::level::level_enum level) {
  if (level_ == level) {
    return;
  }

  level_ = level;
  auto set_level = [this](std::shared_ptr<spdlog::logger> logger) {
    logger->set_level(level_);
  };
  spdlog::apply_all(set_level);
}

std::shared_ptr<spdlog::logger> Manager::CreateChannel(
    const std::string& name) {
  std::vector<spdlog::sink_ptr> sinks;

#if defined(_MSC_VER)
  sinks.push_back(std::make_shared<spdlog::sinks::wincolor_stderr_sink_mt>());
  sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#elif defined(__unix__)
  sinks.push_back(std::make_shared<spdlog::sinks::ansicolor_stderr_sink_mt>());
#if defined(SSF_ENABLE_SYSLOG)
  sinks.push_back(std::make_shared<spdlog::sinks::syslog_sink>());
#endif  // defined(SSF_ENABLE_SYSLOG)
#endif  // defined(_MSC_VER)

  auto logger = std::make_shared<spdlog::logger>(name, std::begin(sinks),
                                                 std::end(sinks));
  logger->set_level(level_);

  try {
    spdlog::register_logger(logger);
  } catch (const std::exception& e) {
    logger = spdlog::get(name);
  }

  return logger;
}

}  // log
}  // ssf

#endif  // SSF_DISABLE_LOGS
