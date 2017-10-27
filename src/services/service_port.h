#ifndef SSF_SERVICES_SERVICE_PORT_H_
#define SSF_SERVICES_SERVICE_PORT_H_

namespace ssf {
namespace services {

enum class MicroservicePort {
  kMin = (1 << 17),
  kAdmin,
  kCopyServer,
  kCopyFileAcceptor,
  kMax
};

}  // services
}  // ssf

#endif  // SSF_SERVICES_SERVICE_PORT_H_
