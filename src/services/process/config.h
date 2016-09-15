#ifndef SSF_SERVICES_PROCESS_CONFIG_H_
#define SSF_SERVICES_PROCESS_CONFIG_H_

#include <string>

#include "services/base_service_config.h"

namespace ssf {
namespace services {
namespace process {

class Config : public BaseServiceConfig {
 public:
  Config();
  Config(const Config& process_service);

  inline std::string path() const { return path_; }
  inline void set_path(const std::string& path) { path_ = path; }

  inline std::string args() const { return args_; }
  inline void set_args(const std::string& args) { args_ = args; }

 private:
  std::string path_;
  std::string args_;
};

}  // process
}  // services
}  // ssf

#endif  // SSF_SERVICES_PROCESS_CONFIG_H_