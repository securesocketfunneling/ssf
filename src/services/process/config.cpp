#include "services/process/config.h"

namespace ssf {
namespace services {
namespace process {

Config::Config() : BaseServiceConfig(false), path_(""), args_("") {}

Config::Config(const Config& process_service)
    : BaseServiceConfig(process_service.enabled()),
      path_(process_service.path_),
      args_(process_service.args_) {}

}  // process
}  // services
}  // ssf