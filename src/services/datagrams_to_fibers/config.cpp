#include "services/datagrams_to_fibers/config.h"

namespace ssf {
namespace services {
namespace datagrams_to_fibers {

Config::Config() : BaseServiceConfig(true), gateway_ports_(false) {}

Config::Config(const Config& datagram_listener)
    : BaseServiceConfig(datagram_listener.enabled()),
      gateway_ports_(datagram_listener.gateway_ports_) {}

}  // datagrams_to_fibers
}  // services
}  // ssf