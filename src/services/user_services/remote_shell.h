#ifndef SSF_SERVICES_USER_SERVICES_REMOTE_PROCESS_H_
#define SSF_SERVICES_USER_SERVICES_REMOTE_PROCESS_H_

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
class RemoteShell : public BaseUserService<Demux> {
 public:
  static std::string GetFullParseName() { return "Y,remote-shell"; }

  static std::string GetParseName() { return "remote-shell"; }

  static std::string GetValueName() {
    return "[bind_address:]port";
  }

  static std::string GetParseDesc() {
    return "Enable remote shell service";
  }

  static UserServiceParameterBag CreateUserServiceParameters(
      const std::string& line, boost::system::error_code& ec) {
    auto listener = OptionParser::ParseListeningOption(line, ec);

    if (ec) {
      SSF_LOG("user_service", error, "[{}] cannot parse {}", GetParseName(),
              line);
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      return {};
    }

    return {{"addr", listener.addr}, {"port", std::to_string(listener.port)}};
  }

  static std::shared_ptr<RemoteShell> CreateUserService(
      const UserServiceParameterBag& parameters,
      boost::system::error_code& ec) {
    if (parameters.count("addr") == 0 || parameters.count("port") == 0) {
      SSF_LOG("user_service", error, "[{}] missing parameters", GetParseName());
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      return std::shared_ptr<RemoteShell>(nullptr);
    }

    uint16_t port = OptionParser::ParsePort(parameters.at("port"), ec);
    if (ec) {
      SSF_LOG("user_service", error, "[{}] invalid port: {}", GetParseName(),
              ec.message());
      return std::shared_ptr<RemoteShell>(nullptr);
    }
    return std::shared_ptr<RemoteShell>(
        new RemoteShell(parameters.at("addr"), port));
  }

 public:
  ~RemoteShell() {}

  std::string GetName() override { return "remote-shell"; };

  std::vector<admin::CreateServiceRequest<Demux>> GetRemoteServiceCreateVector()
      override {
    std::vector<admin::CreateServiceRequest<Demux>> result;

    services::admin::CreateServiceRequest<Demux> r_forward(
        services::sockets_to_fibers::SocketsToFibers<Demux>::GetCreateRequest(
            remote_addr_, remote_port_, remote_port_));

    result.push_back(r_forward);

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
    services::admin::CreateServiceRequest<Demux> r_forward(
        services::sockets_to_fibers::SocketsToFibers<Demux>::GetCreateRequest(
            remote_addr_, remote_port_, remote_port_));

    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    return p_service_factory->GetStatus(r_forward.service_id(),
                                        r_forward.parameters(),
                                        GetRemoteServiceId(demux));
  };

  bool StartLocalServices(Demux& demux) override {
    services::admin::CreateServiceRequest<Demux> l_process_server(
        services::process::Server<Demux>::GetCreateRequest(remote_port_));

    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    boost::system::error_code ec;
    localServiceId_ = p_service_factory->CreateRunNewService(
        l_process_server.service_id(), l_process_server.parameters(), ec);
    if (ec) {
      SSF_LOG("user_service", error,
              "[{}] local microservice[process]: start failed: {}",
              GetParseName(), ec.message());
    }
    return !ec;
  };

  void StopLocalServices(Demux& demux) override {
    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    p_service_factory->StopService(localServiceId_);
  };

 private:
  RemoteShell(const std::string& remote_addr, uint16_t remote_port)
      : remote_addr_(remote_addr),
        remote_port_(remote_port),
        remoteServiceId_(0),
        localServiceId_(0) {}

  uint32_t GetRemoteServiceId(Demux& demux) {
    if (remoteServiceId_) {
      return remoteServiceId_;
    } else {
      services::admin::CreateServiceRequest<Demux> r_forward(
          services::sockets_to_fibers::SocketsToFibers<Demux>::GetCreateRequest(
              remote_addr_, remote_port_, remote_port_));

      auto p_service_factory =
          ServiceFactoryManager<Demux>::GetServiceFactory(&demux);

      auto id = p_service_factory->GetIdFromParameters(r_forward.service_id(),
                                                       r_forward.parameters());

      remoteServiceId_ = id;
      return id;
    }
  }

 private:
  std::string remote_addr_;
  uint16_t remote_port_;
  uint32_t remoteServiceId_;
  uint32_t localServiceId_;
};

}  // services
}  // ssf

#endif  // SSF_SERVICES_USER_SERVICES_PROCESS_H_
