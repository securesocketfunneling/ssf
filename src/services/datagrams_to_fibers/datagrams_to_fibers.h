#ifndef SSF_SERVICES_DATAGRAMS_TO_FIBERS_DATAGRAMS_TO_FIBERS_H_
#define SSF_SERVICES_DATAGRAMS_TO_FIBERS_DATAGRAMS_TO_FIBERS_H_

#include <cstdint>

#include <memory>

#include <boost/asio.hpp>

#include "common/boost/fiber/datagram_fiber.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"

#include "core/factories/service_factory.h"

#include "services/base_service.h"

#include "services/admin/requests/create_service_request.h"
#include "services/datagram/datagram_link.h"
#include "services/datagram/datagram_link_operator.h"
#include "services/datagrams_to_fibers/config.h"

namespace ssf {
namespace services {
namespace datagrams_to_fibers {

// datagram_listener microservice
// Listen to UDP endpoint (local_addr, local_port). Each received datagrams
// will be forwarded to the fiber remote_port. Datagrams from fiber will be
// forwarded to sending UDP socket.
template <typename Demux>
class DatagramsToFibers : public BaseService<Demux> {
 public:
  using LocalPortType = typename Demux::local_port_type;
  using RemotePortType = typename Demux::remote_port_type;

  using Parameters = typename ssf::BaseService<Demux>::Parameters;
  using FiberDatagram = typename ssf::BaseService<Demux>::fiber_datagram;
  using FiberEndpoint = typename ssf::BaseService<Demux>::datagram_endpoint;

  using Udp = boost::asio::ip::udp;

  using UdpOperator = DatagramLinkOperator<Udp::endpoint, Udp::socket,
                                           FiberEndpoint, FiberDatagram>;
  using UdpOperatorPtr = std::shared_ptr<UdpOperator>;
  using DatagramsToFibersPtr = std::shared_ptr<DatagramsToFibers>;

  using WorkingBufferType = std::array<uint8_t, 50 * 1024>;

 public:
  enum { kFactoryId = 6 };

 public:
  DatagramsToFibers() = delete;
  DatagramsToFibers(const DatagramsToFibers&) = delete;

  ~DatagramsToFibers() {
    SSF_LOG(kLogDebug) << "microservice[datagram_listener]: destroy";
  }

 public:
  // Factory method for datagram_listener microservice
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
  static DatagramsToFibersPtr Create(boost::asio::io_service& io_service,
                                     Demux& fiber_demux, Parameters parameters,
                                     bool gateway_ports) {
    if (!parameters.count("local_addr") || !parameters.count("local_port") ||
        !parameters.count("remote_port")) {
      return DatagramsToFibersPtr(nullptr);
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
        SSF_LOG(kLogWarning) << "microservice[datagram_listener]: cannot "
                                "listen on network interface <"
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
          << "microservice[datagram_listener]: cannot extract port parameters";
      return DatagramsToFibersPtr(nullptr);
    }

    if (local_port > 65535) {
      SSF_LOG(kLogError) << "microservice[datagram_listener]: local port ("
                         << local_port << ") out of range ";
      return DatagramsToFibersPtr(nullptr);
    }

    return std::shared_ptr<DatagramsToFibers>(new DatagramsToFibers(
        io_service, fiber_demux, local_addr,
        static_cast<LocalPortType>(local_port), remote_port));
  }

  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<Demux>> p_factory, const Config& config) {
    if (!config.enabled()) {
      // service factory is not enabled
      return;
    }

    p_factory->RegisterServiceCreator(
        kFactoryId, boost::bind(&DatagramsToFibers::Create, _1, _2, _3,
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

 private:
  DatagramsToFibers(boost::asio::io_service& io_service, Demux& fiber_demux,
                    const std::string& local_addr, LocalPortType local_port,
                    RemotePortType remote_port);

  void AsyncReceiveDatagram();
  void SocketDatagramReceiveHandler(const boost::system::error_code& ec,
                                    size_t length);

  DatagramsToFibersPtr SelfFromThis() {
    return std::static_pointer_cast<DatagramsToFibers>(
        this->shared_from_this());
  }

 private:
  std::string local_addr_;
  LocalPortType local_port_;
  RemotePortType remote_port_;
  Udp::socket socket_;

  Udp::endpoint from_endpoint_;
  FiberEndpoint to_endpoint_;

  WorkingBufferType working_buffer_;

  UdpOperatorPtr p_udp_operator_;
};

}  // datagrams_to_fibers
}  // services
}  // ssf

#include "services/datagrams_to_fibers/datagrams_to_fibers.ipp"

#endif  // SSF_SERVICES_DATAGRAMS_TO_FIBERS_DATAGRAMS_TO_FIBERS_H_