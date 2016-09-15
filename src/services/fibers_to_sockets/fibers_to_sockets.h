#ifndef SSF_SERVICES_FIBERS_TO_SOCKETS_FIBERS_TO_SOCKETS_H_
#define SSF_SERVICES_FIBERS_TO_SOCKETS_FIBERS_TO_SOCKETS_H_

#include <cstdint>

#include <boost/asio.hpp>

#include "common/boost/fiber/stream_fiber.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"

#include <ssf/network/socket_link.h>
#include <ssf/network/manager.h>
#include <ssf/network/base_session.h>

#include "services/base_service.h"

#include "core/factories/service_factory.h"

#include "services/admin/requests/create_service_request.h"
#include "services/fibers_to_sockets/config.h"

namespace ssf {
namespace services {
namespace fibers_to_sockets {

template <typename Demux>
class FibersToSockets : public BaseService<Demux> {
 private:
  typedef typename Demux::local_port_type local_port_type;

  typedef std::shared_ptr<FibersToSockets> FibersToSocketsPtr;
  typedef ItemManager<BaseSessionPtr> SessionManager;
  typedef typename ssf::BaseService<Demux>::Parameters Parameters;
  typedef typename ssf::BaseService<Demux>::demux demux;
  typedef typename ssf::BaseService<Demux>::fiber fiber;
  typedef typename ssf::BaseService<Demux>::endpoint endpoint;
  typedef typename ssf::BaseService<Demux>::fiber_acceptor fiber_acceptor;

  typedef boost::asio::ip::tcp::socket socket;

 public:
  enum { factory_id = 3 };

 public:
  FibersToSockets() = delete;
  FibersToSockets(const FibersToSockets&) = delete;

 public:
  static FibersToSocketsPtr create(boost::asio::io_service& io_service,
                                   demux& fiber_demux, Parameters parameters) {
    if (!parameters.count("local_port") || !parameters.count("remote_ip") ||
        !parameters.count("remote_port")) {
      return FibersToSocketsPtr(nullptr);
    } else {
      return std::shared_ptr<FibersToSockets>(new FibersToSockets(
          io_service, fiber_demux, std::stoul(parameters["local_port"]),
          parameters["remote_ip"],
          (uint16_t)std::stoul(parameters["remote_port"])));
    }
  }

  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<demux>> p_factory, const Config& config) {
    if (!config.enabled()) {
      // service factory is not enabled
      return;
    }

    p_factory->RegisterServiceCreator(factory_id, &FibersToSockets::create);
  }

  static ssf::services::admin::CreateServiceRequest<demux> GetCreateRequest(
      local_port_type local_port, std::string remote_addr,
      uint16_t remote_port) {
    ssf::services::admin::CreateServiceRequest<demux> create(factory_id);
    create.add_parameter("local_port", std::to_string(local_port));
    create.add_parameter("remote_ip", remote_addr);
    create.add_parameter("remote_port", std::to_string(remote_port));

    return create;
  }

 public:
  void start(boost::system::error_code& ec) override;
  void stop(boost::system::error_code& ec) override;
  uint32_t service_type_id() override;

 private:
  FibersToSockets(boost::asio::io_service& io_service, demux& fiber_demux,
                  local_port_type local, const std::string& ip,
                  uint16_t remote_port);

  // No check on prioror presence implemented
  void StartAcceptFibers();

  void FiberAcceptHandler(const boost::system::error_code& ec);

  void SocketConnectHandler(const boost::system::error_code& ec);

  template <typename Handler, typename This>
  auto Then(Handler handler,
            This me) -> decltype(boost::bind(handler, me->SelfFromThis(), _1)) {
    return boost::bind(handler, me->SelfFromThis(), _1);
  }

  std::shared_ptr<FibersToSockets> SelfFromThis() {
    return std::static_pointer_cast<FibersToSockets>(this->shared_from_this());
  }

 private:
  uint16_t remote_port_;
  std::string ip_;
  local_port_type local_port_;
  fiber_acceptor fiber_acceptor_;
  fiber fiber_;
  socket socket_;

  boost::asio::ip::tcp::endpoint endpoint_;

  SessionManager manager_;
};

}  // fibers_to_sockets
}  // services
}  // ssf

#include "services/fibers_to_sockets/fibers_to_sockets.ipp"

#endif  // SSF_SERVICES_FIBERS_TO_SOCKETS_FIBERS_TO_SOCKETS_H_