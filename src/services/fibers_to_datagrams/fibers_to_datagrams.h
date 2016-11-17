#ifndef SSF_SERVICES_FIBERS_TO_DATAGRAMS_FIBERS_TO_DATAGRAMS_H_
#define SSF_SERVICES_FIBERS_TO_DATAGRAMS_FIBERS_TO_DATAGRAMS_H_

#include <cstdint>

#include <memory>

#include <boost/asio.hpp>

#include "common/boost/fiber/datagram_fiber.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"

#include "common/network/datagram_link.h"
#include "common/network/datagram_link_operator.h"

#include "services/base_service.h"

#include "core/factories/service_factory.h"

#include "services/admin/requests/create_service_request.h"
#include "services/fibers_to_datagrams/config.h"

namespace ssf {
namespace services {
namespace fibers_to_datagrams {

template <typename Demux>
class FibersToDatagrams : public BaseService<Demux> {
 private:
  typedef typename Demux::local_port_type local_port_type;
  typedef typename Demux::remote_port_type remote_port_type;

  typedef boost::asio::ip::udp::socket::endpoint_type remote_udp_endpoint_type;
  typedef boost::asio::ip::udp::socket socket;
  typedef typename ssf::BaseService<Demux>::Parameters Parameters;
  typedef typename ssf::BaseService<Demux>::demux demux;
  typedef typename ssf::BaseService<Demux>::fiber_datagram fiber_datagram;
  typedef typename ssf::BaseService<Demux>::datagram_endpoint datagram_endpoint;

  typedef DatagramLinkOperator<datagram_endpoint, fiber_datagram,
                               remote_udp_endpoint_type, socket> UdpOperator;
  typedef std::shared_ptr<UdpOperator> UdpOperatorPtr;

  typedef std::shared_ptr<FibersToDatagrams> FibersToDatagramsPtr;

  typedef std::array<uint8_t, 50 * 1024> WorkingBufferType;

 public:
  enum { factory_id = 5 };

 public:
  FibersToDatagrams() = delete;
  FibersToDatagrams(const FibersToDatagrams&) = delete;

 public:
  static FibersToDatagramsPtr create(boost::asio::io_service& io_service,
                                     Demux& fiber_demux,
                                     Parameters parameters) {
    if (!parameters.count("local_port") || !parameters.count("remote_ip") ||
        !parameters.count("remote_port")) {
      return FibersToDatagramsPtr(nullptr);
    }

    uint32_t local_port;
    uint32_t remote_port;
    try {
      local_port = std::stoul(parameters["local_port"]);
      remote_port = std::stoul(parameters["remote_port"]);
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

    return std::shared_ptr<FibersToDatagrams>(new FibersToDatagrams(
        io_service, fiber_demux, local_port, parameters["remote_ip"],
        static_cast<uint16_t>(remote_port)));
  }

  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<Demux>> p_factory, const Config& config) {
    if (!config.enabled()) {
      // service factory is not enabled
      return;
    }

    p_factory->RegisterServiceCreator(factory_id, &FibersToDatagrams::create);
  }

  static ssf::services::admin::CreateServiceRequest<Demux> GetCreateRequest(
      local_port_type local_port, std::string remote_addr,
      uint16_t remote_port) {
    ssf::services::admin::CreateServiceRequest<Demux> create(factory_id);
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
  FibersToDatagrams(boost::asio::io_service& io_service, Demux& fiber_demux,
                    local_port_type local, const std::string& ip,
                    uint16_t remote_port);

  void StartReceivingDatagrams();
  void FiberReceiveHandler(const boost::system::error_code& ec, size_t length);

  template <typename Handler, typename This>
  auto Then(Handler handler, This me)
      -> decltype(boost::bind(handler, me->SelfFromThis(), _1, _2)) {
    return boost::bind(handler, me->SelfFromThis(), _1, _2);
  }

  std::shared_ptr<FibersToDatagrams> SelfFromThis() {
    return std::static_pointer_cast<FibersToDatagrams>(
        this->shared_from_this());
  }

 private:
  uint16_t remote_port_;
  std::string ip_;
  local_port_type local_port_;
  fiber_datagram fiber_;

  boost::asio::ip::udp::endpoint endpoint_;
  datagram_endpoint received_from_;

  WorkingBufferType working_buffer_;

  UdpOperatorPtr p_udp_operator_;
};

}  // fibers_to_datagrams
}  // services
}  // ssf

#include "services/fibers_to_datagrams/fibers_to_datagrams.ipp"

#endif  // SSF_SERVICES_FIBERS_TO_DATAGRAMS_FIBERS_TO_DATAGRAMS_H_