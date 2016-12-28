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
#include "services/sockets_to_fibers/config.h"

namespace ssf {
namespace services {
namespace sockets_to_fibers {

// stream_listener microservice
// Listen to new connection on TCP endpoint (local_addr, local_port). Each
// connection will create a new fiber connected to the remote_port and forward
// I/O from/to TCP socket
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
  enum { factory_id = 4 };

 public:
  SocketsToFibers() = delete;
  SocketsToFibers(const SocketsToFibers&) = delete;

 public:
  // Factory method for stream_listener microservice
  // @param io_service
  // @param fiber_demux
  // @param parameters microservice configuration parameters
  // @param gateway_ports true to interpret local_addr parameters. Default
  //   behavior will set local_addr to 127.0.0.1
  // @returns Microservice or nullptr if an error occured
  //
  // parameters format:
  // {
  //    "local_addr": IP_ADDR|*|""
  //    "local_port": TCP_PORT
  //    "remote_port": FIBER_PORT
  //  }
  static SocketsToFibersPtr Create(boost::asio::io_service& io_service,
                                   demux& fiber_demux, Parameters parameters,
                                   bool gateway_ports) {
    if (!parameters.count("local_addr") || !parameters.count("local_port") ||
        !parameters.count("remote_port")) {
      return SocketsToFibersPtr(nullptr);
    }

    std::string local_addr("127.0.0.1");
    if (gateway_ports && parameters.count("local_addr") &&
        !parameters["local_addr"].empty()) {
      if (parameters["local_addr"] == "*") {
        local_addr = "0.0.0.0";
      } else {
        local_addr = parameters["local_addr"];
      }
    }

    uint32_t local_port;
    uint32_t remote_port;
    try {
      local_port = std::stoul(parameters["local_port"]);
      remote_port = std::stoul(parameters["remote_port"]);
    } catch (const std::exception&) {
      SSF_LOG(kLogError)
          << "microservice[stream_listener]: cannot extract port parameters";
      return SocketsToFibersPtr(nullptr);
    }

    if (local_port > 65535) {
      SSF_LOG(kLogError) << "microservice[stream_listener]: local port ("
                         << local_port << ") out of range ";
      return SocketsToFibersPtr(nullptr);
    }

    return std::shared_ptr<SocketsToFibers>(
        new SocketsToFibers(io_service, fiber_demux, local_addr,
                            static_cast<uint16_t>(local_port), remote_port));
  }

  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<demux>> p_factory, const Config& config) {
    if (!config.enabled()) {
      // service factory is not enabled
      return;
    }

    p_factory->RegisterServiceCreator(
        factory_id, boost::bind(&SocketsToFibers::Create, _1, _2, _3,
                                config.gateway_ports()));
  }

  static ssf::services::admin::CreateServiceRequest<demux> GetCreateRequest(
      const std::string& local_addr, uint16_t local_port,
      remote_port_type remote_port) {
    ssf::services::admin::CreateServiceRequest<demux> create(factory_id);
    create.add_parameter("local_addr", local_addr);
    create.add_parameter("local_port", std::to_string(local_port));
    create.add_parameter("remote_port", std::to_string(remote_port));

    return create;
  }

 public:
  void start(boost::system::error_code& ec) override;
  void stop(boost::system::error_code& ec) override;
  uint32_t service_type_id() override;

 private:
  SocketsToFibers(boost::asio::io_service& io_service, demux& fiber_demux,
                  const std::string& local_addr, uint16_t local_port,
                  remote_port_type remote_port);

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

 private:
  std::string local_addr_;
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