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
  static FibersToDatagramsPtr create(boost::asio::io_service& io_service,
                                     Demux& fiber_demux,
                                     Parameters parameters) {
    if (!parameters.count("local_port") || !parameters.count("remote_ip") ||
        !parameters.count("remote_port")) {
      return FibersToDatagramsPtr(nullptr);
    } else {
      return std::shared_ptr<FibersToDatagrams>(new FibersToDatagrams(
          io_service, fiber_demux, std::stoul(parameters["local_port"]),
          parameters["remote_ip"],
          (uint16_t)std::stoul(parameters["remote_port"])));
    }
  }

  enum { factory_id = 5 };

  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<Demux>> p_factory, const Config& config) {
    if (!config.enabled()) {
      // service factory is not enabled
      return;
    }

    p_factory->RegisterServiceCreator(factory_id, &FibersToDatagrams::create);
  }

  virtual void start(boost::system::error_code& ec);
  virtual void stop(boost::system::error_code& ec);
  virtual uint32_t service_type_id();

  static ssf::services::admin::CreateServiceRequest<Demux> GetCreateRequest(
      local_port_type local_port, std::string remote_addr,
      uint16_t remote_port) {
    ssf::services::admin::CreateServiceRequest<Demux> create(factory_id);
    create.add_parameter("local_port", std::to_string(local_port));
    create.add_parameter("remote_ip", remote_addr);
    create.add_parameter("remote_port", std::to_string(remote_port));

    return create;
  }

 private:
  FibersToDatagrams(boost::asio::io_service& io_service, Demux& fiber_demux,
                    local_port_type local, const std::string& ip,
                    uint16_t remote_port);

 private:
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