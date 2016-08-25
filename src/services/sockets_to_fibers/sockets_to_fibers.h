#ifndef SSF_SERVICES_SOCKETS_TO_FIBERS_SOCKETS_TO_FIBERS_H_
#define SSF_SERVICES_SOCKETS_TO_FIBERS_SOCKETS_TO_FIBERS_H_

#include <boost/asio.hpp>

#include "common/boost/fiber/stream_fiber.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"

#include <ssf/network/socket_link.h>
#include <ssf/network/manager.h>
#include <ssf/network/base_session.h>

#include "services/base_service.h"

#include "core/factories/service_factory.h"

#include "services/admin/requests/create_service_request.h"

namespace ssf {
namespace services {
namespace sockets_to_fibers {

template <typename Demux>
class SocketsToFibers : public BaseService<Demux> {
 public:
  typedef typename Demux::remote_port_type remote_port_type;

  typedef std::shared_ptr<SocketsToFibers> SocketsToFibersPtr;
  typedef ItemManager<BaseSessionPtr> SessionManager;
  typedef typename ssf::BaseService<Demux>::Parameters Parameters;
  typedef typename ssf::BaseService<Demux>::demux demux;
  typedef typename ssf::BaseService<Demux>::fiber fiber;
  typedef typename ssf::BaseService<Demux>::endpoint endpoint;

  typedef boost::asio::ip::tcp::socket socket;
  typedef boost::asio::ip::tcp::acceptor socket_acceptor;

 public:
  static SocketsToFibersPtr Create(boost::asio::io_service& io_service,
                                   demux& fiber_demux, Parameters parameters) {
    if (!parameters.count("local_port") || !parameters.count("remote_port")) {
      return SocketsToFibersPtr(nullptr);
    } else {
      return std::shared_ptr<SocketsToFibers>(
          new SocketsToFibers(io_service, fiber_demux,
                              (uint16_t)std::stoul(parameters["local_port"]),
                              std::stoul(parameters["remote_port"])));
    }
  }

  enum { factory_id = 4 };

  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<demux>> p_factory) {
    p_factory->RegisterServiceCreator(factory_id, &SocketsToFibers::Create);
  }

  virtual void start(boost::system::error_code& ec);
  virtual void stop(boost::system::error_code& ec);
  virtual uint32_t service_type_id();

  static ssf::services::admin::CreateServiceRequest<demux> GetCreateRequest(
      uint16_t local_port, remote_port_type remote_port) {
    ssf::services::admin::CreateServiceRequest<demux> create(factory_id);
    create.add_parameter("local_port", std::to_string(local_port));
    create.add_parameter("remote_port", std::to_string(remote_port));

    return create;
  }

 private:
  SocketsToFibers(boost::asio::io_service& io_service, demux& fiber_demux,
                  uint16_t local, remote_port_type remote_port);

 private:
  void StartAcceptSockets();

  void SocketAcceptHandler(const boost::system::error_code& ec);

  void FiberConnectHandler(const boost::system::error_code& ec);

  template <typename Handler, typename This>
  auto Then(Handler handler,
            This me) -> decltype(boost::bind(handler, me->SelfFromThis(), _1)) {
    return boost::bind(handler, me->SelfFromThis(), _1);
  }

  std::shared_ptr<SocketsToFibers> SelfFromThis() {
    return std::static_pointer_cast<SocketsToFibers>(this->shared_from_this());
  }

  uint16_t local_port_;
  remote_port_type remote_port_;
  socket_acceptor socket_acceptor_;
  socket socket_;
  fiber fiber_;

  SessionManager manager_;
};

}  // sockets_to_fibers
}  // services
}  // ssf

#include "services/sockets_to_fibers/sockets_to_fibers.ipp"

#endif  // SSF_SERVICES_SOCKETS_TO_FIBERS_SOCKETS_TO_FIBERS_H_