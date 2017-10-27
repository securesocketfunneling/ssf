#ifndef SSF_SERVICES_FIBERS_TO_DATAGRAMS_FIBERS_TO_DATAGRAMS_H_
#define SSF_SERVICES_FIBERS_TO_DATAGRAMS_FIBERS_TO_DATAGRAMS_H_

#include <cstdint>

#include <memory>

#include <boost/asio.hpp>

#include "common/boost/fiber/basic_fiber_demux.hpp"
#include "common/boost/fiber/datagram_fiber.hpp"
#include "common/utils/to_underlying.h"

#include "services/base_service.h"
#include "services/service_id.h"

#include "core/factories/service_factory.h"

#include "services/admin/requests/create_service_request.h"
#include "services/datagram/datagram_link.h"
#include "services/datagram/datagram_link_operator.h"
#include "services/fibers_to_datagrams/config.h"

namespace ssf {
namespace services {
namespace fibers_to_datagrams {

template <typename Demux>
class FibersToDatagrams : public BaseService<Demux> {
 private:
  using LocalPortType = typename Demux::local_port_type;
  using RemotePortType = typename Demux::remote_port_type;

  using BaseServicePtr = std::shared_ptr<BaseService<Demux>>;
  using Parameters = typename ssf::BaseService<Demux>::Parameters;
  using FiberDatagram = typename ssf::BaseService<Demux>::fiber_datagram;
  using FiberEndpoint = typename ssf::BaseService<Demux>::datagram_endpoint;

  using Udp = boost::asio::ip::udp;

  using UdpOperator = DatagramLinkOperator<FiberEndpoint, FiberDatagram,
                                           Udp::endpoint, Udp::socket>;
  using UdpOperatorPtr = std::shared_ptr<UdpOperator>;

  using FibersToDatagramsPtr = std::shared_ptr<FibersToDatagrams>;

  using WorkingBufferType = std::array<uint8_t, 50 * 1024>;

 public:
  enum { kFactoryId = to_underlying(MicroserviceId::kFibersToDatagrams) };

 public:
  FibersToDatagrams() = delete;
  FibersToDatagrams(const FibersToDatagrams&) = delete;

  ~FibersToDatagrams() {
    SSF_LOG(kLogDebug) << "microservice[datagram_forwarder]: destroy";
  }

 public:
  static FibersToDatagramsPtr Create(boost::asio::io_service& io_service,
                                     Demux& fiber_demux,
                                     const Parameters& parameters) {
    if (!parameters.count("local_port") || !parameters.count("remote_ip") ||
        !parameters.count("remote_port")) {
      return FibersToDatagramsPtr(nullptr);
    }

    uint32_t local_port;
    uint32_t remote_port;
    try {
      local_port = std::stoul(parameters.at("local_port"));
      remote_port = std::stoul(parameters.at("remote_port"));
    } catch (const std::exception&) {
      SSF_LOG(kLogError) << "microservice[datagram_forwarder]: cannot extract "
                            "port parameters";
      return FibersToDatagramsPtr(nullptr);
    }

    if (remote_port > 65535) {
      SSF_LOG(kLogError) << "microservice[datagram_forwarder]: remote port ("
                         << remote_port << ") out of range ";
      return FibersToDatagramsPtr(nullptr);
    }

    return FibersToDatagramsPtr(new FibersToDatagrams(
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
      return FibersToDatagrams::Create(io_service, fiber_demux, parameters);
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

 private:
  FibersToDatagrams(boost::asio::io_service& io_service, Demux& fiber_demux,
                    LocalPortType local, const std::string& ip,
                    RemotePortType remote_port);

  void AsyncReceiveDatagram();
  void OnFiberDatagramReceive(BaseServicePtr self,
                              const boost::system::error_code& ec,
                              size_t length);

 private:
  RemotePortType remote_port_;
  std::string ip_;
  LocalPortType local_port_;
  FiberDatagram fiber_;

  Udp::endpoint to_endpoint_;
  FiberEndpoint from_endpoint_;

  WorkingBufferType working_buffer_;

  UdpOperatorPtr p_udp_operator_;
};

}  // fibers_to_datagrams
}  // services
}  // ssf

#include "services/fibers_to_datagrams/fibers_to_datagrams.ipp"

#endif  // SSF_SERVICES_FIBERS_TO_DATAGRAMS_FIBERS_TO_DATAGRAMS_H_
