#include "services/copy_file/config.h"

namespace ssf {
namespace services {
namespace copy_file {

Config::Config() : BaseServiceConfig(false) {}

Config::Config(const Config& process_service)
    : BaseServiceConfig(process_service.enabled()) {}

}  // copy_file
}  // services
}  // ssf