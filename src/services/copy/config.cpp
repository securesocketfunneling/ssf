#include "services/copy/config.h"

namespace ssf {
namespace services {
namespace copy {

Config::Config() : BaseServiceConfig(false) {}

Config::Config(const Config& copy_service)
    : BaseServiceConfig(copy_service.enabled()) {}

}  // copy
}  // services
}  // ssf