#ifndef SSF_SERVICES_SOCKS_SOCKS_SERVER_H_
#define SSF_SERVICES_SOCKS_SOCKS_SERVER_H_

#include <functional>
#include <map>
#include <string>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>
#include <ssf/network/base_session.h>
#include <ssf/network/manager.h>

#include "common/utils/to_underlying.h"

#include "services/base_service.h"
#include "services/service_id.h"

#include "core/factories/service_factory.h"

#include "services/admin/requests/create_service_request.h"

#include "services/socks/config.h"

namespace ssf {
namespace services {
namespace socks {

template <typename Demux>
class SocksServer : public BaseService<Demux> {
 private:
  using LocalPortType = typename Demux::local_port_type;

  using SocksServerPtr = std::shared_ptr<SocksServer>;
  using SessionManager = ItemManager<BaseSessionPtr>;

  using BaseServicePtr = std::shared_ptr<BaseService<Demux>>;
  using Parameters = typename ssf::BaseService<Demux>::Parameters;
  using Fiber = typename ssf::BaseService<Demux>::fiber;
  using FiberPtr = std::shared_ptr<Fiber>;
  using FiberAcceptor = typename ssf::BaseService<Demux>::fiber_acceptor;
  using FiberEndpoint = typename ssf::BaseService<Demux>::endpoint;

 public:
  // Service ID in the service factory
  enum { kFactoryId = to_underlying(MicroserviceId::kSocksServer) };

 public:
  SocksServer(const SocksServer&) = delete;
  SocksServer& operator=(const SocksServer&) = delete;

  ~SocksServer() { SSF_LOG(kLogDebug) << "microservice[socks]: destroy"; }

  // Create a new instance of the service
  static SocksServerPtr Create(boost::asio::io_service& io_service,
                               Demux& fiber_demux,
                               const Parameters& parameters) {
    if (!parameters.count("local_port")) {
      return SocksServerPtr(nullptr);
    }

    try {
      uint32_t local_port = std::stoul(parameters.at("local_port"));
      return SocksServerPtr(
          new SocksServer(io_service, fiber_demux, local_port));
    } catch (const std::exception&) {
      SSF_LOG(kLogError)
          << "microservice[socks]: cannot extract port parameter";
      return SocksServerPtr(nullptr);
    }
  }

  // Register the microservice to the given factory
  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<Demux>> p_factory, const Config& config) {
    if (!config.enabled()) {
      // service factory is not enabled
      return;
    }

    auto creator = [](boost::asio::io_service& io_service, Demux& fiber_demux,
                      const Parameters& parameters) {
      return SocksServer::Create(io_service, fiber_demux, parameters);
    };
    p_factory->RegisterServiceCreator(kFactoryId, creator);
  }

  // Generate create service request
  static ssf::services::admin::CreateServiceRequest<Demux> GetCreateRequest(
      LocalPortType local_port) {
    ssf::services::admin::CreateServiceRequest<Demux> create(kFactoryId);
    create.add_parameter("local_port", std::to_string(local_port));

    return std::move(create);
  }

 public:
  // BaseService
  void start(boost::system::error_code& ec) override;

  void stop(boost::system::error_code& ec) override;

  uint32_t service_type_id() override;

 public:
  void StopSession(BaseSessionPtr session, boost::system::error_code& ec);

 private:
  SocksServer(boost::asio::io_service& io_service, Demux& fiber_demux,
              const LocalPortType& port);

  void AsyncAcceptFiber();
  void FiberAcceptHandler(FiberPtr fiber_connection,
                          const boost::system::error_code& e);
  void HandleStop();

  SocksServerPtr SelfFromThis() {
    return std::static_pointer_cast<SocksServer>(this->shared_from_this());
  }

 private:
  FiberAcceptor fiber_acceptor_;
  SessionManager session_manager_;
  boost::system::error_code init_ec_;
  LocalPortType local_port_;
};

}  // socks
}  // services
}  // ssf

#include "services/socks/socks_server.ipp"

#endif  // SSF_SERVICES_SOCKS_SOCKS_SERVER_H_
