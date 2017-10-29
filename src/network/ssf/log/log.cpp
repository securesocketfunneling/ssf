#include "ssf/log/log.h"

#ifdef SSF_DISABLE_LOGS

void SetLogLevel(spdlog::level::level_enum level) {}

#else

#include <spdlog/spdlog.h>

#include "spdlog/sinks/stdout_sinks.h"

#if defined(_MSC_VER)
#include "spdlog/sinks/msvc_sink.h"
#elif defined(__unix__)
#ifdef SSF_ENABLE_SYSLOG
#define SPDLOG_ENABLE_SYSLOG
#include "spdlog/sinks/syslog_sink.h"
#endif
#endif  // defined(_MSC_VER)

void SetLogLevel(spdlog::level::level_enum level) {
  ssf::log::GetManager()->SetLevel(level);
}

namespace ssf {
namespace log {

Manager* GetManager() {
  static Manager* manager = NULL;
  if (!manager) {
    manager = new Manager();
  }

  return manager;
}

Manager::Manager()
    : channel_mutex_(), level_(::spdlog::level::info), channels_() {}

std::shared_ptr<spdlog::logger> Manager::GetChannel(const std::string& name) {
  std::lock_guard<std::mutex> lock(channel_mutex_);
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
  // update channel levels
  for (const auto& channel : channels_) {
    GetChannel(channel)->set_level(level_);
  }
}

std::shared_ptr<spdlog::logger> Manager::CreateChannel(
    const std::string& name) {
  std::vector<spdlog::sink_ptr> sinks;

#if defined(_MSC_VER)
  sinks.push_back(std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>());
  sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#elif defined(__unix__)
  sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
#if defined(SSF_ENABLE_SYSLOG)
  sinks.push_back(std::make_shared<spdlog::sinks::syslog_sink>());
#endif  // defined(SSF_ENABLE_SYSLOG)
#endif  // defined(_MSC_VER)

  auto logger = std::make_shared<spdlog::logger>(name, std::begin(sinks),
                                                 std::end(sinks));
  logger->set_level(level_);
  spdlog::register_logger(logger);

  channels_.push_back(name);

  return logger;
}

}  // log
}  // ssf

#endif  // SSF_DISABLE_LOGS
