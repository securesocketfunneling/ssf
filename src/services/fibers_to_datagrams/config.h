#ifndef SSF_SERVICES_FIBERS_TO_DATAGRAMS_CONFIG_H_
#define SSF_SERVICES_FIBERS_TO_DATAGRAMS_CONFIG_H_

#include "services/base_service_config.h"

namespace ssf {
namespace services {
namespace fibers_to_datagrams {

class Config : public BaseServiceConfig {
 public:
  Config();
  Config(const Config& datagram_forwarder);
};

}  // fibers_to_datagrams
}  // services
}  // ssf

#endif  // SSF_SERVICES_FIBERS_TO_DATAGRAMS_CONFIG_H_