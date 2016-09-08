#ifndef SSF_SERVICES_SOCKS_SOCKS_SERVER_H_
#define SSF_SERVICES_SOCKS_SOCKS_SERVER_H_

#include <string>
#include <map>

#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>  // NOLINT

#include <ssf/network/manager.h>
#include <ssf/network/base_session.h>

#include "services/base_service.h"

#include "common/boost/fiber/stream_fiber.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"

#include "core/factories/service_factory.h"

#include "services/admin/requests/create_service_request.h"

#include "services/socks/config.h"

namespace ssf {
namespace services {
namespace socks {

template <typename Demux>
class SocksServer : public BaseService<Demux> {
 private:
  typedef typename Demux::local_port_type local_port_type;

  typedef std::shared_ptr<SocksServer> SocksServerPtr;
  typedef ItemManager<BaseSessionPtr> SessionManager;
  typedef typename ssf::BaseService<Demux>::Parameters Parameters;
  typedef typename ssf::BaseService<Demux>::demux demux;
  typedef typename ssf::BaseService<Demux>::fiber fiber;
  typedef typename ssf::BaseService<Demux>::fiber_acceptor fiber_acceptor;
  typedef typename ssf::BaseService<Demux>::endpoint endpoint;

 public:
  // SSF service ID for identification in the service factory
  enum { factory_id = 2 };

 public:
  SocksServer(const SocksServer&) = delete;
  SocksServer& operator=(const SocksServer&) = delete;

  /// Create a new instance of the service
  static SocksServerPtr create(boost::asio::io_service& io_service,
                               demux& fiber_demux, Parameters parameters) {
    if (!parameters.count("local_port")) {
      return SocksServerPtr(nullptr);
    } else {
      return std::shared_ptr<SocksServer>(new SocksServer(
          io_service, fiber_demux, std::stoul(parameters["local_port"])));
    }
  }

  /// Function used to register the micro service to the given factory
  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<demux>> p_factory, const Config& config) {
    if (!config.enabled()) {
      // service factory is not enabled
      return;
    }

    p_factory->RegisterServiceCreator(factory_id, &SocksServer::create);
  }

  /// Function used to generate create service request
  static ssf::services::admin::CreateServiceRequest<demux> GetCreateRequest(
      uint16_t local_port) {
    ssf::services::admin::CreateServiceRequest<demux> create(factory_id);
    create.add_parameter("local_port", std::to_string(local_port));

    return std::move(create);
  }

 public:
  /// Start the service instance
  void start(boost::system::error_code& ec) override;

  /// Stop the service instance
  void stop(boost::system::error_code& ec) override;

  /// Return the type of the service
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
