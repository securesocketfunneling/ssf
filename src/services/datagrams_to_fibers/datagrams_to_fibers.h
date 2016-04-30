#ifndef SSF_SERVICES_DATAGRAMS_TO_FIBERS_DATAGRAMS_TO_FIBERS_H_
#define SSF_SERVICES_DATAGRAMS_TO_FIBERS_DATAGRAMS_TO_FIBERS_H_

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

namespace ssf {
namespace services {
namespace datagrams_to_fibers {

template <typename Demux>
class DatagramsToFibers : public BaseService<Demux> {
 public:
  typedef typename Demux::local_port_type local_port_type;
  typedef typename Demux::remote_port_type remote_port_type;

  typedef boost::asio::ip::udp::socket::endpoint_type remote_udp_endpoint_type;
  typedef boost::asio::ip::udp::socket socket;

  typedef typename ssf::BaseService<Demux>::Parameters Parameters;
  typedef typename ssf::BaseService<Demux>::demux demux;
  typedef typename ssf::BaseService<Demux>::fiber_datagram fiber_datagram;
  typedef typename ssf::BaseService<Demux>::datagram_endpoint datagram_endpoint;

  typedef DatagramLinkOperator<remote_udp_endpoint_type, socket,
                               datagram_endpoint, fiber_datagram> UdpOperator;
  typedef std::shared_ptr<UdpOperator> UdpOperatorPtr;
  typedef std::shared_ptr<DatagramsToFibers> DatagramsToFibersPtr;

  typedef std::array<uint8_t, 50 * 1024> WorkingBufferType;

 public:
  static DatagramsToFibersPtr Create(boost::asio::io_service& io_service,
                                     Demux& fiber_demux,
                                     Parameters parameters) {
    if (!parameters.count("local_port") || !parameters.count("remote_port")) {
      return DatagramsToFibersPtr(nullptr);
    } else {
      return std::shared_ptr<DatagramsToFibers>(
          new DatagramsToFibers(io_service, fiber_demux,
                                (uint16_t)std::stoul(parameters["local_port"]),
                                std::stoul(parameters["remote_port"])));
    }
  }

  enum { factory_id = 6 };

  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<Demux>> p_factory) {
    p_factory->RegisterServiceCreator(factory_id, &DatagramsToFibers::Create);
  }

  virtual void start(boost::system::error_code& ec);
  virtual void stop(boost::system::error_code& ec);
  virtual uint32_t service_type_id();

  static ssf::services::admin::CreateServiceRequest<Demux> GetCreateRequest(
      uint16_t local_port, remote_port_type remote_port) {
    ssf::services::admin::CreateServiceRequest<Demux> create(factory_id);
    create.add_parameter("local_port", std::to_string(local_port));
    create.add_parameter("remote_port", std::to_string(remote_port));

    return create;
  }

 private:
  DatagramsToFibers(boost::asio::io_service& io_service, Demux& fiber_demux,
                    uint16_t local, remote_port_type remote_port);

 private:
  void StartReceivingDatagrams();
  void SocketReceiveHandler(const boost::system::error_code& ec, size_t length);

  template <typename Handler, typename This>
  auto Then(Handler handler, This me)
      -> decltype(boost::bind(handler, me->SelfFromThis(), _1, _2)) {
    return boost::bind(handler, me->SelfFromThis(), _1, _2);
  }

  std::shared_ptr<DatagramsToFibers> SelfFromThis() {
    return std::static_pointer_cast<DatagramsToFibers>(
        this->shared_from_this());
  }

  uint16_t local_port_;
  remote_port_type remote_port_;
  socket socket_;

  boost::asio::ip::udp::endpoint endpoint_;
  datagram_endpoint send_to_;

  WorkingBufferType working_buffer_;

  UdpOperatorPtr p_udp_operator_;
};

}  // datagrams_to_fibers
}  // services
}  // ssf

#include "services/datagrams_to_fibers/datagrams_to_fibers.ipp"

#endif  // SSF_SERVICES_DATAGRAMS_TO_FIBERS_DATAGRAMS_TO_FIBERS_H_