#ifndef SSF_SERVICES_USER_SERVICES_PROCESS_H_
#define SSF_SERVICES_USER_SERVICES_PROCESS_H_

#include <cstdint>

#include <memory>
#include <string>
#include <vector>

#include "services/user_services/option_parser.h"

#include "services/admin/requests/create_service_request.h"
#include "services/admin/requests/stop_service_request.h"
#include "services/process/server.h"
#include "services/sockets_to_fibers/sockets_to_fibers.h"
#include "services/user_services/base_user_service.h"

namespace ssf {
namespace services {

template <typename Demux>
class Process : public BaseUserService<Demux> {
 public:
  static std::string GetFullParseName() { return "shell,X"; }

  static std::string GetParseName() { return "shell"; }

  static std::string GetValueName() { return "[[bind_address]:]port"; }

  static std::string GetParseDesc() {
    return "Forward server shell I/O to the specified port on the local side. "
           "Each connection creates a new shell process";
  }

  static UserServiceParameterBag CreateUserServiceParameters(
      const std::string& line, boost::system::error_code& ec) {
    auto listener = OptionParser::ParseListeningOption(line, ec);

    if (ec) {
      SSF_LOG(kLogError) << "user_service " << GetParseName()
                         << ": cannot parse " << line;
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      return {};
    }

    return {{"addr", listener.addr}, {"port", std::to_string(listener.port)}};
  }

  static std::shared_ptr<Process> CreateUserService(
      const UserServiceParameterBag& parameters,
      boost::system::error_code& ec) {
    if (parameters.count("addr") == 0 || parameters.count("port") == 0) {
      SSF_LOG(kLogError) << "user_service " << GetParseName()
                         << ": missing parameters";
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      return std::shared_ptr<Process>(nullptr);
    }

    uint16_t port = OptionParser::ParsePort(parameters.at("port"), ec);
    if (ec) {
      SSF_LOG(kLogError) << "user_service " << GetParseName()
                         << ": invalid port: "
                         << "(" << ec.message() << ")";
      return std::shared_ptr<Process>(nullptr);
    }
    return std::shared_ptr<Process>(new Process(parameters.at("addr"), port));
  }

 public:
  virtual ~Process() {}

  std::string GetName() override { return "shell"; };

  std::vector<admin::CreateServiceRequest<Demux>> GetRemoteServiceCreateVector()
      override {
    std::vector<admin::CreateServiceRequest<Demux>> result;

    services::admin::CreateServiceRequest<Demux> r_process_server(
        services::process::Server<Demux>::GetCreateRequest(local_port_));

    result.push_back(r_process_server);

    return result;
  };

  std::vector<admin::StopServiceRequest<Demux>> GetRemoteServiceStopVector(
      Demux& demux) override {
    std::vector<admin::StopServiceRequest<Demux>> result;

    auto id = GetRemoteServiceId(demux);

    if (id) {
      result.push_back(admin::StopServiceRequest<Demux>(id));
    }

    return result;
  };

  uint32_t CheckRemoteServiceStatus(Demux& demux) override {
    services::admin::CreateServiceRequest<Demux> r_process_server(
        services::process::Server<Demux>::GetCreateRequest(local_port_));

    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    return p_service_factory->GetStatus(r_process_server.service_id(),
                                        r_process_server.parameters(),
                                        GetRemoteServiceId(demux));
  };

  bool StartLocalServices(Demux& demux) override {
    services::admin::CreateServiceRequest<Demux> l_forward(
        services::sockets_to_fibers::SocketsToFibers<Demux>::GetCreateRequest(
            local_addr_, local_port_, local_port_));

    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    boost::system::error_code ec;
    localServiceId_ = p_service_factory->CreateRunNewService(
        l_forward.service_id(), l_forward.parameters(), ec);
    if (ec) {
      SSF_LOG(kLogError)
          << "user_service[shell]: "
          << "local microservice[stream_listener]: start failed: "
          << ec.message();
    }
    return !ec;
  };

  void StopLocalServices(Demux& demux) override {
    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    p_service_factory->StopService(localServiceId_);
  };

 private:
  Process(const std::string& local_addr, uint16_t local_port)
      : local_addr_(local_addr),
        local_port_(local_port),
        remoteServiceId_(0),
        localServiceId_(0) {}

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
  std::string local_addr_;
  uint16_t local_port_;
  uint32_t remoteServiceId_;
  uint32_t localServiceId_;
};

}  // services
}  // ssf

#endif  // SSF_SERVICES_USER_SERVICES_PROCESS_H_
