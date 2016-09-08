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

  inline bool gateway_ports() const { return gateway_ports_; }
  inline void set_gateway_ports(bool gateway_ports) {
    gateway_ports_ = gateway_ports;
  }

 private:
  bool gateway_ports_;
};

}  // datagrams_to_fibers
}  // services
}  // ssf

#endif  // SSF_SERVICES_DATAGRAMS_TO_FIBERS_CONFIG_H_