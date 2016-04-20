#ifndef SSF_SERVICES_USER_SERVICES_REMOTE_PORT_FORWARDING_H_
#define SSF_SERVICES_USER_SERVICES_REMOTE_PORT_FORWARDING_H_

#include <cstdint>

#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

#ifndef BOOST_SPIRIT_USE_PHOENIX_V3
#define BOOST_SPIRIT_USE_PHOENIX_V3 1
#endif
#include <boost/fusion/include/std_pair.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/classic.hpp>

#include <boost/system/error_code.hpp>

#include "common/error/error.h"

#include "services/user_services/base_user_service.h"
#include "services/admin/requests/create_service_request.h"
#include "services/admin/requests/stop_service_request.h"

#include "core/factories/service_option_factory.h"

#include "common/boost/fiber/detail/fiber_id.hpp"

#include "services/sockets_to_fibers/sockets_to_fibers.h"
#include "services/fibers_to_sockets/fibers_to_sockets.h"

namespace ssf {
namespace services {

template <typename Demux>
class RemotePortForwading : public BaseUserService<Demux> {
 private:
  typedef boost::asio::fiber::detail::fiber_id::local_port_type local_port_type;

 private:
  RemotePortForwading(uint16_t remote_port, std::string local_addr,
                      uint16_t local_port)
      : local_port_(local_port),
        local_addr_(local_addr),
        remote_port_(remote_port),
        remoteServiceId_(0),
        localServiceId_(0) {
    relay_fiber_port_ = remote_port_;
  }

 public:
  static std::string GetFullParseName() { return "remote,R"; }

  static std::string GetParseName() { return "remote"; }

  static std::string GetParseDesc() {
    return "Forward remote port on given target";
  }

 public:
  static std::shared_ptr<RemotePortForwading> CreateServiceOptions(
      std::string line, boost::system::error_code& ec) {
    using boost::spirit::qi::int_;
    using boost::spirit::qi::alnum;
    using boost::spirit::qi::char_;
    using boost::spirit::qi::rule;
    typedef std::string::const_iterator str_it;

    rule<str_it, std::string()> target_pattern = +char_("0-9a-zA-Z.-");
    uint16_t listening_port, target_port;
    std::string target_addr;
    str_it begin = line.begin(), end = line.end();
    bool parsed = boost::spirit::qi::parse(
        begin, end, int_ >> ":" >> target_pattern >> ":" >> int_,
        listening_port, target_addr, target_port);

    if (parsed) {
      return std::shared_ptr<RemotePortForwading>(
          new RemotePortForwading(listening_port, target_addr, target_port));
    } else {
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      return std::shared_ptr<RemotePortForwading>(nullptr);
    }
  }

  static void RegisterToServiceOptionFactory() {
    ServiceOptionFactory<Demux>::RegisterUserServiceParser(
        GetParseName(), GetFullParseName(), GetParseDesc(),
        &RemotePortForwading::CreateServiceOptions);
  }

  virtual std::string GetName() { return "remote"; }

  virtual std::vector<admin::CreateServiceRequest<Demux>>
  GetRemoteServiceCreateVector() {
    std::vector<admin::CreateServiceRequest<Demux>> result;

    services::admin::CreateServiceRequest<Demux> r_forward(
        services::sockets_to_fibers::SocketsToFibers<Demux>::GetCreateRequest(
            remote_port_, relay_fiber_port_));

    result.push_back(r_forward);

    return result;
  }

  virtual std::vector<admin::StopServiceRequest<Demux>>
  GetRemoteServiceStopVector(Demux& demux) {
    std::vector<admin::StopServiceRequest<Demux>> result;

    auto id = GetRemoteServiceId(demux);

    if (id) {
      result.push_back(admin::StopServiceRequest<Demux>(id));
    }

    return result;
  }

  virtual bool StartLocalServices(Demux& demux) {
    services::admin::CreateServiceRequest<Demux> l_forward(
        services::fibers_to_sockets::FibersToSockets<Demux>::GetCreateRequest(
            relay_fiber_port_, local_addr_, local_port_));

    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    boost::system::error_code ec;
    localServiceId_ = p_service_factory->CreateRunNewService(
        l_forward.service_id(), l_forward.parameters(), ec);
    return !ec;
  }

  virtual uint32_t CheckRemoteServiceStatus(Demux& demux) {
    services::admin::CreateServiceRequest<Demux> r_forward(
        services::sockets_to_fibers::SocketsToFibers<Demux>::GetCreateRequest(
            remote_port_, relay_fiber_port_));
    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    auto status = p_service_factory->GetStatus(r_forward.service_id(),
                                               r_forward.parameters(),
                                               GetRemoteServiceId(demux));

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
      services::admin::CreateServiceRequest<Demux> r_forward(
          services::sockets_to_fibers::SocketsToFibers<Demux>::GetCreateRequest(
              remote_port_, relay_fiber_port_));

      auto p_service_factory =
          ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
      auto id = p_service_factory->GetIdFromParameters(r_forward.service_id(),
                                                       r_forward.parameters());
      remoteServiceId_ = id;
      return id;
    }
  }

  uint16_t local_port_;
  std::string local_addr_;
  uint16_t remote_port_;

  local_port_type relay_fiber_port_;

  uint32_t remoteServiceId_;
  uint32_t localServiceId_;
};

}  // services
}  // ssf

#endif  // SSF_SERVICES_USER_SERVICES_REMOTE_PORT_FORWARDING_H_
