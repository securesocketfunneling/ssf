#ifndef SSF_SERVICES_COPY_FILE_CONFIG_H_
#define SSF_SERVICES_COPY_FILE_CONFIG_H_

#include <string>

#include "services/base_service_config.h"

namespace ssf {
namespace services {
namespace copy_file {

class Config : public BaseServiceConfig {
 public:
  Config();
  Config(const Config& process_service);
};

}  // copy_file
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_FILE_CONFIG_H_