#ifndef SSF_SERVICES_COPY_CONFIG_H_
#define SSF_SERVICES_COPY_CONFIG_H_

#include "services/base_service_config.h"

namespace ssf {
namespace services {
namespace copy {

class Config : public BaseServiceConfig {
 public:
  Config();
  Config(const Config& process_service);
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_CONFIG_H_