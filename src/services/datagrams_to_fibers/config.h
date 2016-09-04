#ifndef SSF_SERVICES_DATAGRAMS_TO_FIBERS_CONFIG_H_
#define SSF_SERVICES_DATAGRAMS_TO_FIBERS_CONFIG_H_

#include <string>

#include "services/base_service_config.h"

namespace ssf {
namespace services {
namespace datagrams_to_fibers {

class Config : public BaseServiceConfig {
 public:
  Config();
  Config(const Config& datagram_listener);
};

}  // datagrams_to_fibers
}  // services
}  // ssf

#endif  // SSF_SERVICES_DATAGRAMS_TO_FIBERS_CONFIG_H_