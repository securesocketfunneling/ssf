#ifndef SSF_SERVICES_USER_SERVICES_BASE_USER_SERVICE_H_
#define SSF_SERVICES_USER_SERVICES_BASE_USER_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "services/admin/requests/create_service_request.h"
#include "services/admin/requests/stop_service_request.h"

#include "services/user_services/parameters.h"

namespace ssf {
namespace services {

template <typename Demux>
class BaseUserService
    : public std::enable_shared_from_this<BaseUserService<Demux>> {
 public:
  typedef typename std::shared_ptr<BaseUserService<Demux>> BaseUserServicePtr;

  virtual std::vector<admin::CreateServiceRequest<Demux>>
  GetRemoteServiceCreateVector() = 0;
  virtual std::vector<admin::StopServiceRequest<Demux>>
  GetRemoteServiceStopVector(Demux& demux) = 0;
  virtual uint32_t CheckRemoteServiceStatus(Demux& demux) = 0;

  virtual std::string GetName() = 0;

  virtual bool StartLocalServices(Demux& demux) = 0;
  virtual void StopLocalServices(Demux& demux) = 0;

  BaseUserService() {}
  virtual ~BaseUserService() {}

 private:
  BaseUserService(const BaseService<Demux>&) = delete;
  BaseUserService<Demux>& operator=(const BaseService<Demux>&) = delete;
};

}  // services
}  // ssf

#endif  // SSF_SERVICES_USER_SERVICES_BASE_USER_SERVICE_H_