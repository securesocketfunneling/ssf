#ifndef SSF_SERVICES_USER_SERVICES_COPY_SERVICE_H_
#define SSF_SERVICES_USER_SERVICES_COPY_SERVICE_H_

#include <cstdint>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "services/copy/copy_server.h"
#include "services/user_services/base_user_service.h"

namespace ssf {
namespace services {

template <typename Demux>
class Copy : public BaseUserService<Demux> {
 public:
  using ServiceId = uint32_t;
  using ServideIdSet = std::set<ServiceId>;
  using UserParameters = std::map<std::string, std::vector<std::string>>;
  using CopyPtr = std::shared_ptr<Copy>;

 public:
  static std::string GetParseName() { return "copy"; }

  static UserServiceParameterBag CreateUserServiceParameters(
      boost::system::error_code& ec) {
    return {};
  }

  static CopyPtr CreateUserService(const UserServiceParameterBag& params,
                                   boost::system::error_code& ec) {
    return CopyPtr(new Copy());
  }

  ~Copy() {}

  std::string GetName() { return "copy"; }

  std::vector<admin::CreateServiceRequest<Demux>>
  GetRemoteServiceCreateVector() {
    std::vector<admin::CreateServiceRequest<Demux>> result;

    services::admin::CreateServiceRequest<Demux> r_copy_server(
        services::copy::CopyServer<Demux>::GetCreateRequest());
    result.push_back(r_copy_server);

    return result;
  }

  std::vector<admin::StopServiceRequest<Demux>> GetRemoteServiceStopVector(
      Demux& demux) {
    std::vector<admin::StopServiceRequest<Demux>> result;
    auto id = GetRemoteServiceId(demux);

    if (id) {
      result.push_back(admin::StopServiceRequest<Demux>(id));
    }
    return result;
  }

  uint32_t CheckRemoteServiceStatus(Demux& demux) {
    services::admin::CreateServiceRequest<Demux> remote_service_request =
        services::copy::CopyServer<Demux>::GetCreateRequest();

    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    return p_service_factory->GetStatus(remote_service_request.service_id(),
                                        remote_service_request.parameters(),
                                        GetRemoteServiceId(demux));
  }

  bool StartLocalServices(Demux& demux) {
    // no local service
    return true;
  }

  void StopLocalServices(Demux& demux) {
    // no local service
  }

 private:
  Copy() : remote_service_id_(0) {}

  uint32_t GetRemoteServiceId(Demux& demux) {
    if (remote_service_id_) {
      return remote_service_id_;
    } else {
      auto remote_service_request =
          services::copy::CopyServer<Demux>::GetCreateRequest();

      auto p_service_factory =
          ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
      auto id = p_service_factory->GetIdFromParameters(
          remote_service_request.service_id(),
          remote_service_request.parameters());
      remote_service_id_ = id;
      return id;
    }
  }

 private:
  ServiceId remote_service_id_;
};

}  // services
}  // ssf

#endif  // SSF_SERVICES_USER_SERVICES_COPY_SERVICE_H_
