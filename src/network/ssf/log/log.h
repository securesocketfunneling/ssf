#ifndef SSF_LOG_LOG_H_
#define SSF_LOG_LOG_H_

#include <spdlog/spdlog.h>

void SetLogLevel(spdlog::level::level_enum level = spdlog::level::info);

#ifdef SSF_DISABLE_LOGS
#define SSF_LOG(...)
#else

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace ssf {
namespace log {

class Manager {
 public:
  Manager();

  std::shared_ptr<spdlog::logger> GetChannel(const std::string& channel);
  void SetLevel(spdlog::level::level_enum level);

 private:
  std::shared_ptr<spdlog::logger> CreateChannel(const std::string& channel);
};

Manager& GetManager();

#define SSF_LOG(channel, level, ...) \
  ssf::log::GetManager().GetChannel(channel)->level(__VA_ARGS__);

}  // log
}  // ssf

#endif  // SSF_DISABLE_LOGS

#endif  // SSF_LOG_LOG_H_
