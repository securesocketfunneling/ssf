#ifndef SSF_SERVICES_USER_SERVICES_SOCKS_H_
#define SSF_SERVICES_USER_SERVICES_SOCKS_H_

#include <cstdint>

#include <vector>
#include <memory>
#include <stdexcept>

#include <boost/system/error_code.hpp>

#include "common/error/error.h"

#include "services/user_services/base_user_service.h"
#include "services/admin/requests/create_service_request.h"
#include "services/admin/requests/stop_service_request.h"

#include "core/factories/service_option_factory.h"

namespace ssf { namespace services {

template <typename Demux>
class Socks : public BaseUserService<Demux> {
private:
 Socks(uint16_t local_port)
     : local_port_(local_port), remoteServiceId_(0), localServiceId_(0) {}

public:
  static std::string GetFullParseName() {
    return "socks,D";
  }

  static std::string GetParseName() {
    return "socks";
  }

  static std::string GetParseDesc() {
    return "Run a proxy socks on remote host";
  }

public:
  static std::shared_ptr<Socks> CreateServiceOptions(
      std::string line, boost::system::error_code& ec) {
    try {
      uint16_t port = (uint16_t)std::stoul(line);
      return std::shared_ptr<Socks>(new Socks(port));
    } catch (const std::invalid_argument&) {
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      return std::shared_ptr<Socks>(nullptr);
    } catch (const std::out_of_range&) {
      ec.assign(::error::out_of_range, ::error::get_ssf_category());
      return std::shared_ptr<Socks>(nullptr);
    }
  }

  static void RegisterToServiceOptionFactory() {
    ServiceOptionFactory<Demux>::RegisterUserServiceParser(
       GetParseName(), GetFullParseName(), GetParseDesc(),
       &Socks::CreateServiceOptions);
  }

  virtual std::string GetName() {
    return "socks";
  }

  virtual std::vector<admin::CreateServiceRequest<Demux>>
      GetRemoteServiceCreateVector() {
    std::vector<admin::CreateServiceRequest<Demux>> result;

    services::admin::CreateServiceRequest<Demux> r_socks(
       services::socks::SocksServer<Demux>::GetCreateRequest(local_port_));

    result.push_back(r_socks);

    return result;
  }

  virtual std::vector<admin::StopServiceRequest<Demux>> GetRemoteServiceStopVector(
      Demux& demux) {
    std::vector<admin::StopServiceRequest<Demux>> result;

    auto id = GetRemoteServiceId(demux);

    if (id) {
      result.push_back(admin::StopServiceRequest<Demux>(id));
    }

    return result;
  }

  virtual bool StartLocalServices(Demux& demux) {
    services::admin::CreateServiceRequest<Demux> l_forward(
        services::sockets_to_fibers::SocketsToFibers<Demux>::GetCreateRequest(
            local_port_, local_port_));

    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    boost::system::error_code ec;
    localServiceId_ = p_service_factory->CreateRunNewService(
        l_forward.service_id(), l_forward.parameters(), ec);
    return !ec;
  }

  virtual uint32_t CheckRemoteServiceStatus(Demux& demux) {
    services::admin::CreateServiceRequest<Demux> r_socks(
        services::socks::SocksServer<Demux>::GetCreateRequest(local_port_));
    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    auto status = p_service_factory->GetStatus(
        r_socks.service_id(), r_socks.parameters(), GetRemoteServiceId(demux));

    return status;
  }

  virtual void StopLocalServices(Demux& demux) {
    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    p_service_factory->StopService(localServiceId_);
  }

 private:
  uint32_t GetRemoteServiceId(Demux& demux) {
    if (remoteServiceId_) {
      return remoteServiceId_;
    } else {
      services::admin::CreateServiceRequest<Demux> l_forward(
          services::socks::SocksServer<Demux>::GetCreateRequest(local_port_));

      auto p_service_factory =
          ServiceFactoryManager<Demux>::GetServiceFactory(&demux);

      auto id = p_service_factory->GetIdFromParameters(l_forward.service_id(),
        l_forward.parameters());

      remoteServiceId_ = id;
      return id;
    }
  }

  uint16_t local_port_;
  uint32_t remoteServiceId_;
  uint32_t localServiceId_;
};

}  // services
}  // ssf

#endif  // SSF_SERVICES_USER_SERVICES_SOCKS_H_
