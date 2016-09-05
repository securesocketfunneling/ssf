#include "services/datagrams_to_fibers/config.h"

namespace ssf {
namespace services {
namespace datagrams_to_fibers {

Config::Config() : BaseServiceConfig(true) {}

Config::Config(const Config& datagram_listener)
    : BaseServiceConfig(datagram_listener.enabled()) {}

}  // datagrams_to_fibers
}  // services
}  // ssf