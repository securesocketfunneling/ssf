#ifndef SSF_SERVICES_SERVICE_ID_H_
#define SSF_SERVICES_SERVICE_ID_H_

namespace ssf {
namespace services {

enum class MicroserviceId {
  kMin = 0,
  kAdmin,
  kCopyServer,
  kDatagramsToFibers,
  kFibersToDatagrams,
  kSocketsToFibers,
  kFibersToSockets,
  kProcessServer,
  kSocksServer,
  kMax
};

}  // services
}  // ssf

#endif  // SSF_SERVICES_SERVICE_ID_H_
