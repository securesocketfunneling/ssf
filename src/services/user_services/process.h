#ifndef SSF_SERVICES_USER_SERVICES_PROCESS_H_
#define SSF_SERVICES_USER_SERVICES_PROCESS_H_

#include <cstdint>

#include <memory>
#include <string>
#include <vector>

#include "core/factories/service_option_factory.h"

#include "services/admin/requests/create_service_request.h"
#include "services/admin/requests/stop_service_request.h"
#include "services/process/server.h"
#include "services/sockets_to_fibers/sockets_to_fibers.h"
#include "services/user_services/base_user_service.h"

namespace ssf {
namespace services {

template <typename Demux>
class Process : public BaseUserService<Demux> {
 private:
  Process(uint16_t local_port)
      : local_port_(local_port), remoteServiceId_(0), localServiceId_(0) {}

 public:
  static std::string GetFullParseName() { return "process,X"; }

  static std::string GetParseName() { return "process"; }

  static std::string GetValueName() { return "local_port"; }

  static std::string GetParseDesc() {
    return "Open a port on the client side, each connection to that port "
           "creates a process with I/O forwarded to/from the server side";
  }

  static std::shared_ptr<Process> CreateServiceOptions(
      std::string line, boost::system::error_code& ec) {
    try {
      uint16_t port = (uint16_t)std::stoul(line);
      return std::shared_ptr<Process>(new Process(port));
    } catch (const std::invalid_argument&) {
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      return std::shared_ptr<Process>(nullptr);
    } catch (const std::out_of_range&) {
      ec.assign(::error::out_of_range, ::error::get_ssf_category());
      return std::shared_ptr<Process>(nullptr);
    }
  }

  static void RegisterToServiceOptionFactory() {
    ServiceOptionFactory<Demux>::RegisterUserServiceParser(
        GetParseName(), GetFullParseName(), GetValueName(), GetParseDesc(),
        &Process::CreateServiceOptions);
  }

 public:
  std::vector<admin::CreateServiceRequest<Demux>>
  GetRemoteServiceCreateVector() {
    std::vector<admin::CreateServiceRequest<Demux>> result;

    services::admin::CreateServiceRequest<Demux> r_process_server(
        services::process::Server<Demux>::GetCreateRequest(local_port_));

    result.push_back(r_process_server);

    return result;
  };

  std::vector<admin::StopServiceRequest<Demux>> GetRemoteServiceStopVector(
      Demux& demux) {
    std::vector<admin::StopServiceRequest<Demux>> result;

    auto id = GetRemoteServiceId(demux);

    if (id) {
      result.push_back(admin::StopServiceRequest<Demux>(id));
    }

    return result;
  };

  uint32_t CheckRemoteServiceStatus(Demux& demux) {
    services::admin::CreateServiceRequest<Demux> r_process_server(
        services::process::Server<Demux>::GetCreateRequest(local_port_));

    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    return p_service_factory->GetStatus(r_process_server.service_id(),
                                        r_process_server.parameters(),
                                        GetRemoteServiceId(demux));
  };

  std::string GetName() { return "process"; };

  bool StartLocalServices(Demux& demux) {
    services::admin::CreateServiceRequest<Demux> l_forward(
        services::sockets_to_fibers::SocketsToFibers<Demux>::GetCreateRequest(
            local_port_, local_port_));

    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    boost::system::error_code ec;
    localServiceId_ = p_service_factory->CreateRunNewService(
        l_forward.service_id(), l_forward.parameters(), ec);
    if (ec) {
      SSF_LOG(kLogError) << "user_service[process]: "
                         << "local_service[sockets to fibers]: start failed: "
                         << ec.message();
    }
    return !ec;
  };

  void StopLocalServices(Demux& demux) {
    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    p_service_factory->StopService(localServiceId_);
  };

 private:
  uint32_t GetRemoteServiceId(Demux& demux) {
    if (remoteServiceId_) {
      return remoteServiceId_;
    } else {
      services::admin::CreateServiceRequest<Demux> l_forward(
          services::process::Server<Demux>::GetCreateRequest(local_port_));

      auto p_service_factory =
          ServiceFactoryManager<Demux>::GetServiceFactory(&demux);

      auto id = p_service_factory->GetIdFromParameters(l_forward.service_id(),
                                                       l_forward.parameters());

      remoteServiceId_ = id;
      return id;
    }
  }

 private:
  uint16_t local_port_;
  uint32_t remoteServiceId_;
  uint32_t localServiceId_;
};

}  // services
}  // ssf

#endif  // SSF_SERVICES_USER_SERVICES_PROCESS_H_
