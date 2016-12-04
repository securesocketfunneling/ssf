#ifndef SSF_SERVICES_SOCKS_SOCKS_SERVER_H_
#define SSF_SERVICES_SOCKS_SOCKS_SERVER_H_

#include <string>
#include <map>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>
#include <ssf/network/manager.h>
#include <ssf/network/base_session.h>

#include "services/base_service.h"

#include "core/factories/service_factory.h"

#include "services/admin/requests/create_service_request.h"

#include "services/socks/config.h"

namespace ssf {
namespace services {
namespace socks {

template <typename Demux>
class SocksServer : public BaseService<Demux> {
 private:
  using local_port_type = typename Demux::local_port_type;

  using SocksServerPtr = std::shared_ptr<SocksServer>;
  using SessionManager = ItemManager<BaseSessionPtr>;
  using Parameters = typename ssf::BaseService<Demux>::Parameters;
  using demux = typename ssf::BaseService<Demux>::demux;
  using fiber = typename ssf::BaseService<Demux>::fiber;
  using fiber_acceptor = typename ssf::BaseService<Demux>::fiber_acceptor;
  using endpoint = typename ssf::BaseService<Demux>::endpoint;

 public:
  // Service ID in the service factory
  enum { factory_id = 2 };

 public:
  SocksServer(const SocksServer&) = delete;
  SocksServer& operator=(const SocksServer&) = delete;

  // Create a new instance of the service
  static SocksServerPtr create(boost::asio::io_service& io_service,
                               demux& fiber_demux, Parameters parameters) {
    if (!parameters.count("local_port")) {
      return SocksServerPtr(nullptr);
    }

    try {
      uint32_t local_port = std::stoul(parameters["local_port"]);
      return std::shared_ptr<SocksServer>(
          new SocksServer(io_service, fiber_demux, local_port));
    } catch (const std::exception&) {
      SSF_LOG(kLogError)
          << "microservice[socks]: cannot extract port parameter";
      return SocksServerPtr(nullptr);
    }
  }

  // Register the microservice to the given factory
  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<demux>> p_factory, const Config& config) {
    if (!config.enabled()) {
      // service factory is not enabled
      return;
    }

    p_factory->RegisterServiceCreator(factory_id, &SocksServer::create);
  }

  // Generate create service request
  static ssf::services::admin::CreateServiceRequest<demux> GetCreateRequest(
      uint16_t local_port) {
    ssf::services::admin::CreateServiceRequest<demux> create(factory_id);
    create.add_parameter("local_port", std::to_string(local_port));

    return std::move(create);
  }

 public:
  // BaseService
  void start(boost::system::error_code& ec) override;

  void stop(boost::system::error_code& ec) override;

  uint32_t service_type_id() override;

 private:
  SocksServer(boost::asio::io_service& io_service, demux& fiber_demux,
              const local_port_type& port);

  void StartAccept();
  void HandleAccept(const boost::system::error_code& e);
  void HandleStop();

  template <typename Handler, typename This>
  auto Then(Handler handler,
            This me) -> decltype(boost::bind(handler, me->SelfFromThis(), _1)) {
    return boost::bind(handler, me->SelfFromThis(), _1);
  }

  std::shared_ptr<SocksServer> SelfFromThis() {
    return std::static_pointer_cast<SocksServer>(this->shared_from_this());
  }

 private:
  fiber_acceptor fiber_acceptor_;
  SessionManager session_manager_;
  fiber new_connection_;
  boost::system::error_code init_ec_;
  local_port_type local_port_;
};

}  // socks
}  // services
}  // ssf

#include "services/socks/socks_server.ipp"

#endif  // SSF_SERVICES_SOCKS_SOCKS_SERVER_H_
