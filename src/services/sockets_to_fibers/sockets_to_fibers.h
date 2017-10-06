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
  using LocalPortType = typename Demux::local_port_type;
  using RemotePortType = typename Demux::remote_port_type;

  using SocketsToFibersPtr = std::shared_ptr<SocketsToFibers>;
  using SessionManager = ItemManager<BaseSessionPtr>;
  using Parameters = typename ssf::BaseService<Demux>::Parameters;
  using Fiber = typename ssf::BaseService<Demux>::fiber;
  using FiberPtr = std::shared_ptr<Fiber>;
  using FiberEndpoint = typename ssf::BaseService<Demux>::endpoint;

  using Tcp = boost::asio::ip::tcp;

 public:
  enum { kFactoryId = 4 };

 public:
  SocketsToFibers() = delete;
  SocketsToFibers(const SocketsToFibers&) = delete;

  ~SocketsToFibers() {
    SSF_LOG(kLogDebug) << "microservice[stream_listener]: destroy";
  }

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
                                   Demux& fiber_demux, Parameters parameters,
                                   bool gateway_ports) {
    if (!parameters.count("local_addr") || !parameters.count("local_port") ||
        !parameters.count("remote_port")) {
      return SocketsToFibersPtr(nullptr);
    }

    std::string local_addr("127.0.0.1");
    if (parameters.count("local_addr") && !parameters["local_addr"].empty()) {
      if (gateway_ports) {
        if (parameters["local_addr"] == "*") {
          local_addr = "0.0.0.0";
        } else {
          local_addr = parameters["local_addr"];
        }
      } else {
        SSF_LOG(kLogWarning) << "microservice[stream_listener]: cannot listen "
                                "on network interface <"
                             << parameters["local_addr"]
                             << "> without gateway ports option";
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

    return SocketsToFibersPtr(
        new SocketsToFibers(io_service, fiber_demux, local_addr,
                            static_cast<uint16_t>(local_port), remote_port));
  }

  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<Demux>> p_factory, const Config& config) {
    if (!config.enabled()) {
      // service factory is not enabled
      return;
    }

    p_factory->RegisterServiceCreator(
        kFactoryId, std::bind(&SocketsToFibers::Create, std::placeholders::_1,
                              std::placeholders::_2, std::placeholders::_3,
                              config.gateway_ports()));
  }

  static ssf::services::admin::CreateServiceRequest<Demux> GetCreateRequest(
      const std::string& local_addr, LocalPortType local_port,
      RemotePortType remote_port) {
    ssf::services::admin::CreateServiceRequest<Demux> create_req(kFactoryId);
    create_req.add_parameter("local_addr", local_addr);
    create_req.add_parameter("local_port", std::to_string(local_port));
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
  SocketsToFibers(boost::asio::io_service& io_service, Demux& fiber_demux,
                  const std::string& local_addr, LocalPortType local_port,
                  RemotePortType remote_port);

  void AsyncAcceptSocket();

  void SocketAcceptHandler(std::shared_ptr<Tcp::socket> socket_connection,
                           const boost::system::error_code& ec);

  void FiberConnectHandler(FiberPtr fiber_connection,
                           std::shared_ptr<Tcp::socket> socket_connection,
                           const boost::system::error_code& ec);

  SocketsToFibersPtr SelfFromThis() {
    return std::static_pointer_cast<SocketsToFibers>(this->shared_from_this());
  }

 private:
  std::string local_addr_;
  LocalPortType local_port_;
  RemotePortType remote_port_;
  Tcp::acceptor socket_acceptor_;

  SessionManager manager_;
};

}  // sockets_to_fibers
}  // services
}  // ssf

#include "services/sockets_to_fibers/sockets_to_fibers.ipp"

#endif  // SSF_SERVICES_SOCKETS_TO_FIBERS_SOCKETS_TO_FIBERS_H_