#include "services/sockets_to_fibers/config.h"

namespace ssf {
namespace services {
namespace sockets_to_fibers {

Config::Config() : BaseServiceConfig(true) {}

Config::Config(const Config& stream_listener)
    : BaseServiceConfig(stream_listener.enabled()) {}

}  // sockets_to_fibers
}  // services
}  // ssf