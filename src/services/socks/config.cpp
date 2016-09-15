#include "services/socks/config.h"

namespace ssf {
namespace services {
namespace socks {

Config::Config() : BaseServiceConfig(true) {}

Config::Config(const Config& process_service)
    : BaseServiceConfig(process_service.enabled()) {}

}  // socks
}  // services
}  // ssf