#ifndef SSF_SERVICES_FIBERS_TO_DATAGRAMS_FIBERS_TO_DATAGRAMS_IPP_
#define SSF_SERVICES_FIBERS_TO_DATAGRAMS_FIBERS_TO_DATAGRAMS_IPP_

#include <ssf/log/log.h>

#include "common/error/error.h"

namespace ssf {
namespace services {
namespace fibers_to_datagrams {
template <typename Demux>
FibersToDatagrams<Demux>::FibersToDatagrams(boost::asio::io_service& io_service,
                                            Demux& fiber_demux,
                                            local_port_type local_port,
                                            const std::string& ip,
                                            uint16_t remote_port)
    : ssf::BaseService<Demux>::BaseService(io_service, fiber_demux),
      remote_port_(remote_port),
      ip_(ip),
      local_port_(local_port),
      fiber_(io_service),
      received_from_(fiber_demux, 0),
      p_udp_operator_(UdpOperator::Create(fiber_)) {}

template <typename Demux>
void FibersToDatagrams<Demux>::start(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "microservice[datagram_forwarder]: start forwarding "
                       "fiber datagrams from fiber port " << local_port_
                    << " to <" << ip_ << ":" << remote_port_ << ">";

  // fiber.open()
  fiber_.bind(datagram_endpoint(this->get_demux(), local_port_), ec);

  // Resolve the given address
  boost::asio::ip::udp::resolver resolver(this->get_io_service());
  boost::asio::ip::udp::resolver::query query(ip_,
                                              std::to_string(remote_port_));
  boost::asio::ip::udp::resolver::iterator iterator(
      resolver.resolve(query, ec));

  if (!ec) {
    endpoint_ = *iterator;
    this->StartReceivingDatagrams();
  }
}

template <typename Demux>
void FibersToDatagrams<Demux>::stop(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "microservice[datagram_forwarder]: stopping";
  ec.assign(::error::success, ::error::get_ssf_category());

  fiber_.close();
  p_udp_operator_->StopAll();
}

template <typename Demux>
uint32_t FibersToDatagrams<Demux>::service_type_id() {
  return factory_id;
}

template <typename Demux>
void FibersToDatagrams<Demux>::StartReceivingDatagrams() {
  SSF_LOG(kLogTrace)
      << "microservice[datagram_forwarder]: receiving new datagrams";

  fiber_.async_receive_from(
      boost::asio::buffer(working_buffer_), received_from_,
      Then(&FibersToDatagrams::FiberReceiveHandler, this->SelfFromThis()));
}

template <typename Demux>
void FibersToDatagrams<Demux>::FiberReceiveHandler(
    const boost::system::error_code& ec, size_t length) {
  if (!ec) {
    auto already_in = p_udp_operator_->Feed(
        received_from_, boost::asio::buffer(working_buffer_), length);

    if (!already_in) {
      boost::asio::ip::udp::socket left(
          this->get_io_service(),
          boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0));
      p_udp_operator_->AddLink(std::move(left), received_from_, endpoint_,
                               this->get_io_service());
      p_udp_operator_->Feed(received_from_,
                            boost::asio::buffer(working_buffer_), length);
    }

    this->StartReceivingDatagrams();
  } else {
    // boost::system::error_code ec;
    // stop(ec);
  }
}

}  // fibers_to_datagrams
}  // services
}  // ssf

#endif  // SSF_SERVICES_FIBERS_TO_DATAGRAMS_FIBERS_TO_DATAGRAMS_IPP_
