#include "services/fibers_to_datagrams/config.h"

namespace ssf {
namespace services {
namespace fibers_to_datagrams {

Config::Config() : BaseServiceConfig(true) {}

Config::Config(const Config& datagram_forwarder)
    : BaseServiceConfig(datagram_forwarder.enabled()) {}

}  // fibers_to_datagrams
}  // services
}  // ssf