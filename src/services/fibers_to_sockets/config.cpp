#include "services/fibers_to_sockets/config.h"

namespace ssf {
namespace services {
namespace fibers_to_sockets {

Config::Config() : BaseServiceConfig(true) {}

Config::Config(const Config& stream_forwarder)
    : BaseServiceConfig(stream_forwarder.enabled()) {}

}  // fibers_to_sockets
}  // services
}  // ssf