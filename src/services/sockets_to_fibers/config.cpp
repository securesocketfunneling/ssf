#include "services/sockets_to_fibers/config.h"

namespace ssf {
namespace services {
namespace sockets_to_fibers {

Config::Config() : BaseServiceConfig(true), gateway_ports_(false) {}

Config::Config(const Config& stream_listener)
    : BaseServiceConfig(stream_listener.enabled()),
      gateway_ports_(stream_listener.gateway_ports_) {}

}  // sockets_to_fibers
}  // services
}  // ssf