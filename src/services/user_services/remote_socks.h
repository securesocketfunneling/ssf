#ifndef SSF_SERVICES_USER_SERVICES_REMOTE_SOCKS_H_
#define SSF_SERVICES_USER_SERVICES_REMOTE_SOCKS_H_

#include <cstdint>

#include <vector>
#include <memory>
#include <stdexcept>

#include <boost/system/error_code.hpp>

#include "common/error/error.h"

#include "services/user_services/option_parser.h"

#include "services/admin/requests/create_service_request.h"
#include "services/admin/requests/stop_service_request.h"
#include "services/sockets_to_fibers/sockets_to_fibers.h"
#include "services/socks/socks_server.h"
#include "services/user_services/base_user_service.h"

#include "core/factories/service_option_factory.h"

namespace ssf {
namespace services {

template <typename Demux>
class RemoteSocks : public BaseUserService<Demux> {
 public:
  static std::string GetFullParseName() { return "remote-socks,F"; }

  static std::string GetParseName() { return "remote-socks"; }

  static std::string GetValueName() { return "[[rem_ip]:]rem_port"; }

  static std::string GetParseDesc() {
    return "Run a SOCKS proxy on localhost accessible from server "
           "[[rem_ip]:]rem_port";
  }

  static std::shared_ptr<RemoteSocks> CreateServiceOptions(
      const std::string& line, boost::system::error_code& ec) {
    auto listener = OptionParser::ParseListeningOption(line, ec);

    if (ec) {
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      return std::shared_ptr<RemoteSocks>(nullptr);
    }

    return std::shared_ptr<RemoteSocks>(
        new RemoteSocks(listener.addr, listener.port));
  }

  static void RegisterToServiceOptionFactory() {
    ServiceOptionFactory<Demux>::RegisterUserServiceParser(
        GetParseName(), GetFullParseName(), GetValueName(), GetParseDesc(),
        &RemoteSocks::CreateServiceOptions);
  }

 public:
  virtual ~RemoteSocks() {}

  std::string GetName() override { return "remote-socks"; }

  std::vector<admin::CreateServiceRequest<Demux>> GetRemoteServiceCreateVector()
      override {
    std::vector<admin::CreateServiceRequest<Demux>> result;

    services::admin::CreateServiceRequest<Demux> r_forward(
        services::sockets_to_fibers::SocketsToFibers<Demux>::GetCreateRequest(
            remote_addr_, remote_port_, remote_port_));

    result.push_back(r_forward);

    return result;
  }

  std::vector<admin::StopServiceRequest<Demux>> GetRemoteServiceStopVector(
      Demux& demux) override {
    std::vector<admin::StopServiceRequest<Demux>> result;

    auto id = GetRemoteServiceId(demux);

    if (id) {
      result.push_back(admin::StopServiceRequest<Demux>(id));
    }

    return result;
  }

  bool StartLocalServices(Demux& demux) override {
    services::admin::CreateServiceRequest<Demux> l_socks(
        services::socks::SocksServer<Demux>::GetCreateRequest(remote_port_));

    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    boost::system::error_code ec;
    local_service_id_ = p_service_factory->CreateRunNewService(
        l_socks.service_id(), l_socks.parameters(), ec);

    if (ec) {
      SSF_LOG(kLogError) << "user_service[remote-socks]: "
                         << "local_service[socks]: start failed: "
                         << ec.message();
    }
    return !ec;
  }

  uint32_t CheckRemoteServiceStatus(Demux& demux) override {
    services::admin::CreateServiceRequest<Demux> r_forward(
        services::sockets_to_fibers::SocketsToFibers<Demux>::GetCreateRequest(
            remote_addr_, remote_port_, remote_port_));
    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    auto status = p_service_factory->GetStatus(r_forward.service_id(),
                                               r_forward.parameters(),
                                               GetRemoteServiceId(demux));

    return status;
  }

  void StopLocalServices(Demux& demux) override {
    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    p_service_factory->StopService(local_service_id_);
  }

 private:
  RemoteSocks(const std::string& remote_addr, uint16_t remote_port)
      : remote_addr_(remote_addr),
        remote_port_(remote_port),
        remote_service_id_(0),
        local_service_id_(0) {}

  uint32_t GetRemoteServiceId(Demux& demux) {
    if (remote_service_id_) {
      return remote_service_id_;
    } else {
      services::admin::CreateServiceRequest<Demux> r_forward(
          services::sockets_to_fibers::SocketsToFibers<Demux>::GetCreateRequest(
              remote_addr_, remote_port_, remote_port_));

      auto p_service_factory =
          ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
      auto id = p_service_factory->GetIdFromParameters(r_forward.service_id(),
                                                       r_forward.parameters());
      remote_service_id_ = id;
      return id;
    }
  }

  std::string remote_addr_;
  uint16_t remote_port_;
  uint32_t remote_service_id_;
  uint32_t local_service_id_;
};

}  // services
}  // ssf

#endif  // SSF_SERVICES_USER_SERVICES_REMOTE_SOCKS_H_
