#ifndef SSF_SERVICES_USER_SERVICES_UDP_PORT_FORWARDING_H_
#define SSF_SERVICES_USER_SERVICES_UDP_PORT_FORWARDING_H_

#include <cstdint>

#include <vector>
#include <string>
#include <memory>

#include <boost/system/error_code.hpp>

#include "common/error/error.h"

#include "services/user_services/option_parser.h"

#include "services/user_services/base_user_service.h"
#include "services/admin/requests/create_service_request.h"
#include "services/admin/requests/stop_service_request.h"

#include "common/boost/fiber/detail/fiber_id.hpp"

#include "services/datagrams_to_fibers/datagrams_to_fibers.h"
#include "services/fibers_to_datagrams/fibers_to_datagrams.h"

namespace ssf {
namespace services {

template <typename Demux>
class UdpPortForwarding : public BaseUserService<Demux> {
 private:
  typedef boost::asio::fiber::detail::fiber_id::local_port_type local_port_type;

 public:
  static std::string GetFullParseName() { return "U,udp-forward"; }

  static std::string GetParseName() { return "udp-forward"; }

  static std::string GetValueName() {
    return "[bind_address:]port:remote_host:remote_port";
  }

  static std::string GetParseDesc() {
    return "Enable client UDP port forwarding service";
  }

  static UserServiceParameterBag CreateUserServiceParameters(
      const std::string& line, boost::system::error_code& ec) {
    auto forward_options = OptionParser::ParseForwardOptions(line, ec);

    if (ec) {
      SSF_LOG("user_service", error, "[{}] cannot parse {}", GetParseName(),
              line);
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      return {};
    }

    return {{"from_addr", forward_options.from.addr},
            {"from_port", std::to_string(forward_options.from.port)},
            {"to_addr", forward_options.to.addr},
            {"to_port", std::to_string(forward_options.to.port)}};
  }

  static std::shared_ptr<UdpPortForwarding> CreateUserService(
      const UserServiceParameterBag& parameters,
      boost::system::error_code& ec) {
    if (parameters.count("from_addr") == 0 ||
        parameters.count("from_port") == 0 ||
        parameters.count("to_addr") == 0 || parameters.count("to_port") == 0) {
      SSF_LOG("user_service", error, "[{}] missing parameters", GetParseName());
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      return std::shared_ptr<UdpPortForwarding>(nullptr);
    }

    uint16_t from_port =
        OptionParser::ParsePort(parameters.at("from_port"), ec);
    if (ec) {
      SSF_LOG("user_service", error, "[{}] invalid local port {}",
              GetParseName(), ec.message());
      return std::shared_ptr<UdpPortForwarding>(nullptr);
    }
    uint16_t to_port = OptionParser::ParsePort(parameters.at("to_port"), ec);
    if (ec) {
      SSF_LOG("user_service", error, "[{}] invalid remote port: {}",
              GetParseName(), ec.message());
      return std::shared_ptr<UdpPortForwarding>(nullptr);
    }
    return std::shared_ptr<UdpPortForwarding>(
        new UdpPortForwarding(parameters.at("from_addr"), from_port,
                              parameters.at("to_addr"), to_port));
  }

 public:
  virtual ~UdpPortForwarding() {}

  std::string GetName() override { return "udp-forward"; }

  std::vector<admin::CreateServiceRequest<Demux>> GetRemoteServiceCreateVector()
      override {
    std::vector<admin::CreateServiceRequest<Demux>> result;

    services::admin::CreateServiceRequest<Demux> r_forward(
        services::fibers_to_datagrams::FibersToDatagrams<
            Demux>::GetCreateRequest(relay_fiber_port_, remote_addr_,
                                     remote_port_));

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
    services::admin::CreateServiceRequest<Demux> l_forward(
        services::datagrams_to_fibers::DatagramsToFibers<
            Demux>::GetCreateRequest(local_addr_, local_port_,
                                     relay_fiber_port_));

    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    boost::system::error_code ec;
    localServiceId_ = p_service_factory->CreateRunNewService(
        l_forward.service_id(), l_forward.parameters(), ec);

    if (ec) {
      SSF_LOG("user_service", error,
              "[{}] local_service[datagrams to fibers]: start failed: {}",
              GetParseName(), ec.message());
    }

    return !ec;
  }

  uint32_t CheckRemoteServiceStatus(Demux& demux) override {
    services::admin::CreateServiceRequest<Demux> r_socks(
        services::fibers_to_datagrams::FibersToDatagrams<
            Demux>::GetCreateRequest(relay_fiber_port_, remote_addr_,
                                     remote_port_));
    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    auto status = p_service_factory->GetStatus(
        r_socks.service_id(), r_socks.parameters(), GetRemoteServiceId(demux));

    return status;
  }

  void StopLocalServices(Demux& demux) override {
    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    p_service_factory->StopService(localServiceId_);
  }

 private:
  UdpPortForwarding(const std::string& local_addr, uint16_t local_port,
                    const std::string& remote_addr, uint16_t remote_port)
      : local_addr_(local_addr),
        local_port_(local_port),
        remote_addr_(remote_addr),
        remote_port_(remote_port),
        remoteServiceId_(0),
        localServiceId_(0) {
    relay_fiber_port_ = remote_port_ + (1 << 16);
  }

  uint32_t GetRemoteServiceId(Demux& demux) {
    if (remoteServiceId_) {
      return remoteServiceId_;
    } else {
      services::admin::CreateServiceRequest<Demux> r_forward(
          services::fibers_to_datagrams::FibersToDatagrams<
              Demux>::GetCreateRequest(relay_fiber_port_, remote_addr_,
                                       remote_port_));

      auto p_service_factory =
          ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
      auto id = p_service_factory->GetIdFromParameters(r_forward.service_id(),
                                                       r_forward.parameters());
      remoteServiceId_ = id;
      return id;
    }
  }

  std::string local_addr_;
  uint16_t local_port_;
  std::string remote_addr_;
  uint16_t remote_port_;

  local_port_type relay_fiber_port_;

  uint32_t remoteServiceId_;
  uint32_t localServiceId_;
};

}  // services
}  // ssf

#endif  // SSF_SERVICES_USER_SERVICES_UDP_PORT_FORWARDING_H_
