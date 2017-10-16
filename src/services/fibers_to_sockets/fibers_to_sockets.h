#ifndef SSF_SERVICES_FIBERS_TO_SOCKETS_FIBERS_TO_SOCKETS_H_
#define SSF_SERVICES_FIBERS_TO_SOCKETS_FIBERS_TO_SOCKETS_H_

#include <cstdint>

#include <boost/asio.hpp>

#include "common/boost/fiber/basic_fiber_demux.hpp"
#include "common/boost/fiber/stream_fiber.hpp"

#include <ssf/network/base_session.h>
#include <ssf/network/manager.h>
#include <ssf/network/socket_link.h>

#include "services/base_service.h"
#include "services/service_id.h"

#include "core/factories/service_factory.h"

#include "services/admin/requests/create_service_request.h"
#include "services/fibers_to_sockets/config.h"

namespace ssf {
namespace services {
namespace fibers_to_sockets {

template <typename Demux>
class FibersToSockets : public BaseService<Demux> {
 private:
  using LocalPortType = typename Demux::local_port_type;
  using RemotePortType = typename Demux::remote_port_type;
  using FibersToSocketsPtr = std::shared_ptr<FibersToSockets>;
  using SessionManager = ItemManager<BaseSessionPtr>;

  using BaseServicePtr = std::shared_ptr<BaseService<Demux>>;
  using Parameters = typename ssf::BaseService<Demux>::Parameters;
  using Fiber = typename ssf::BaseService<Demux>::fiber;
  using FiberPtr = std::shared_ptr<Fiber>;
  using FiberEndpoint = typename ssf::BaseService<Demux>::endpoint;
  using FiberAcceptor = typename ssf::BaseService<Demux>::fiber_acceptor;

  using Tcp = boost::asio::ip::tcp;

 public:
  enum { kFactoryId = ServiceId::kFibersToSockets };

 public:
  FibersToSockets() = delete;
  FibersToSockets(const FibersToSockets&) = delete;

  ~FibersToSockets() {
    SSF_LOG(kLogDebug) << "microservice[stream_forwarder]: destroy";
  }

 public:
  static FibersToSocketsPtr Create(boost::asio::io_service& io_service,
                                   Demux& fiber_demux,
                                   const Parameters& parameters) {
    if (!parameters.count("local_port") || !parameters.count("remote_ip") ||
        !parameters.count("remote_port")) {
      return FibersToSocketsPtr(nullptr);
    }

    uint32_t local_port;
    uint32_t remote_port;
    try {
      local_port = std::stoul(parameters.at("local_port"));
      remote_port = std::stoul(parameters.at("remote_port"));
    } catch (const std::exception&) {
      SSF_LOG(kLogError) << "microservice[stream_forwarder]: cannot extract "
                            "port parameters";
      return FibersToSocketsPtr(nullptr);
    }

    if (remote_port > 65535) {
      SSF_LOG(kLogError) << "microservice[stream_forwarder]: local port ("
                         << remote_port << ") out of range ";
      return FibersToSocketsPtr(nullptr);
    }

    return FibersToSocketsPtr(new FibersToSockets(
        io_service, fiber_demux, local_port, parameters.at("remote_ip"),
        static_cast<RemotePortType>(remote_port)));
  }

  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<Demux>> p_factory, const Config& config) {
    if (!config.enabled()) {
      // service factory is not enabled
      return;
    }

    auto creator = [](boost::asio::io_service& io_service, Demux& fiber_demux,
                      const Parameters& parameters) {
      return FibersToSockets::Create(io_service, fiber_demux, parameters);
    };
    p_factory->RegisterServiceCreator(kFactoryId, creator);
  }

  static ssf::services::admin::CreateServiceRequest<Demux> GetCreateRequest(
      LocalPortType local_port, std::string remote_addr,
      RemotePortType remote_port) {
    ssf::services::admin::CreateServiceRequest<Demux> create_req(kFactoryId);
    create_req.add_parameter("local_port", std::to_string(local_port));
    create_req.add_parameter("remote_ip", remote_addr);
    create_req.add_parameter("remote_port", std::to_string(remote_port));

    return create_req;
  }

 public:
  void start(boost::system::error_code& ec) override;
  void stop(boost::system::error_code& ec) override;
  uint32_t service_type_id() override;

 public:
  void StopSession(BaseSessionPtr session, boost::system::error_code& ec);

 private:
  FibersToSockets(boost::asio::io_service& io_service, Demux& fiber_demux,
                  LocalPortType local_port, const std::string& ip,
                  RemotePortType remote_port);

  void AsyncAcceptFibers();

  void FiberAcceptHandler(FiberPtr new_connection,
                          const boost::system::error_code& ec);

  void TcpSocketConnectHandler(std::shared_ptr<Tcp::socket> socket,
                               FiberPtr fiber_connection,
                               const boost::system::error_code& ec);

  FibersToSocketsPtr SelfFromThis() {
    return std::static_pointer_cast<FibersToSockets>(this->shared_from_this());
  }

 private:
  RemotePortType remote_port_;
  std::string ip_;
  LocalPortType local_port_;
  FiberAcceptor fiber_acceptor_;

  Tcp::endpoint remote_endpoint_;

  SessionManager manager_;
};

}  // fibers_to_sockets
}  // services
}  // ssf

#include "services/fibers_to_sockets/fibers_to_sockets.ipp"

#endif  // SSF_SERVICES_FIBERS_TO_SOCKETS_FIBERS_TO_SOCKETS_H_
