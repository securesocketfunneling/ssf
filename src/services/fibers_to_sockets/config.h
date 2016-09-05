#ifndef SSF_SERVICES_FIBERS_TO_SOCKETS_CONFIG_H_
#define SSF_SERVICES_FIBERS_TO_SOCKETS_CONFIG_H_

#include <string>

#include "services/base_service_config.h"

namespace ssf {
namespace services {
namespace fibers_to_sockets {

class Config : public BaseServiceConfig {
 public:
  Config();
  Config(const Config& stream_forwarder);
};

}  // fibers_to_sockets
}  // services
}  // ssf

#endif  // SSF_SERVICES_FIBERS_TO_SOCKETS_CONFIG_H_